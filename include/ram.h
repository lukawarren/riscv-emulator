#pragma once
#include "bus_device.h"

class RAM : public BusDevice
{
public:
    RAM(const uint64_t size);
    ~RAM();

    std::optional<u64> read_byte(const u64 address) override;
    bool write_byte(const u64 address, const u8 value) override;

private:
    u8* memory;
    uint64_t size;
};
