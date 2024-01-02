#include "devices/ram.h"
#include <iostream>
#include <cstring>

RAM::RAM(const uint64_t size)
{
    memory = new u8[size];
    this->size = size;
}

std::optional<u64> RAM::read_byte(const u64 address)
{
    return memory[address];
}

bool RAM::write_byte(const u64 address, const u8 value)
{
    memory[address] = value;
    return true;
}

template<typename T>
T RAM::read_fast_path(const u64 address)
{
    T ret;
    memcpy(&ret, memory + address, sizeof(T));
    return ret;
}

template<typename T>
void RAM::write_fast_path(const u64 address, const T value)
{
    memcpy(memory + address, &value, sizeof(T));
}

std::optional<u16> RAM::read_16(const u64 address) { return read_fast_path<u16>(address); }
std::optional<u32> RAM::read_32(const u64 address) { return read_fast_path<u32>(address); }
std::optional<u64> RAM::read_64(const u64 address) { return read_fast_path<u64>(address); }

bool RAM::write_16(const u64 address, const u16 value) { write_fast_path<u16>(address, value); return true; }
bool RAM::write_32(const u64 address, const u32 value) { write_fast_path<u32>(address, value); return true; }
bool RAM::write_64(const u64 address, const u64 value) { write_fast_path<u64>(address, value); return true; }

RAM::~RAM()
{
    delete[] memory;
}
