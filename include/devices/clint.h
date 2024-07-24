#pragma once
#include "devices/bus_device.h"

class CPU;

class CLINT : public BusDevice
{
public:
    std::optional<u64> read_byte(const u64 address) override;
    bool write_byte(const u64 address, const u8 value) override;
    void increment(CPU& cpu);

private:
    u32 msip = 0;
    u64 mtimecmp = 0;
    u64 mtime = 0;
};
