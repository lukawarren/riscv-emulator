#include "bus.h"
#include "cpu.h"
#include "io.h"

Bus::Bus(
    const u64 ram_size,
    const std::optional<std::string> block_device_image,
    const bool is_test_mode
) : ram(ram_size), uart(!is_test_mode), block_device(block_device_image),
    is_test_mode(is_test_mode) {}

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

size_t Bus::write_file(const u64 address, const std::string& filename)
{
    std::pair<u8*, size_t> file = io_read_file(filename);

    for (u64 i = 0; i < file.second; ++i)
        assert(write_8(address + i, file.first[i]));

    delete[] file.first;
    return file.second;
}

void Bus::clock(CPU& cpu, bool is_jit)
{
    clint.increment(cpu);

    // The CLINT is pretty sensitive to not being called every cycle (Linux
    // will hang), but UART and the PLIC don't need to be called every clock
    // cycle to work, and they're actually fairly costly. Only calling them
    // every so often shaves about 1 second of Linux's (currently) 13 second
    // boot time.

    if (((++clock_counter) % 1024) == 0 || is_jit)
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

    // riscv-tests purposefully reads invalid addresses
    if (!is_test_mode)
    {
        throw std::runtime_error(std::format(
            "attempt to read unmapped memory address {:0x}",
            address
        ));
    }
    return { error, 0 };
}
