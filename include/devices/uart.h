#pragma once
#include "devices/bus_device.h"
#include "devices/plic.h"

class UART : public BusDevice
{
public:
    UART(bool listen_for_input = true);
    ~UART();

    std::optional<u64> read_byte(const u64 address) override;
    bool write_byte(const u64 address, const u8 value) override;
    void clock(PLIC& plic);

private:
    // ns16550a state
    u8 ier;
    u8 lcr;
    u8 dll;
    u8 dlm;
    u8 mcr;
    u8 current_interrupt;
    u8 pending_interrupts;

    // Input
    bool listening_to_input;
    std::thread input_thread;
    std::queue<char> input_buffer;
    std::mutex input_mutex;
    struct termios original_termios;
    static void input_thread_run(UART& uart);
    static int read_character();
};
