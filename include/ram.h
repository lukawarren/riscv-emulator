#pragma once
#include "types.h"
#include <optional>

class RAM
{
public:
    RAM(const uint64_t size);
    ~RAM();

    std::optional<u8>  read_8 (const u64 address);
    std::optional<u16> read_16(const u64 address);
    std::optional<u32> read_32(const u64 address);
    std::optional<u64> read_64(const u64 address);

    [[nodiscard]] bool write_8 (const u64 address, const u8  value);
    [[nodiscard]] bool write_16(const u64 address, const u16 value);
    [[nodiscard]] bool write_32(const u64 address, const u32 value);
    [[nodiscard]] bool write_64(const u64 address, const u64 value);

private:
    std::optional<u64> read(const u64 address);
    bool write(const u64 address, const u8 value);

    u8* memory;
    uint64_t size;
};
