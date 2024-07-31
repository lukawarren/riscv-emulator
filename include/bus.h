#pragma once
#include "common.h"
#include "devices/ram.h"
#include "devices/uart.h"
#include "devices/plic.h"
#include "devices/clint.h"
#include "devices/error_device.h"
#include "devices/virtio_block_device.h"

class CPU;

class Bus
{
// Needs access to raw RAM
friend VirtioBlockDevice;

public:
    Bus(
        const u64 ram_size,
        const std::optional<std::string> block_device_image,
        const bool is_test_mode
    );

    [[nodiscard]] std::optional<u8>  read_8 (const u64 address);
    [[nodiscard]] std::optional<u16> read_16(const u64 address);
    [[nodiscard]] std::optional<u32> read_32(const u64 address);
    [[nodiscard]] std::optional<u64> read_64(const u64 address);

    [[nodiscard]] bool write_8 (const u64 address, const u8  value);
    [[nodiscard]] bool write_16(const u64 address, const u16 value);
    [[nodiscard]] bool write_32(const u64 address, const u32 value);
    [[nodiscard]] bool write_64(const u64 address, const u64 value);

    void write_file(const u64 address, const std::string& filename);

    // Bus layout for emulator
    constexpr static u64 plic_base = 0xc000000;
    constexpr static u64 plic_end = plic_base + 0x3fff004;
    constexpr static u64 clint_base = 0x2000000;
    constexpr static u64 clint_end = 0x2010000;
    constexpr static u64 uart_address = 0x3000000;
    constexpr static u64 uart_length = 0x100;
    constexpr static u64 blk_address = 0x4000000;
    constexpr static u64 blk_length = 0x200;
    constexpr static u64 ram_base = 0x80000000;
    constexpr static u64 programs_base = 0x80000000;

    // For A extension
    std::unordered_set<u64> reservations = {};

    void clock(CPU& cpu);

private:
    std::pair<BusDevice&, u64> get_bus_device(const u64 address, const u64 size);
    RAM ram;
    UART uart;
    PLIC plic;
    CLINT clint;
    ErrorDevice error;
    VirtioBlockDevice block_device;
    u64 clock_counter = 0;
};
