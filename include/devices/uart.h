#pragma once
#include "devices/bus_device.h"
#include "devices/plic.h"
#include <queue>

class UART : public BusDevice
{
public:
    std::optional<u64> read_byte(const u64 address) override;
    bool write_byte(const u64 address, const u8 value) override;
    void clock(PLIC& plic);

private:
    u8 rx_triggered = 0;
    u8 rx_irq_enabled = 0;
    u8 tx_triggered = 0;
    u8 tx_irq_enabled = 0;

    constexpr static size_t max_buffers_size = 1;
    std::queue<u8> rx_buffer = {};
    std::queue<u8> tx_buffer = {};
};
