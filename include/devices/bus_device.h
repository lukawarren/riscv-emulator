#pragma once
#include "types.h"
#include <optional>
#include <array>
#include <optional>

class BusDevice
{
public:
    inline std::optional<u8> read_8(const u64 address)
    {
        return read_byte(address);
    }

    inline std::optional<u16> read_16(const u64 address)
    {
        std::array<std::optional<u64>, 2> values =
        {
            read_byte(address + 0),
            read_byte(address + 1)
        };

        for (const auto& opt : values)
            if (!opt.has_value())
                return std::nullopt;

        return (*values[0] << 0) |
            (*values[1] << 8);
    }

    inline std::optional<u32> read_32(const u64 address)
    {
        std::array<std::optional<u64>, 4> values =
        {
            read_byte(address + 0),
            read_byte(address + 1),
            read_byte(address + 2),
            read_byte(address + 3)
        };

        for (const auto& opt : values)
            if (!opt.has_value())
                return std::nullopt;

        return (*values[0] << 0)  |
               (*values[1] << 8)  |
               (*values[2] << 16) |
               (*values[3] << 24);
    }

    inline std::optional<u64> read_64(const u64 address)
    {
        std::array<std::optional<u64>, 8> values =
        {
            read_byte(address + 0),
            read_byte(address + 1),
            read_byte(address + 2),
            read_byte(address + 3),
            read_byte(address + 4),
            read_byte(address + 5),
            read_byte(address + 6),
            read_byte(address + 7)
        };

        for (const auto& opt : values)
            if (!opt.has_value())
                return std::nullopt;

        return (*values[0] << 0)  |
            (*values[1] << 8)  |
            (*values[2] << 16) |
            (*values[3] << 24) |
            (*values[4] << 32) |
            (*values[5] << 40) |
            (*values[6] << 48) |
            (*values[7] << 56);
    }

    inline bool write_8(const u64 address, const u8 value)
    {
        return write_byte(address, value);
    }

    inline bool write_16(const u64 address, const u16 value)
    {
        return
            write_byte(address, value) &&
            write_byte(address + 1, (value >> 8) & 0xff);
    }

    inline bool write_32(const u64 address, const u32 value)
    {
        return
            write_byte(address, value) &&
            write_byte(address + 1, (value >> 8)  & 0xff) &&
            write_byte(address + 2, (value >> 16) & 0xff) &&
            write_byte(address + 3, (value >> 24) & 0xff);
    }

    inline bool write_64(const u64 address, const u64 value)
    {
        return
            write_byte(address, value) &&
            write_byte(address + 1, (value >> 8)  & 0xff) &&
            write_byte(address + 2, (value >> 16) & 0xff) &&
            write_byte(address + 3, (value >> 24) & 0xff) &&
            write_byte(address + 4, (value >> 32) & 0xff) &&
            write_byte(address + 5, (value >> 40) & 0xff) &&
            write_byte(address + 6, (value >> 48) & 0xff) &&
            write_byte(address + 7, (value >> 56) & 0xff);
    }

private:
    virtual std::optional<u64> read_byte(const u64 address) = 0;
    virtual bool write_byte(const u64 address, const u8 value) = 0;
};
