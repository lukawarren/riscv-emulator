#include "ram.h"
#include <cassert>

RAM::RAM(const uint64_t size)
{
    memory = new u8[size];
    this->size = size;
}

std::optional<u64> RAM::read_byte(const u64 address)
{
    if (address >= size) return std::nullopt;
    return memory[address];
}

bool RAM::write_byte(const u64 address, const u8 value)
{
    if (address >= size) return false;
    memory[address] = value;
    return true;
}

RAM::~RAM()
{
    delete[] memory;
}