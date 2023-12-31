#include "devices/ram.h"
#include <iostream>

RAM::RAM(const uint64_t size)
{
    memory = new u8[size];
    this->size = size;
}

void print_warning(const u64 address)
{
    std::cout << "warning: failed to access memory location " <<
        std::hex << (address) + 0x80000000 << std::endl;
}

std::optional<u64> RAM::read_byte(const u64 address)
{
    if (address >= size)
    {
        print_warning(address);
        return std::nullopt;
    }
    return memory[address];
}

bool RAM::write_byte(const u64 address, const u8 value)
{
    if (address >= size)
    {
        print_warning(address);
        return false;
    }
    memory[address] = value;
    return true;
}

RAM::~RAM()
{
    delete[] memory;
}
