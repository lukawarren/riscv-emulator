#include "devices/virtio_block_device.h"

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

VirtioBlockDevice::VirtioBlockDevice()
{
    // Setup features
    device_features = 0;
    device_features |= FEATURE_VIRTIO_F_VERSION_1;
    device_features |= FEATURE_VIRTIO_BLK_F_FLUSH;

    // The number of 512-byte logical blocks
    capacity = (10 * 1024 * 1024) / 512;
}

u32* VirtioBlockDevice::get_register(const u64 address, const Mode mode)
{
    static int i = 0;
    if (((i++) % 4) == 0 &&
        address != MAGIC_VALUE &&
        address != VERSION &&
        address != DEVICE_ID &&
        address != VENDOR_ID &&
        address != DRIVER_FEATURES &&
        address != DRIVER_FEATURES_SELECT &&
        address != DEVICE_FEATURES &&
        address != DEVICE_FEATURES_SELECT &&
        address != STATUS &&
        address != QUEUE_SELECT &&
        address != QUEUE_READY &&
        address != QUEUE_NUM_MAX &&
        address != QUEUE_NUM &&
        address != QUEUE_DESC_LOW &&
        address != QUEUE_DESC_HIGH &&
        address != QUEUE_USED_LOW &&
        address != QUEUE_USED_HIGH &&
        address != QUEUE_AVAIL_LOW &&
        address != QUEUE_AVAIL_HIGH &&
        address != CONFIG_GENERATION &&
        address != CAPACITY_LOW &&
        address != CAPACITY_HIGH)
    dbg(dbg::hex(address), mode);

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
                case INTERRUPT_STATUS:  return &interrupt_status;;
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
                case QUEUE_NOTIFY:           return &queue_notify;
                case INTERRUPT_ACK:          return &interrupt_ack;
                case STATUS:                 return &status;
                case QUEUE_DESC_LOW:         return &queue_desc_low;
                case QUEUE_DESC_HIGH:        return &queue_desc_high;
                case QUEUE_AVAIL_LOW:        return &queue_avail_low;
                case QUEUE_AVAIL_HIGH:       return &queue_avail_high;
                case QUEUE_USED_LOW:         return &queue_used_low;
                case QUEUE_USED_HIGH:        return &queue_used_high;

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

VirtioBlockDevice::~VirtioBlockDevice() {}
