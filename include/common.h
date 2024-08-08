#pragma once
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <format>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <limits>
#include <string>
#include <unordered_set>
#include <expected>
#include <optional>
#include <array>
#include <queue>
#include <thread>
#include <mutex>
#include <functional>

// For UART stdin
#include <termios.h>
#include <unistd.h>

// For virtio endianness check
#include <bit>

// For nice formatting
#define DBG_MACRO_NO_WARNING
#include <dbg.h>

// For RV64FD
#include <cfenv>
#include <cmath>

// For virtio mmap
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern "C" {
    #include <riscv-disas.h>
}

__extension__ using u128 =  unsigned __int128;
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
__extension__ using i128 = __int128;
using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using i8 = int8_t;

using size_t = std::size_t;
