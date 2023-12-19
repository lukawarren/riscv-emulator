#pragma once
#include "types.h"

class RAM
{
public:
    RAM(const uint64_t size);
    ~RAM();

    u8  read_8 (const u64 address);
    u16 read_16(const u64 address);
    u32 read_32(const u64 address);
    u64 read_64(const u64 address);

    void write_8 (const u64 address, const u8  value);
    void write_16(const u64 address, const u16 value);
    void write_32(const u64 address, const u32 value);
    void write_64(const u64 address, const u64 value);

private:
    u64 read(const u64 address);
    void write(const u64 address, const u8 value);

    u8* memory;
    uint64_t size;
};
