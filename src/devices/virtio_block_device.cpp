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

#define BLOCK_SIZE                  512

VirtioBlockDevice::VirtioBlockDevice(const std::optional<std::string> image)
{
    // Setup features
    device_features = 0;
    device_features |= FEATURE_VIRTIO_F_VERSION_1;
    device_features |= FEATURE_VIRTIO_BLK_F_FLUSH;

    // If we don't actually have an image to play with, let's mess with
    // the magic so Linux will ignore us, because I can't be bothered
    // modifying the device tree
    if (!image.has_value())
        magic_value = 0;
    else
    {
        std::pair<u8*, size_t> file = io_read_file(image.value());
        size_t file_length;

        // If we aren't aligned to a sector size, we're going to be
        // in trouble. Let's just re-allocate a new buffer (since true,
        // valid images should be aligned (e.g. ext4), whereas small test
        // files might not be, but performance cost will be negligible).
        if (file.second % BLOCK_SIZE != 0)
        {
            file_length = ((file.second + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
            u8* new_buffer = new u8[file_length];
            memset(new_buffer, 0, file_length);
            memcpy(new_buffer, file.first, file.second);
            delete[] file.first;
            this->image = new_buffer;
        }
        else
        {
            this->image = file.first;
            file_length = file.second;
        }

        capacity = file_length / BLOCK_SIZE;
    }
}

void VirtioBlockDevice::clock(CPU& cpu, PLIC& plic)
{
    // "Writing a value with bits set as defined in InterruptStatus to this
    // register notifies the device that events causing the interrupt have been
    // handled."
    if (interrupt_ack == interrupt_status)
    {
        interrupt_ack = 0;
        plic.clear_interrupt_pending(PLIC_INTERRUPT_BLK);
    }

    if (wrote_to_queue_notify)
    {
        wrote_to_queue_notify = false;
        process_queue_buffers(cpu, plic);
    }
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
    auto* descriptions = get_structure<QueueDescription>(cpu, queue_desc);
    auto* available = get_structure<QueueAvailable>(cpu, queue_avail);
    auto* used = get_structure<QueueUsed>(cpu, queue_used);

    // The available buffer contains buffers offered to us (the device)
    u16 ring_index = last_processed_idx;
    while (true)
    {
        u16 descriptor_index = available->ring[ring_index];
        ring_index += 1;
        ring_index %= max_queue_size;

        // Process entire description chain
        u16 local_index = 0;
        while (true)
        {
            QueueDescription* description = descriptions + descriptor_index;
            assert(description->is_indirect() == false);
            u32 length_written = process_queue_description(cpu, description, local_index++);

            // Place processed description into used ring if applicable;
            // I'd personally place them all but Linux seems to only want
            // the very first
            if (local_index == 1)
            {
                used->ring[used->idx].id = descriptor_index;
                used->ring[used->idx].length = length_written;
                used->idx += 1;
                used->idx %= max_queue_size;
            }

            if (description->has_next_field())
                descriptor_index++;
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
        // The spec defines "virtio_blk_req.data" to have a length of 512,
        // but Linux seems to use 4096. Limiting the length to 512 and returning
        // the proper "written amount" just results in memory corruption, so
        // instead I am just assuming there is something else in the spec I
        // misread or forgot to read.
        u32 length = description->length;
        u8* data = get_structure<u8>(cpu, description->address);

        if (current_header->type == BlockDeviceHeader::Type::Read &&
            description->is_device_write_only())
        {
            // Read from image
            u8* image_buffer = image + current_header->sector * BLOCK_SIZE;
            memcpy(data, image_buffer, length);

            next_footer.status = BlockDeviceFooter::Status::Ok;
            ret = length;
        }
        else if (current_header->type == BlockDeviceHeader::Type::Write &&
            !description->is_device_write_only())
        {
            throw std::runtime_error("todo: write");
        }
        else
            throw std::runtime_error("unsupported virtio_blk_req type");
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
        if (queue_select != 0)
            throw std::runtime_error("invalid virtio QueueSel");
    };

    switch (mode)
    {
        case Mode::Read:
        {
            switch (address)
            {
                // Common registers
                case MAGIC_VALUE:       return &magic_value;
                case VERSION:           return &version;
                case DEVICE_ID:         return &device_id;
                case VENDOR_ID:         return &vendor_id;
                case DEVICE_FEATURES:
                {
                    if (device_feature_select >= 2)
                        throw std::runtime_error("invalid virtio DeviceFeaturesSel");
                    return (u32*)&device_features + device_feature_select;
                }
                case QUEUE_NUM_MAX:
                {
                    check_queue();
                    return &requestq.max_size;
                }
                case QUEUE_READY:       return &queue_ready;
                case INTERRUPT_STATUS:  return &interrupt_status;
                case STATUS:            return &status;
                case CONFIG_GENERATION: return &config_generation;

                // Block device registers
                case CAPACITY_LOW:      return (u32*)&capacity + 0;
                case CAPACITY_HIGH:     return (u32*)&capacity + 1;

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
                case DEVICE_FEATURES_SELECT: return &device_feature_select;
                case DRIVER_FEATURES:
                {
                    if (driver_features_select >= 2)
                        throw std::runtime_error("invalid virtio DriverFeaturesSel");
                    return (u32*)&driver_features + device_feature_select;
                }
                case DRIVER_FEATURES_SELECT: return &driver_features_select;
                case QUEUE_SELECT:           return &queue_select;
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
                    return &queue_notify;
                }
                case INTERRUPT_ACK:          return &interrupt_ack;
                case STATUS:                 return &status; // TODO: writing 0 = device reset
                case QUEUE_DESC_LOW:         return (u32*)&queue_desc + 0;
                case QUEUE_DESC_HIGH:        return (u32*)&queue_desc + 1;
                case QUEUE_AVAIL_LOW:        return (u32*)&queue_avail + 0;
                case QUEUE_AVAIL_HIGH:       return (u32*)&queue_avail + 1;
                case QUEUE_USED_LOW:         return (u32*)&queue_used + 0;
                case QUEUE_USED_HIGH:        return (u32*)&queue_used + 1;

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
    delete[] image;
}
