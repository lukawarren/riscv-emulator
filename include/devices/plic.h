#pragma once
#include "devices/bus_device.h"
#include <iostream>

// We support two contexts - HART 0 supervisor and HART 0 machine mode
#define PLIC_NUM_INTERRUPTS      1024
#define PLIC_NUM_CONTEXTS        15872
#define PLIC_SUPPORTED_CONTEXTS  2

class PLIC : public BusDevice
{
public:
    std::optional<u64> read_byte(const u64 address) override;
    bool write_byte(const u64 address, const u8 value) override;

private:
    u32* get_register(const u64 address);

    // Priority: individual register for each interrupt
    // Pending: singular bit per register (i.e. 32 per word)
    u32 interrupt_priority[PLIC_NUM_INTERRUPTS] = {};
    u32 interrupt_pending[PLIC_NUM_INTERRUPTS / 8] = {};

    // We support two contexts - HART 0 supervisor and HART 0 machine mode
    u32 interrupt_enable_bits[PLIC_NUM_INTERRUPTS / 8 * PLIC_SUPPORTED_CONTEXTS] = {};

    // Contexts misc.
    u32 context_priority_threshold[PLIC_SUPPORTED_CONTEXTS] = {};
    u32 context_claim[PLIC_SUPPORTED_CONTEXTS] = {};
};
