#pragma once
#include "types.h"
#include "devices/ram.h"
#include "devices/uart.h"
#include "devices/plic.h"
#include "devices/clint.h"
#include <string>
#include <unordered_set>

class CPU;

class Bus
{
public:
    Bus(const u64 ram_size);

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
    constexpr static u64 plic_end = 0xc200000;
    constexpr static u64 clint_base = 0x2000000;
    constexpr static u64 clint_end = 0x2010000;
    constexpr static u64 uart_address_one = 0x3000000;
    constexpr static u64 uart_address_two = 0x3000001;
    constexpr static u64 ram_base = 0x80000000;
    constexpr static u64 programs_base = 0x80000000;

    // For A extension
    std::unordered_set<u64> reservations = {};

    void clock(CPU& cpu);

private:
    std::pair<BusDevice&, u64> get_bus_device(const u64 address);
    RAM ram;
    UART uart;
    PLIC plic;
    CLINT clint;
};
