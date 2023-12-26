#pragma once
#include "devices/bus_device.h"

class UART : public BusDevice
{
public:
    std::optional<u64> read_byte(const u64 address) override;
    bool write_byte(const u64 address, const u8 value) override;
};
