#pragma once
#include "devices/register_device.h"

/*
    Implements a VirtIO block device as per:
    https://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.pdf
*/
class VirtioBlockDevice : public RegisterDevice
{
public:
    VirtioBlockDevice();
    ~VirtioBlockDevice();

protected:
    u32* get_register(const u64 address, const Mode mode) override;

private:
    // Registers common to all virtio devices
    u32 magic_value = 0x74726976;   // spells "virt"
    u32 version = 2;                // correct as of virtio 1.0
    u32 device_id = 2;              // block device
    u32 vendor_id = 0;
    u64 device_features;
    u32 device_feature_select = 0;
    u64 driver_features = 0;
    u32 driver_features_select = 0;
    u32 queue_select = 0;
    u32 queue_number_max = 0;
    u32 queue_number = 0;
    u32 queue_ready = 0;
    u32 queue_notify = 0;
    u32 interrupt_status = 0;
    u32 interrupt_ack = 0;
    u32 status = 0;
    u32 queue_desc_low = 0;
    u32 queue_desc_high = 0;
    u32 queue_avail_low = 0;
    u32 queue_avail_high = 0;
    u32 queue_used_low = 0;
    u32 queue_used_high = 0;
    u32 config_generation = 0;

    // Registers for block devices
    u64 capacity = 0;

    // Queues
    struct Queue
    {
        u32 size = 0;
        u32 max_size = 8192;
        u32 ready = false;
    };
    Queue requestq;
};
