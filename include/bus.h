#pragma once
#include "types.h"
#include "ram.h"
#include <string>

class Bus
{
public:
    Bus(const uint64_t ram_size);

    u8  read_8 (const u64 address);
    u16 read_16(const u64 address);
    u32 read_32(const u64 address);
    u64 read_64(const u64 address);

    void write_8 (const u64 address, const u8  value);
    void write_16(const u64 address, const u16 value);
    void write_32(const u64 address, const u32 value);
    void write_64(const u64 address, const u64 value);

    void write_file(const u64 address, const std::string& filename);

    /*
        Bus layout for emulator:
        - RAM starts at 0x80000000
    */
    constexpr static u64 ram_base = 0x80000000;

private:
    RAM ram;
};
