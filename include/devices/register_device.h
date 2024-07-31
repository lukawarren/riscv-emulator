#pragma once
#include "devices/bus_device.h"

/*
    Represents a bus device consisting of 32-bit little-endian registers
*/
class RegisterDevice : public BusDevice
{
public:
    std::optional<u64> read_byte(const u64 address)
    {
        const u32* reg = get_register(round_address_to_nearest_word(address), Mode::Read);
        if (reg == nullptr) return { 0 };

        switch (address % 4)
        {
            case 0: return { (*reg >>  0) & 0xff };
            case 1: return { (*reg >>  8) & 0xff };
            case 2: return { (*reg >> 16) & 0xff };
            case 3: return { (*reg >> 24) & 0xff };
        }

        return { 0 };
    }

    bool write_byte(const u64 address, const u8 value)
    {
        u32* reg = get_register(round_address_to_nearest_word(address), Mode::Write);
        if (reg == nullptr) return true;

        switch (address % 4)
        {
            case 0: *reg = (*reg & 0xffffff00) | ((u32)value <<  0); break;
            case 1: *reg = (*reg & 0xffff00ff) | ((u32)value <<  8); break;
            case 2: *reg = (*reg & 0xff00ffff) | ((u32)value << 16); break;
            case 3: *reg = (*reg & 0x00ffffff) | ((u32)value << 24); break;
        }

        return true;
    }

protected:
    enum class Mode { Read, Write };
    virtual u32* get_register(const u64 address, const Mode mode) = 0;

private:
    static u64 round_address_to_nearest_word(const u64 address)
    {
        return address - (address % 4);
    }
};
