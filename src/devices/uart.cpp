#include "devices/uart.h"

#define RBR_OFFSET         0 // In:  Recieve Buffer Register
#define THR_OFFSET         0 // Out: Transmitter Holding Register
#define DLL_OFFSET         0 // Out: Divisor Latch Low
#define IER_OFFSET         1 // I/O: Interrupt Enable Register
#define DLM_OFFSET         1 // Out: Divisor Latch High
#define FCR_OFFSET         2 // Out: FIFO Control Register
#define IIR_OFFSET         2 // I/O: Interrupt Identification Register
#define LCR_OFFSET         3 // Out: Line Control Register
#define MCR_OFFSET         4 // Out: Modem Control Register
#define LSR_OFFSET         5 // In:  Line Status Register
#define MSR_OFFSET         6 // In:  Modem Status Register
#define SCR_OFFSET         7 // I/O: Scratch Register
#define MDR1_OFFSET        8 // I/O: Mode Register

#define LSR_FIFOE          0x80 // Fifo error
#define LSR_TEMT           0x40 // Transmitter empty
#define LSR_THRE           0x20 // Transmit-hold-register empty
#define LSR_BI             0x10 // Break interrupt indicator
#define LSR_FE             0x08 // Frame error indicator
#define LSR_PE             0x04 // Parity error indicator
#define LSR_OE             0x02 // Overrun error indicator
#define LSR_DR             0x01 // Receiver data ready
#define LSR_BRK_ERROR_BITS 0x1E // BI, FE, PE, OE bits

#define INT_THRE           1

constexpr size_t max_input_buffer_size = 10;

UART::UART(bool listen_for_input) : listening_to_input(listen_for_input)
{
    if (listen_for_input)
    {
        if (tcgetattr(0, &original_termios) < 0)
            throw std::runtime_error("failed to get terminal settings");

        input_thread = std::thread(input_thread_run, std::ref(*this));
    }
}

UART::~UART()
{
    if (listening_to_input)
    {
        pthread_cancel(input_thread.native_handle());
        input_thread.join();
        tcsetattr(0, TCSADRAIN, &original_termios);
    }
}

std::optional<u64> UART::read_byte(const u64 address)
{
    const bool dlab = (lcr & (1 << 7)) != 0;

    if (!dlab)
    {
        switch (address)
        {
            case RBR_OFFSET:
            {
                // Contains the byte received if no FIFO is used,
                // else the oldest unread byte
                input_mutex.lock();
                if (input_buffer.empty())
                {
                    input_mutex.unlock();
                    return 0;
                }
                else
                {
                    char c = input_buffer.front();
                    input_buffer.pop();
                    input_mutex.unlock();
                    return c;
                }
            }
            case IER_OFFSET:
            {
                return ier;
            }
            case IIR_OFFSET:
            {
                u8 value = (current_interrupt << 1) | (pending_interrupts ? 0 : 1);
                if (current_interrupt == INT_THRE)
                    pending_interrupts &= ~(1 << current_interrupt);
                return value;
            }
            case LCR_OFFSET: return lcr;
            case MCR_OFFSET: return mcr;
            case LSR_OFFSET:
            {
                // Bit 0 = DR (i.e. data available to be read)
                // Bit 5 = THR empty (i.e. we're ready to receive data)
                // Bit 6 = THR empty *and* line is idle
                input_mutex.lock();
                u64 ret = LSR_THRE | LSR_TEMT | (!input_buffer.empty());
                input_mutex.unlock();
                return ret;
            }
            case MSR_OFFSET:
            {
                // - carrier detect
                // - no ring
                // - data ready
                // - clear to send
                return 0xb0;
            }
            case SCR_OFFSET:
            {
                // Plain 8250, so we don't have a scratch register
                return 0;
            }
            default: break;
        }
    }
    else
    {
        switch (address)
        {
            case DLL_OFFSET: return dll;
            case DLM_OFFSET: return dlm;
            case IIR_OFFSET:
            {
                // As above
                u8 value = (current_interrupt << 1) | (pending_interrupts ? 0 : 1);
                if (current_interrupt == INT_THRE)
                    pending_interrupts &= ~(1 << current_interrupt);
                return value;
            }
            case LCR_OFFSET: return lcr;
            case MCR_OFFSET: return mcr;
            case LSR_OFFSET:
            {
                // As above
                return LSR_THRE | LSR_TEMT;
            }
            case MSR_OFFSET:
            {
                // As above
                return 0xb0;
            }
            case SCR_OFFSET:
            {
                // As above
                return 0;
            }
            default: break;
        }
    }

    throw std::runtime_error(std::format(
        "unknown UART read with base 0x{:x}, dlab = {}",
        address,
        dlab
    ));
    return std::nullopt;
}

bool UART::write_byte(const u64 address, const u8 value)
{
    const bool dlab = (lcr & (1 << 7)) != 0;

    if (!dlab)
    {
        switch (address)
        {
            case THR_OFFSET:
            {
                // Used to hold data to be transmitted (barring FIFO)
                std::cout << (char)value << std::flush;
                pending_interrupts |= 1 << INT_THRE;
                return true;
            }

            case IER_OFFSET: ier = value; return true;

            // We don't care if FIFO is enabled or disabled
            case FCR_OFFSET: return true;

            case LCR_OFFSET: lcr = value; return true;
            case MCR_OFFSET: mcr = value; return true;

            // Plain 8250, so we don't have a scratch register
            case SCR_OFFSET: return true;
            default: break;
        }
    }
    else
    {
        switch (address)
        {
            // Attempts to set the divisor (frequency)
            case DLL_OFFSET: dll = value; return true;
            case DLM_OFFSET: dlm = value; return true;

            // As above
            case FCR_OFFSET: return true;

            case LCR_OFFSET: lcr = value; return true;
            case MCR_OFFSET: mcr = value; return true;

            // As above
            case SCR_OFFSET: return true;
            default: break;
        }
    }

    throw std::runtime_error(std::format(
        "unknown UART write with base 0x{:x}, dlab = {}",
        address,
        dlab
    ));
    return false;
}

void UART::clock(PLIC& plic)
{
    // Input
    input_mutex.lock();
    if (!input_buffer.empty()) pending_interrupts |= 1;
    else pending_interrupts &= ~1;
    input_mutex.unlock();

    // Don't generate any disabled interrupts
    pending_interrupts &= ier;

    if (pending_interrupts)
    {
        current_interrupt = 31 - __builtin_clz(pending_interrupts | 1);
        plic.set_interrupt_pending(PLIC_INTERRUPT_UART);
    }
    else
        plic.clear_interrupt_pending(PLIC_INTERRUPT_UART);
}

void UART::input_thread_run(UART& uart)
{
    while(true)
    {
        // Block and wait for input
        int character = read_character();
        if (character != -1 && character != '\0')
        {
            uart.input_mutex.lock();
            if (uart.input_buffer.size() < max_input_buffer_size)
                uart.input_buffer.push(character);
            uart.input_mutex.unlock();
        }
    }
}

int UART::read_character()
{
    char buf = 0;
    struct termios old;

    if (tcgetattr(0, &old) < 0)
        return -1;

    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 10; // 0.1 seconds timeout

    if (tcsetattr(0, TCSANOW, &old) < 0)
        return -1;

    if (read(0, &buf, 1) < 0)
        return -1;

    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;

    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        return -1;

    return (buf);
}
