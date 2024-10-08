#include "devices/plic.h"
#include "cpu.h"
#include "csrs.h"

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
#define MACHINE_CONTEXT                 0
#define SUPERVISOR_CONTEXT              1

u32* PLIC::get_register(const u64 address, const Mode mode)
{
    (void)mode;

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

    // Interrupt enable bits - each byte has bits for 8 interrupts
    // Store for context 0, context 1, ... context 15871
    if (address >= ENABLE_OFFSET && address < ENABLE_OFFSET + ENABLE_SIZE)
    {
        const u16 context = (address - ENABLE_OFFSET) / ENABLE_CONTEXT_SIZE;
        if (context > PLIC_SUPPORTED_CONTEXTS)
        {
            dbg("warning: unsupported context ", context);
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

        dbg("warning: unsupported context with address 0x", dbg::hex(address));
        return nullptr;
    }

    assert(false);
    return nullptr;
}

void PLIC::clock(CPU& cpu)
{
    // If enabled, clear and set back later if need be
    if (cpu.mie.mei()) cpu.mip.clear_mei();
    if (cpu.mie.sei()) cpu.mip.clear_sei();

    // Same for claimed
    set_interrupt_claimed(0, 0);

    for (int context = 0; context < PLIC_SUPPORTED_CONTEXTS; ++context)
    {
        const u64 enabled_offset = (context * PLIC_NUM_INTERRUPTS / 32);

        // Find any enabled interrupts - go first in groups of 32 for speed
        for (int i = 0; i < PLIC_SUPPORTED_INTERRUPTS / 32; ++i)
        {
            if (interrupt_pending[i] == 0 || interrupt_enable_bits[enabled_offset + i] == 0)
                continue;

            // At least 1 interrupt identified - ignore priority for now and just
            // find the first one (unlikely two interrupts occur at once!)
            // TODO: pick the pending update with the *highest* priority,
            //       not just the first one that pops up...

            const u32 threshold = context_priority_threshold[context];

            for (int j = 0; j < 32; j++)
            {
                const u16 id = i * 32 + j;
                if (get_interrupt_enabled(id, context) &&
                    get_interrupt_pending(id) &&
                    get_interrupt_priority(id) >= threshold)
                {
                    // Found enabled, pending interrupt with sufficient priority
                    // Update claim and set external interrupt pending flag!
                    set_interrupt_claimed(id, context);

                    // NOTE: I am unsure if it should be set_sei() or set_mei().
                    //       Linux will fail to correctly detect interrupts if
                    //       it's mei, however, and a few other sources seem to
                    //       use sei too. To me that doesn't make sense because
                    //       surely one should check if context == MACHINE_CONTEXT
                    //       (and raise mei if it is) but hey-ho.
                    cpu.mip.set_sei();

                    return;
                }
            }
        }
    }
}

u32 PLIC::get_interrupt_priority(const u16 interrupt)
{
    assert(interrupt < PLIC_SUPPORTED_INTERRUPTS);
    return interrupt_priority[interrupt];
}

bool PLIC::get_interrupt_pending(const u16 interrupt)
{
    assert(interrupt < PLIC_SUPPORTED_INTERRUPTS);
    const u16 slot = interrupt / 32;
    const u16 bit = interrupt % 32;
    const u32 value = (interrupt_pending[slot] >> bit) & 0b1;
    return (value == 1) ? true : false;
}

void PLIC::set_interrupt_pending(const u16 interrupt)
{
    assert(interrupt < PLIC_SUPPORTED_INTERRUPTS);
    const u16 slot = interrupt / 32;
    const u16 bit = interrupt % 32;
    interrupt_pending[slot] |= (1 << bit);
}

void PLIC::clear_interrupt_pending(const u16 interrupt)
{
    assert(interrupt < PLIC_SUPPORTED_INTERRUPTS);
    const u16 slot = interrupt / 32;
    const u16 bit = interrupt % 32;
    interrupt_pending[slot] &= ~(1 << bit);
}

bool PLIC::get_interrupt_enabled(const u16 interrupt, const u16 context)
{
    assert(interrupt < PLIC_SUPPORTED_INTERRUPTS);
    const u16 slot = interrupt / 32 + (context * PLIC_NUM_INTERRUPTS / 32);
    const u16 bit = interrupt % 32;
    const u32 value = (interrupt_enable_bits[slot] >> bit) & 0b1;
    return (value == 1) ? true : false;
}

void PLIC::set_interrupt_enabled(const u16 interrupt, const u16 context)
{
    assert(interrupt < PLIC_SUPPORTED_INTERRUPTS);
    const u16 slot = interrupt / 32 + (context * PLIC_NUM_INTERRUPTS / 32);
    const u16 bit = interrupt % 32;
    interrupt_enable_bits[slot] |= (1 << bit);
}

bool PLIC::get_interrupt_claimed(const u16 interrupt, const u16 context)
{
    return context_claim[context] == interrupt;
}

void PLIC::set_interrupt_claimed(const u16 interrupt, const u16 context)
{
    context_claim[context] = interrupt;
}
