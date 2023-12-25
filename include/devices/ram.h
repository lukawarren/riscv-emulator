#pragma once
#include "devices/bus_device.h"

class RAM : public BusDevice
{
public:
    RAM(const u64 size);
    ~RAM();

    std::optional<u64> read_byte(const u64 address) override;
    bool write_byte(const u64 address, const u8 value) override;

    uint64_t size;
private:
    u8* memory;
};
