#include "devices/virtio_block_device.h"
#include "cpu.h"
#include "io.h"

// Registers common to all virtio devices
#define MAGIC_VALUE                 0x00
#define VERSION                     0x04
#define DEVICE_ID                   0x08
#define VENDOR_ID                   0x0c
#define DEVICE_FEATURES             0x10
#define DEVICE_FEATURES_SELECT      0x14
#define DRIVER_FEATURES             0x20
#define DRIVER_FEATURES_SELECT      0x24
#define QUEUE_SELECT                0x30
#define QUEUE_NUM_MAX               0x34
#define QUEUE_NUM                   0x38
#define QUEUE_READY                 0x44
#define QUEUE_NOTIFY                0x50
#define INTERRUPT_STATUS            0x60
#define INTERRUPT_ACK               0x64
#define STATUS                      0x70
#define QUEUE_DESC_LOW              0x80
#define QUEUE_DESC_HIGH             0x84
#define QUEUE_AVAIL_LOW             0x90
#define QUEUE_AVAIL_HIGH            0x94
#define QUEUE_USED_LOW              0xa0
#define QUEUE_USED_HIGH             0xa4
#define CONFIG_GENERATION           0xfc

// Block device registers
#define CAPACITY_LOW                0x100
#define CAPACITY_HIGH               0x104

// Features
#define FEATURE_VIRTIO_F_VERSION_1  (1UL << 32)
#define FEATURE_VIRTIO_BLK_F_FLUSH  (1UL << 9)
#define FEATURE_VIRTIO_BLK_F_RO     (1UL << 5)

// Status flags
#define STATUS_DRIVER_OK            4

#define BLOCK_SIZE                  512

VirtioBlockDevice::VirtioBlockDevice(const std::optional<std::string> image)
{
    // Setup features
    common_registers.device_features = 0;
    common_registers.device_features |= FEATURE_VIRTIO_F_VERSION_1;
    common_registers.device_features |= FEATURE_VIRTIO_BLK_F_FLUSH;

    // If we don't actually have an image to play with, let's mess with
    // the magic so Linux will ignore us, because I can't be bothered
    // modifying the device tree
    if (!image.has_value())
        common_registers.magic_value = 0;
    else
    {
        auto [buffer, size, fd] = io_map_file(*image);

        // If we aren't aligned to a sector size, we're going to be
        // in trouble. But that shouldn't happen to valid images...
        if (size % BLOCK_SIZE != 0)
            throw std::runtime_error("invalid virtio image - not aligned to 512 block size");

        this->image = buffer;
        block_registers.capacity = size / BLOCK_SIZE;
        image_fd = fd;
    }
}

void VirtioBlockDevice::clock(CPU& cpu, PLIC& plic)
{
    if (wrote_to_interrupt_ack)
    {
        wrote_to_interrupt_ack = false;

        // "Writing a value with bits set as defined in InterruptStatus to this
        // register notifies the device that events causing the interrupt have been
        // handled."
        if (common_registers.interrupt_ack == common_registers.interrupt_status) [[likely]]
        {
            common_registers.interrupt_ack = 0;
            plic.clear_interrupt_pending(PLIC_INTERRUPT_BLK);
        }
        else throw std::runtime_error("unknown virtio interrupt_ack " + common_registers.interrupt_ack);
    }

    if (wrote_to_queue_notify)
    {
        // "The device MUST NOT consume buffers or notify the driver before DRIVER_OK"
        if ((common_registers.status & STATUS_DRIVER_OK) != 0)
        {
            wrote_to_queue_notify = false;
            process_queue_buffers(cpu, plic);
        }
    }

    if (wrote_to_status)
    {
        wrote_to_status = false;
        if (common_registers.status == 0)
            reset_device();
    }
}

void VirtioBlockDevice::reset_device()
{
    // Writing zero to status triggers a device reset
    // We need to preserve the magic value and capacity
    u32 magic_value = common_registers.magic_value;
    u64 capacity = block_registers.capacity;
    common_registers = CommonRegisters {};
    block_registers = BlockRegisters {};
    common_registers.magic_value = magic_value;
    block_registers.capacity = capacity;
}

