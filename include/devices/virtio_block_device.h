#pragma once
#include "devices/register_device.h"

class CPU;
class PLIC;

/*
    Implements a VirtIO block device as per:
    https://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.pdf
*/
class VirtioBlockDevice : public RegisterDevice
{
public:
    VirtioBlockDevice(const std::optional<std::string> image);
    ~VirtioBlockDevice();
    void clock(CPU& cpu, PLIC& plic);

protected:
    u32* get_register(const u64 address, const Mode mode) override;

private:
    // Registers common to all virtio devices
    struct CommonRegisters
    {
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
        u32 interrupt_status = 1;       // all interrupts will be due from updating the used ring
        u32 interrupt_ack = 0;
        u32 status = 0;
        u64 queue_desc = 0;
        u64 queue_avail = 0;
        u64 queue_used = 0;
        u32 config_generation = 0;
    } common_registers;

    // Registers for block devices
    struct BlockRegisters
    {
        u64 capacity = 0;
    } block_registers;

    // Internal state
    bool wrote_to_queue_notify = false;
    bool wrote_to_interrupt_ack = false;
    bool wrote_to_status = false;
    u16 last_processed_idx = 0;
    u8* image = nullptr;
    int image_fd;

    constexpr static u32 max_queue_size = 32768;
    struct Queue
    {
        u32 size = 0;
        u32 max_size = max_queue_size;
        u32 ready = false;
    } requestq;

    struct QueueDescription
    {
        u64 address;
        u32 length;
        u16 flags;
        u16 next;

        bool has_next_field() const { return (flags & 1) != 0; }
        bool is_device_write_only() const { return (flags & 2) != 0; }
        bool is_indirect() const { return (flags & 4) != 0; }
    }  __attribute__((packed));

    // Used to offer buffers to us (the device)
    struct QueueAvailable
    {
        u16 flags;
        u16 idx;
        u16 ring[max_queue_size];

        bool no_interrupt() const { return (flags & 1) != 0; }
    } __attribute__((packed));

    struct QueueUsedElement
    {
        u32 id;
        u32 length;
    } __attribute__((packed));

    // Used to return buffers to the guest once we're done
    struct QueueUsed
    {
        u16 flags;
        u16 idx;
        QueueUsedElement ring[max_queue_size];

        bool no_notify() const { return (flags & 1) != 0; }
    } __attribute__((packed));

    // A complete "packet" consists of three descriptions - a header detailing
    // the operation (R/W), a variable length buffer to hold the data, then a
    // footer to hold the final return status.
    struct BlockDeviceHeader
    {
        enum class Type : u32
        {
            Read = 0,
            Write = 1,
            Flush = 4,
            GetID = 8
        } type;
        u32 reserved;
        u64 sector;
    } __attribute__((packed));
    struct BlockDeviceFooter
    {
        enum class Status : u8
        {
            Ok = 0,
            IOError = 1,
            Unsupported = 2
        } status;
    } __attribute__((packed));
    BlockDeviceHeader current_header;

private:
    void reset_device();
    void process_queue_buffers(CPU& cpu, PLIC& plic);
    u32 process_queue_description(CPU& cpu, const QueueDescription& description, u16 local_index);
    template<typename T> T get_structure(CPU& cpu, u64 address);
    template<typename T> void set_structure(CPU& cpu, u64 address, T& structure);
};