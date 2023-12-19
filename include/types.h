#pragma once
#include <cstdint>
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using i8 = int8_t;

inline u64 sign_extend_32(const u32 value)
{
    const i64 x = (i64)((i32)value);
    return (u64)x;
}

inline u64 sign_extend_16(const u16 value)
{
    const i64 x = (i64)((i16)value);
    return (u64)x;
}

inline u64 sign_extend_8(const u8 value)
{
    const i64 x = (i64)((i8)value);
    return (u64)x;
}
