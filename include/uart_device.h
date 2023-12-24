#pragma once
#include "bus_device.h"
#include <iostream>

class UARTDevice : public BusDevice
{
public:
    std::optional<u64> read_byte(const u64 address) override
    {
        return { 0 };
    }

    bool write_byte(const u64 address, const u8 value) override
    {
        std::cout << value;
        return true;
    }
};
