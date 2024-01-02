#pragma once
#include "devices/bus_device.h"

class ErrorDevice : public BusDevice
{
public:
    std::optional<u64> read_byte(const u64 address) override
    {
        return std::nullopt;
    }

    bool write_byte(const u64 address, const u8 value) override
    {
        return false;
    }
};
