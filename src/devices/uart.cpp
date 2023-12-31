#include "devices/uart.h"
#include <iostream>
#include <cassert>

#define TX_RX_REG   0
#define STATUS_REG  1

#define RXEMPTY_BIT         0
#define RX_INTERRUPT_BIT    1
#define TXEMPTY_BIT         2
#define TX_INTERRUPT_BIT    3

std::optional<u64> UART::read_byte(const u64 address)
{
    if (address == TX_RX_REG)
    {
        if (rx_buffer.size() > 0)
        {
            const u8 value = rx_buffer.front();
            rx_buffer.pop();
            return value;
        }
        return  0;
    }
    else if (address == STATUS_REG)
    {
        u8 value = 0;
        value |= rx_buffer.empty() << RXEMPTY_BIT;
        value |= tx_buffer.empty() << TXEMPTY_BIT;
        value |= (rx_irq_enabled & 1) << RX_INTERRUPT_BIT;
        value |= (tx_irq_enabled & 1) << TX_INTERRUPT_BIT;
        return { value };
    }
    else
    {
        std::cout << "unmapped UART address " << std::hex << address << std::endl;
        assert(false);
    }

    return { 0 };
}

bool UART::write_byte(const u64 address, const u8 value)
{
    if (address == TX_RX_REG)
    {
        tx_buffer.push(value);
    }
    else if (address == STATUS_REG)
    {
        // Status register
        rx_irq_enabled = (value >> RX_INTERRUPT_BIT) & 1;
        tx_irq_enabled = (value >> TX_INTERRUPT_BIT) & 1;
    }
    else
    {
        std::cout << "unmapped UART address " << std::hex << address << std::endl;
        assert(false);
    }

    return true;
}

void UART::clock(PLIC& plic)
{
    bool should_trigger_irq = false;

    if (tx_buffer.size() > max_buffers_size - 1)
    {
        // Print out all characters in "transmitted buffer",
        // and transfer them to the "received buffer"
        while (tx_buffer.size() > 0)
        {
            const u8 character = tx_buffer.front();
            std::cout << character;
            tx_buffer.pop();
        }
        fflush(stdout);
    }

    if (rx_irq_enabled && rx_buffer.size() >= max_buffers_size)
        should_trigger_irq = true;

    if (tx_irq_enabled && tx_buffer.size() == 0)
        should_trigger_irq = true;

    if (should_trigger_irq)
        plic.set_interrupt_pending(PLIC_INTERRUPT_UART);
    else
        plic.clear_interrupt_pending(PLIC_INTERRUPT_UART);
}
