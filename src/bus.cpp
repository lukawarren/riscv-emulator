#include "bus.h"
#include <fstream>
#include <filesystem>
#include <iostream>

Bus::Bus(const uint64_t ram_size) : ram(ram_size) {}

u64 get_ram_address(const u64 address)
{
    if (address < Bus::ram_base)
    {
        throw std::runtime_error("unmapped memory location");
        return 0;
    }

    return address - Bus::ram_base;
}

std::optional<u8> Bus::read_8(const u64 address) { return ram.read_8(get_ram_address(address)); }
std::optional<u16>Bus::read_16(const u64 address) { return ram.read_16(get_ram_address(address)); }
std::optional<u32>Bus::read_32(const u64 address) { return ram.read_32(get_ram_address(address)); }
std::optional<u64>Bus::read_64(const u64 address) { return ram.read_64(get_ram_address(address)); }

[[nodiscard]] bool Bus::write_8(const u64 address, const u8 value) { return ram.write_8(get_ram_address(address), value); }
[[nodiscard]] bool Bus::write_16(const u64 address, const u16 value) { return ram.write_16(get_ram_address(address), value); }
[[nodiscard]] bool Bus::write_32(const u64 address, const u32 value) { return ram.write_32(get_ram_address(address), value); }
[[nodiscard]] bool Bus::write_64(const u64 address, const u64 value) { return ram.write_64(get_ram_address(address), value); }

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