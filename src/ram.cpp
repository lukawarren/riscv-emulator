#include "ram.h"
#include <stdexcept>

RAM::RAM(const uint64_t size)
{
    memory = new u8[size];
    this->size = size;
}

u8 RAM::read_8(const u64 address)
{
    return read(address);
}

u16 RAM::read_16(const u64 address)
{
    return (read(address + 0) << 0) |
           (read(address + 1) << 8);
}

u32 RAM::read_32(const u64 address)
{
    return (read(address + 0) << 0)  |
           (read(address + 1) << 8)  |
           (read(address + 2) << 16) |
           (read(address + 3) << 24);
}

u64 RAM::read_64(const u64 address)
{
    return (read(address + 0) << 0)  |
           (read(address + 1) << 8)  |
           (read(address + 2) << 16) |
           (read(address + 3) << 24) |
           (read(address + 4) << 32) |
           (read(address + 5) << 40) |
           (read(address + 6) << 48) |
           (read(address + 7) << 56);
}

void RAM::write_8(const u64 address, const u8 value)
{
    write(address, value);
}

void RAM::write_16(const u64 address, const u16 value)
{
    write(address, value);
    write(address + 1, (value >> 8) & 0xff);
}

void RAM::write_32(const u64 address, const u32 value)
{
    write(address, value);
    write(address + 1, (value >> 8)  & 0xff);
    write(address + 2, (value >> 16) & 0xff);
    write(address + 3, (value >> 24) & 0xff);
}

void RAM::write_64(const u64 address, const u64 value)
{
    write(address, value);
    write(address + 1, (value >> 8)  & 0xff);
    write(address + 2, (value >> 16) & 0xff);
    write(address + 3, (value >> 24) & 0xff);
    write(address + 4, (value >> 32) & 0xff);
    write(address + 5, (value >> 40) & 0xff);
    write(address + 6, (value >> 48) & 0xff);
    write(address + 7, (value >> 56) & 0xff);
}

u64 RAM::read(const u64 address)
{
    if (address >= size)
    {
        throw std::runtime_error("invalid memory read");
        return 0;
    }
    return memory[address];
}

void RAM::write(const u64 address, const u8 value)
{
    if (address >= size)
    {
        throw std::runtime_error("invalid memory read");
        return;
    }
    memory[address] = value;
}

RAM::~RAM()
{
    delete[] memory;
}