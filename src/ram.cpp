#include "ram.h"
#include <array>

RAM::RAM(const uint64_t size)
{
    memory = new u8[size];
    this->size = size;
}

std::optional<u8> RAM::read_8(const u64 address)
{
    return read(address);
}

std::optional<u16> RAM::read_16(const u64 address)
{
    std::array<std::optional<u64>, 2> values =
    {
        read(address + 0),
        read(address + 1)
    };

    for (const auto& opt : values)
        if (!opt.has_value())
            return std::nullopt;

    return (*values[0] << 0) |
           (*values[1] << 8);
}

std::optional<u32> RAM::read_32(const u64 address)
{
    std::array<std::optional<u64>, 4> values =
    {
        read(address + 0),
        read(address + 1),
        read(address + 2),
        read(address + 3)
    };

    for (const auto& opt : values)
        if (!opt.has_value())
            return std::nullopt;

    return (*values[0] << 0)  |
           (*values[1] << 8)  |
           (*values[2] << 16) |
           (*values[3] << 24);
}

std::optional<u64> RAM::read_64(const u64 address)
{
    std::array<std::optional<u64>, 8> values =
    {
        read(address + 0),
        read(address + 1),
        read(address + 2),
        read(address + 3),
        read(address + 4),
        read(address + 5),
        read(address + 6),
        read(address + 7)
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

bool RAM::write_8(const u64 address, const u8 value)
{
    return write(address, value);
}

bool RAM::write_16(const u64 address, const u16 value)
{
    return
        write(address, value) &&
        write(address + 1, (value >> 8) & 0xff);
}

bool RAM::write_32(const u64 address, const u32 value)
{
    return
        write(address, value) &&
        write(address + 1, (value >> 8)  & 0xff) &&
        write(address + 2, (value >> 16) & 0xff) &&
        write(address + 3, (value >> 24) & 0xff);
}

bool RAM::write_64(const u64 address, const u64 value)
{
    return
        write(address, value) &&
        write(address + 1, (value >> 8)  & 0xff) &&
        write(address + 2, (value >> 16) & 0xff) &&
        write(address + 3, (value >> 24) & 0xff) &&
        write(address + 4, (value >> 32) & 0xff) &&
        write(address + 5, (value >> 40) & 0xff) &&
        write(address + 6, (value >> 48) & 0xff) &&
        write(address + 7, (value >> 56) & 0xff);
}

std::optional<u64> RAM::read(const u64 address)
{
    if (address >= size)
        return std::nullopt;

    return { memory[address] };
}

bool RAM::write(const u64 address, const u8 value)
{
    if (address >= size)
        return false;

    memory[address] = value;
    return true;
}

RAM::~RAM()
{
    delete[] memory;
}