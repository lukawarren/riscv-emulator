#include "devices/plic.h"
#include <cassert>
#include <iostream>

#define PRIORITY_OFFSET     0x0
#define PENDING_OFFSET      0x1000
#define PENDING_SIZE        (PLIC_NUM_INTERRUPTS / 8)
#define ENABLE_OFFSET       0x2000
#define ENABLE_CONTEXT_SIZE (PLIC_NUM_INTERRUPTS / 8)
#define ENABLE_SIZE         (ENABLE_CONTEXT_SIZE * PLIC_NUM_CONTEXTS)
#define CONTEXT_OFFSET      0x1ffffc
#define ADDRESS_RANGE       0x4000000

#define CONTEXT_0_PRIORITY_THRESHOLD    0x200000
#define CONTEXT_0_CLAIM                 0x200004
#define CONTEXT_1_PRIORITY_THRESHOLD    0x201000
#define CONTEXT_1_CLAIM                 0x201004

u64 round_address_to_nearest_word(const u64 address)
{
    return address - (address % 4);
}

std::optional<u64> PLIC::read_byte(const u64 address)
{
    const u32* reg = get_register(round_address_to_nearest_word(address));
    if (reg == nullptr) return { 0 };

    switch (address % 4)
    {
        case 0: return { (*reg >>  0) & 0xff };
        case 1: return { (*reg >>  8) & 0xff };
        case 2: return { (*reg >> 16) & 0xff };
        case 3: return { (*reg >> 24) & 0xff };
    }

    return { 0 };
}

bool PLIC::write_byte(const u64 address, const u8 value)
{
    u32* reg = get_register(round_address_to_nearest_word(address));
    if (reg == nullptr) return true;

    switch (address % 4)
    {
        case 0: *reg = (*reg & 0xffffff00) | ((u32)value <<  0); break;
        case 1: *reg = (*reg & 0xffff00ff) | ((u32)value <<  8); break;
        case 2: *reg = (*reg & 0xff00ffff) | ((u32)value << 16); break;
        case 3: *reg = (*reg & 0x00ffffff) | ((u32)value << 24); break;
    }

    return true;
}

u32* PLIC::get_register(const u64 address)
{
    // Interrupt priority
    if (address < PENDING_OFFSET)
    {
        return &interrupt_priority[address / sizeof(u32)];
    }

    // Interrupt pending - each byte holds pending for 8 interrupts
    if (address >= PENDING_OFFSET && address < PENDING_OFFSET + PENDING_SIZE)
    {
        return &interrupt_pending[(address - PENDING_OFFSET)];
    }

    // Interrupt enable bits - each byte bits for 8 interrupts
    // Store for context 0, context 1, ... context 15871
    if (address >= ENABLE_OFFSET && address < ENABLE_OFFSET + ENABLE_SIZE)
    {
        const u16 context = (address - ENABLE_OFFSET) / ENABLE_CONTEXT_SIZE;
        if (context > PLIC_SUPPORTED_CONTEXTS)
        {
            std::cout << "warning: unsupported context " << context << std::endl;
            return nullptr;
        }

        return &interrupt_enable_bits[address - ENABLE_OFFSET];
    }

    // Misc. context stuff
    if (address >= CONTEXT_OFFSET && address < ADDRESS_RANGE)
    {
        if (address == CONTEXT_0_PRIORITY_THRESHOLD)
            return &context_priority_threshold[0];
        if (address == CONTEXT_1_PRIORITY_THRESHOLD)
            return &context_priority_threshold[1];

        if (address == CONTEXT_0_CLAIM)
            return &context_claim[0];
        if (address == CONTEXT_1_CLAIM)
            return &context_claim[1];

        std::cout << "warning: unsupported context with address 0x" << std::hex << address << std::endl;
        return nullptr;
    }

    assert(false);
    return nullptr;
}

u32 PLIC::get_interrupt_priority(const u16 interrupt)
{
    assert(interrupt < PLIC_NUM_INTERRUPTS);
    return interrupt_priority[interrupt];
}

bool PLIC::get_interrupt_pending(const u16 interrupt)
{
    assert(interrupt < PLIC_NUM_INTERRUPTS);
    const u16 slot = interrupt / 32;
    const u16 bit = interrupt % 32;
    const u32 value = (interrupt_pending[slot] >> bit) & 0b1;
    return (value == 1) ? true : false;
}

void PLIC::set_interrupt_pending(const u16 interrupt)
{
    assert(interrupt < PLIC_NUM_INTERRUPTS);
    const u16 slot = interrupt / 32;
    const u16 bit = interrupt % 32;
    interrupt_pending[slot] |= (1 << bit);
}

bool PLIC::get_interrupt_enabled(const u16 interrupt)
{
    assert(interrupt < PLIC_NUM_INTERRUPTS);
    const u16 slot = interrupt / 32;
    const u16 bit = interrupt % 32;
    const u32 value = (interrupt_enable_bits[slot] >> bit) & 0b1;
    return (value == 1) ? true : false;
}

void PLIC::set_interrupt_enabled(const u16 interrupt)
{
    assert(interrupt < PLIC_NUM_INTERRUPTS);
    const u16 slot = interrupt / 32;
    const u16 bit = interrupt % 32;
    interrupt_enable_bits[slot] |= (1 << bit);
}
