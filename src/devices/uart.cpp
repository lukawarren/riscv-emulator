#include "devices/uart.h"
#include <iostream>

#define TX_RX_REG   0
#define STATUS_REG  0

#define RXEMPTY_BIT 0
#define RXIEN_BIT   1
#define TXEMPTY_BIT 2
#define TXIEN_BIT   3

std::optional<u64> UART::read_byte(const u64 address)
{
    return { 0xff };
}

bool UART::write_byte(const u64 address, const u8 value)
{
    std::cout << value;
    return true;
}
