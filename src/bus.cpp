#include "bus.h"
#include "cpu.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <format>

Bus::Bus(const u64 ram_size) : ram(ram_size) {}

#define READ_X(x) std::optional<u##x> Bus::read_##x(const u64 address)\
{\
    std::pair<BusDevice&, u64> device_info = get_bus_device(address);\
    return device_info.first.read_##x(address - device_info.second);\
}

READ_X(8)
READ_X(16)
READ_X(32)
READ_X(64)

#define WRITE_X(x) bool Bus::write_##x(const u64 address, const u##x value)\
{\
    std::pair<BusDevice&, u64> device_info = get_bus_device(address);\
    return device_info.first.write_##x(address - device_info.second, value);\
}

WRITE_X(8 )
WRITE_X(16)
WRITE_X(32)
WRITE_X(64)

void Bus::write_file(const u64 address, const std::string& filename)
{
    // Check isn't folder
    if (!std::filesystem::is_regular_file(filename))
    {
        throw std::runtime_error(filename + " is not a file");
        return;
    }

    // Open file
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("unable to open file" + filename);
        return;
    }

    // Get file length
    file.seekg(0, std::ios::end);
    u64 fileLen = file.tellg();
    file.seekg(0, std::ios::beg);

    // Allocate memory
    u8* buffer = new u8[fileLen + 1];
    if (!buffer)
    {
        throw std::runtime_error("unable to load entire file into memory");
        file.close();
        return;
    }

    // Read file contents into buffer
    file.read(reinterpret_cast<char*>(buffer), fileLen);
    file.close();

    // TODO: memcpy fast-path
    for (u64 i = 0; i < fileLen; ++i)
        std::ignore = write_8(address + i, buffer[i]);
}

void Bus::clock(CPU& cpu)
{
    clint.increment(cpu);
}

std::pair<BusDevice&, u64> Bus::get_bus_device(const u64 address)
{
    if (address == uart_address_one || address == uart_address_two)
        return { uart, uart_address_one };

    if (address >= plic_base && address <= plic_end)
        return { plic, plic_base };

    if (address >= clint_base && address <= clint_end)
        return { clint, clint_base };

    if (address < ram_base)
        throw std::runtime_error(std::format(
            "attempt to read unmapped memory address 0x{:0x}",
            address
        ));

    return { ram, ram_base };
}
