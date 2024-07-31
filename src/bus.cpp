#include "bus.h"
#include "cpu.h"

Bus::Bus(const u64 ram_size, const bool is_test_mode) :
    ram(ram_size),
    uart(!is_test_mode)
{}

#define READ_X(x) std::optional<u##x> Bus::read_##x(const u64 address)\
{\
    std::pair<BusDevice&, u64> device_info = get_bus_device(address, x / 8);\
    return device_info.first.read_##x(address - device_info.second);\
}

READ_X(8)
READ_X(16)
READ_X(32)
READ_X(64)

#define WRITE_X(x) bool Bus::write_##x(const u64 address, const u##x value)\
{\
    std::pair<BusDevice&, u64> device_info = get_bus_device(address, x / 8);\
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

    for (u64 i = 0; i < fileLen; ++i)
        std::ignore = write_8(address + i, buffer[i]);
}

void Bus::clock(CPU& cpu)
{
    clint.increment(cpu);

    // The CLINT is pretty sensitive to not being called every cycle (Linux
    // will hang), but UART and the PLIC don't need to be called every clock
    // cycle to work, and they're actually fairly costly. Only calling them
    // every so often shaves about 1 second of Linux's (currently) 13 second
    // boot time.

    if (((++clock_counter) % 1024) == 0)
    {
        uart.clock(plic);
        block_device.clock(cpu, plic);
        plic.clock(cpu);
    }
}

std::pair<BusDevice&, u64> Bus::get_bus_device(const u64 address, const u64 size)
{
    // Check RAM first as is by far the most common
    // We include the size (in bytes) of the fetch to make sure addresses that
    // eclipse the end of RAM do not succeed. This edge case doesn't need to
    // be checked for other devices as they do their own checks.
    if (address >= ram_base && address + (size-1) < ram_base + ram.size) [[likely]]
        return { ram, ram_base };

    if (address >= blk_address && address < blk_address + blk_length)
        return { block_device, blk_address };

    if (address >= uart_address && address < uart_address + uart_length)
        return { uart, uart_address };

    if (address >= plic_base && address <= plic_end)
        return { plic, plic_base };

    if (address >= clint_base && address <= clint_end)
        return { clint, clint_base };

    dbg("attempt to read unmapped memory address ", dbg::hex(address));
    return { error, 0 };
}
