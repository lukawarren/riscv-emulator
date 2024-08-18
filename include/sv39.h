#pragma once
#include "common.h"

/*
    Implements the decoding of the Sv39 paging model, supporting a 39-bit
    virtual address space.
*/

struct Address
{
    u64 address;

    Address(const u64 address) : address(address)
    {
        // TODO: load and store effective addresses, which are 64 bits, must have
        // bit 63–39 all equal to bit 38, or else a page-fault exception will occur
    }

    u64 get_page_offset() const
    {
        return address & 0b111111111111;
    }
};

struct VirtualAddress : Address
{
    VirtualAddress(const u64 address) : Address(address) {}

    std::array<u64, 3> get_vpns() const
    {
        return {
            (address >> 12) & 0x1ff,
            (address >> 21) & 0x1ff,
            (address >> 30) & 0x1ff,
        };
    }
};

struct PhysicalAddress : Address
{
    PhysicalAddress(const u64 address) : Address(address) {}

    std::array<u64, 3> get_ppns() const
    {
        return {
            (address >> 12) & 0x1ff,
            (address >> 21) & 0x1ff,
            (address >> 30) & 0x3ffffff,
        };
    }
};

struct PageTableEntry : Address
{
    PageTableEntry(const u64 address) : Address(address) {}

    u8 get_v() const { return (address >> 0) & 0b1; }
    u8 get_r() const { return (address >> 1) & 0b1; }
    u8 get_w() const { return (address >> 2) & 0b1; }
    u8 get_x() const { return (address >> 3) & 0b1; }
    u8 get_u() const { return (address >> 4) & 0b1; }
    u8 get_g() const { return (address >> 5) & 0b1; }
    u8 get_a() const { return (address >> 6) & 0b1; }
    u8 get_d() const { return (address >> 7) & 0b1; }

    void set_a() { address |= (1 << 6); }
    void set_d() { address |= (1 << 7); }

    u8 get_rsw() const { return (address >> 8) & 0b11; }

    std::array<u64, 3> get_ppns() const
    {
        return {
            (address >> 10) & 0x1ff,
            (address >> 19) & 0x1ff,
            (address >> 28) & 0x3ffffff,
        };
    }

    u64 get_ppn() const
    {
        return (address >> 10) & 0x0fffffffffff;
    }
};
