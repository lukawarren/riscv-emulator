#include "cpu.h"

bool does_pass(const std::string& filename)
{
    try
    {
        CPU cpu(1 * 1024 * 1024, true);
        cpu.bus.write_file(Bus::programs_base, filename);
        while(1)
        {
            cpu.trace();
            cpu.bus.clock(cpu);

            if (!cpu.waiting_for_interrupts)
                cpu.do_cycle();

            const std::optional<Interrupt> interrupt = cpu.get_pending_interrupt();
            if (interrupt.has_value())
                cpu.raise_interrupt(*interrupt);
        }
    }
    catch (std::string& s)
    {
        return (s == "pass");
    }
}

void emulate(const std::string& filename)
{
    CPU cpu(128 * 1024 * 1024);
    cpu.bus.write_file(Bus::programs_base, filename);

    while(1)
    {
        cpu.bus.clock(cpu);

        const std::optional<Interrupt> interrupt = cpu.get_pending_interrupt();
        if (interrupt.has_value())
            cpu.raise_interrupt(*interrupt);

        if (!cpu.waiting_for_interrupts)
            cpu.do_cycle();
    }
}

int main(int argc, char** argv)
{
    if (argc == 3) // called with --tests or whatever
        return does_pass(std::string(argv[2]));
    else
        emulate(std::string(argv[1]));
}
