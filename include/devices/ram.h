#pragma once
#include "devices/bus_device.h"

class RAM : public BusDevice
{
public:
    RAM(const u64 size);
    ~RAM();

    std::optional<u64> read_byte(const u64 address) override;
    bool write_byte(const u64 address, const u8 value) override;

    // For RAM, it is quicker to simply memcpy rather than
    // deal with all the bitwise that the parent BusDevice does.
    // This cannot so easily be done for over devices on the bus,
    // so intead we have a "fast-path" for RAM specifically.
    template<typename T> T read_fast_path(const u64 address);
    template<typename T> void write_fast_path(const u64 address, const T value);

    std::optional<u16> read_16(const u64 address) override;
    std::optional<u32> read_32(const u64 address) override;
    std::optional<u64> read_64(const u64 address) override;
    bool write_16(const u64 address, const u16 value) override;
    bool write_32(const u64 address, const u32 value) override;
    bool write_64(const u64 address, const u64 value) override;

    uint64_t size;
private:
    u8* memory;
};
