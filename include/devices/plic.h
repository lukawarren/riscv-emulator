#pragma once
#include "devices/bus_device.h"

class CPU;

// We support two contexts - HART 0 supervisor and HART 0 machine mode
#define PLIC_NUM_INTERRUPTS         1024
#define PLIC_NUM_CONTEXTS           15872
#define PLIC_SUPPORTED_CONTEXTS     2
#define PLIC_SUPPORTED_INTERRUPTS   32

#define PLIC_INTERRUPT_UART     10

class PLIC : public BusDevice
{
public:
    std::optional<u64> read_byte(const u64 address) override;
    bool write_byte(const u64 address, const u8 value) override;
    void clock(CPU& cpu);

    u32  get_interrupt_priority (const u16 interrupt);
    bool get_interrupt_pending  (const u16 interrupt);
    void set_interrupt_pending  (const u16 interrupt);
    void clear_interrupt_pending(const u16 interrupt);
    bool get_interrupt_enabled  (const u16 interrupt, const u16 context);
    void set_interrupt_enabled  (const u16 interrupt, const u16 context);
    bool get_interrupt_claimed  (const u16 interrupt, const u16 context);
    void set_interrupt_claimed  (const u16 interrupt, const u16 context);
    void clear_interrupt_claimed(const u16 interrupt, const u16 context);

private:
    // Priority: individual register for each interrupt
    // Pending: singular bit per register (i.e. 32 per word)
    u32 interrupt_priority[PLIC_NUM_INTERRUPTS] = {};
    u32 interrupt_pending[PLIC_NUM_INTERRUPTS / 32] = {};

    // We support two contexts - HART 0 supervisor and HART 0 machine mode
    u32 interrupt_enable_bits[PLIC_NUM_INTERRUPTS / 32 * PLIC_SUPPORTED_CONTEXTS] = {};

    // Contexts misc.
    u32 context_priority_threshold[PLIC_SUPPORTED_CONTEXTS] = {};
    u32 context_claim[PLIC_SUPPORTED_CONTEXTS] = {};

    u32* get_register(const u64 address);
};