template<typename T>
T* VirtioBlockDevice::get_structure(CPU& cpu, u64 address)
{
    // Assuming a little-endian host simplifies a lost of code.
    // I don't even have a big-endian CPU to test this on, so
    // unfortunately... TODO: support big endian
    static_assert(std::endian::native == std::endian::little);
    assert(address - cpu.bus.ram_base <= cpu.bus.ram.size - sizeof(T));
    u8* addr = cpu.bus.ram.memory + (address - cpu.bus.ram_base);
    return (T*)addr;
}

void VirtioBlockDevice::process_queue_buffers(CPU& cpu, PLIC& plic)
{
    auto* descriptions = get_structure<QueueDescription>(cpu, common_registers.queue_desc);
    auto* available = get_structure<QueueAvailable>(cpu, common_registers.queue_avail);
    auto* used = get_structure<QueueUsed>(cpu, common_registers.queue_used);

    // The available buffer contains buffers offered to us (the device)
    u16 ring_index = last_processed_idx;
    while (true)
    {
        u16 descriptor_index = available->ring[ring_index];
        ring_index += 1;
        ring_index %= max_queue_size;

        // Process entire description chain
        u16 local_index = 0;
        u16 head_index = 0;
        while (true)
        {
            QueueDescription* description = descriptions + descriptor_index;
            assert(description->is_indirect() == false);
            u32 length_written = process_queue_description(cpu, description, local_index);

            // We are meant to place the head of the chain in the used buffer
            // (i.e. the ID is the index of the head), but the length has to
            // be the *total* length, which we don't know until the second
            // entry in the chain.
            if (local_index == 1)
            {
                used->ring[used->idx].id = head_index;
                used->ring[used->idx].length = length_written;
                used->idx += 1;
                used->idx %= max_queue_size;
            }
            else if (local_index == 0)
                head_index = descriptor_index;

            local_index++;
            if (description->has_next_field())
                descriptor_index = description->next;
            else
                break;
        }

        // Break out if we're reached the end of the ring buffer
        if (ring_index == available->idx)
            break;
    }
    last_processed_idx = available->idx;

    if (!available->no_interrupt())
        plic.set_interrupt_pending(PLIC_INTERRUPT_BLK);
}

u32 VirtioBlockDevice::process_queue_description(
    CPU& cpu,
    QueueDescription* description,
    u16 local_index
)
{
    u32 ret = 0;

    // Header; keep track for later
    if (description->length == sizeof(BlockDeviceHeader) && local_index == 0)
        current_header = get_structure<BlockDeviceHeader>(cpu, description->address);

    // The data itself; perform actual work
    else if (local_index == 1)
    {
        // Version 1.0 of the spec has the length at a fixed 512, but later
        // versions do away with this requirement
        u32 length = description->length;
        u8* data = get_structure<u8>(cpu, description->address);
        u8* image_buffer = image + current_header->sector * BLOCK_SIZE;

        if (current_header->type == BlockDeviceHeader::Type::Read &&
            description->is_device_write_only())
        {
            memcpy(data, image_buffer, length);
            ret = length;
        }
        else if (current_header->type == BlockDeviceHeader::Type::Write &&
            !description->is_device_write_only())
        {
            memcpy(image_buffer, data, length);
        }
        else if (current_header->type == BlockDeviceHeader::Type::Flush)
        {
            io_flush_file(image_buffer, length);
        }
        else if (current_header->type == BlockDeviceHeader::Type::GetID)
        {
            // In newer versions of the spec (we're 1.0)
            // Debian doesn't care and will give it a go anyway
            const char* id = "riscv-emulator";
            memcpy(data, id, strlen(id) + 1);
            ret = strlen(id) + 1;
        }
        else
            throw std::runtime_error("unsupported virtio_blk_req type " +
                std::to_string((int)current_header->type));

        next_footer.status = BlockDeviceFooter::Status::Ok;
    }
    else if (description->length == sizeof(BlockDeviceFooter) && local_index == 2)
    {
        // Footer
        *get_structure<BlockDeviceFooter>(cpu, description->address) = next_footer;
    }
    else
        throw std::runtime_error(
            "unsupported queue description with length " +
                std::to_string(description->length)
        );

    return ret;
}

