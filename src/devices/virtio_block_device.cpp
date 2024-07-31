#include "devices/virtio_block_device.h"

// Registers common to all virtio devices
#define MAGIC_VALUE             0x00
#define VERSION                 0x04
#define DEVICE_ID               0x08
#define VENDOR_ID               0x0c
#define DEVICE_FEATURES         0x10
#define DEVICE_FEATURES_SELECT  0x14
#define DRIVER_FEATURES         0x20
#define DRIVER_FEATURES_SELECT  0x24
#define QUEUE_SELECT            0x30
#define QUEUE_NUM_MAX           0x34
#define QUEUE_NUM               0x38
#define QUEUE_READY             0x44
#define QUEUE_NOTIFY            0x50
#define INTERRUPT_STATUS        0x60
#define INTERRUPT_ACK           0x64
#define STATUS                  0x70
#define QUEUE_DESC_LOW          0x80
#define QUEUE_DESC_HIGH         0x84
#define QUEUE_AVAIL_LOW         0x90
#define QUEUE_AVAIL_HIGH        0x94
#define QUEUE_USED_LOW          0xa0
#define QUEUE_USED_HIGH         0xa4
#define CONFIG_GENERATION       0x0fc
#define CONFIG                  0x100

VirtioBlockDevice::VirtioBlockDevice() {}

u32* VirtioBlockDevice::get_register(const u64 address, const Mode mode)
{
    switch (mode)
    {
        case Mode::Read:
        {
            switch (address)
            {
                case MAGIC_VALUE:       return &magic_value;
                case VERSION:           return &version;
                case DEVICE_ID:         return &device_id;
                case VENDOR_ID:         return &vendor_id;
                case DEVICE_FEATURES:   return &device_features;
                case QUEUE_NUM_MAX:     return &queue_number_max;
                case QUEUE_READY:       return &queue_ready;
                case INTERRUPT_STATUS:  return &interrupt_status;;
                case STATUS:            return &status;
                case CONFIG_GENERATION: return &config_generation;
                case CONFIG:            return &config;

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
                case DEVICE_FEATURES_SELECT: return &device_feature_select;
                case DRIVER_FEATURES:        return &driver_features;
                case DRIVER_FEATURES_SELECT: return &driver_features_select;
                case QUEUE_SELECT:           return &queue_select;
                case QUEUE_NUM:              return &queue_number;
                case QUEUE_READY:            return &queue_ready;
                case QUEUE_NOTIFY:           return &queue_notify;
                case INTERRUPT_ACK:          return &interrupt_ack;
                case STATUS:                 return &status;
                case QUEUE_DESC_LOW:         return &queue_desc_low;
                case QUEUE_DESC_HIGH:        return &queue_desc_high;
                case QUEUE_USED_LOW:         return &queue_used_low;
                case QUEUE_USED_HIGH:        return &queue_used_high;
                case CONFIG:                 return &config;

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