u32* VirtioBlockDevice::get_register(const u64 address, const Mode mode)
{
    const auto check_queue = [&]()
    {
        if (common_registers.queue_select != 0)
            throw std::runtime_error("invalid virtio QueueSel");
    };

    switch (mode)
    {
        case Mode::Read:
        {
            switch (address)
            {
                // Common registers
                case MAGIC_VALUE:       return &common_registers.magic_value;
                case VERSION:           return &common_registers.version;
                case DEVICE_ID:         return &common_registers.device_id;
                case VENDOR_ID:         return &common_registers.vendor_id;
                case DEVICE_FEATURES:
                {
                    if (common_registers.device_feature_select >= 2)
                        throw std::runtime_error("invalid virtio DeviceFeaturesSel");
                    return (u32*)&common_registers.device_features + common_registers.device_feature_select;
                }
                case QUEUE_NUM_MAX:
                {
                    check_queue();
                    return &requestq.max_size;
                }
                case QUEUE_READY:       return &common_registers.queue_ready;
                case INTERRUPT_STATUS:  return &common_registers.interrupt_status;
                case STATUS:            return &common_registers.status;
                case CONFIG_GENERATION: return &common_registers.config_generation;

                // Block device registers
                case CAPACITY_LOW:      return (u32*)&block_registers.capacity + 0;
                case CAPACITY_HIGH:     return (u32*)&block_registers.capacity + 1;

                default:
                    throw std::runtime_error(std::format(
                        "unknown virtio block device register read 0x{:x}",
                        address
                    ));
            }
        }

        case Mode::Write:
        {
            switch (address)
            {
                // Common registers
                case DEVICE_FEATURES_SELECT: return &common_registers.device_feature_select;
                case DRIVER_FEATURES:
                {
                    if (common_registers.driver_features_select >= 2)
                        throw std::runtime_error("invalid virtio DriverFeaturesSel");
                    return (u32*)&common_registers.driver_features + common_registers.device_feature_select;
                }
                case DRIVER_FEATURES_SELECT: return &common_registers.driver_features_select;
                case QUEUE_SELECT:           return &common_registers.queue_select;
                case QUEUE_NUM:
                {
                    check_queue();
                    return &requestq.size;
                }
                case QUEUE_READY:
                {
                    check_queue();
                    return &requestq.ready;
                }
                case QUEUE_NOTIFY:
                {
                    wrote_to_queue_notify = true;
                    return &common_registers.queue_notify;
                }
                case INTERRUPT_ACK:
                {
                    wrote_to_interrupt_ack = true;
                    return &common_registers.interrupt_ack;
                }
                case STATUS:
                {
                    wrote_to_status = true;
                    return &common_registers.status;
                }
                case QUEUE_DESC_LOW:   return (u32*)&common_registers.queue_desc + 0;
                case QUEUE_DESC_HIGH:  return (u32*)&common_registers.queue_desc + 1;
                case QUEUE_AVAIL_LOW:  return (u32*)&common_registers.queue_avail + 0;
                case QUEUE_AVAIL_HIGH: return (u32*)&common_registers.queue_avail + 1;
                case QUEUE_USED_LOW:   return (u32*)&common_registers.queue_used + 0;
                case QUEUE_USED_HIGH:  return (u32*)&common_registers.queue_used + 1;

                default:
                    throw std::runtime_error(std::format(
                        "unknown virtio block device register write 0x{:x}",
                        address
                    ));
            }
        }

        default:
            return nullptr;
    }
}

VirtioBlockDevice::~VirtioBlockDevice()
{
    if (image != nullptr)
        io_unmap_file(image, block_registers.capacity * BLOCK_SIZE, image_fd);
}
