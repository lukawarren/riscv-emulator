#include "cpu.h"

bool does_pass(const std::string& filename)
{
    try
    {
        CPU cpu(128 * 1024 * 1024, true);
        cpu.bus.write_file(Bus::programs_base, filename);

        while(1)
        {
            cpu.trace();
            cpu.do_cycle();
            cpu.bus.clock(cpu);

            const std::optional<CPU::PendingTrap> trap = cpu.get_pending_trap();
            if (trap.has_value())
                cpu.handle_trap(trap->cause, trap->info, trap->is_interrupt);
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
        cpu.do_cycle();
        cpu.bus.clock(cpu);

        const std::optional<CPU::PendingTrap> trap = cpu.get_pending_trap();
        if (trap.has_value())
            cpu.handle_trap(trap->cause, trap->info, trap->is_interrupt);
    }
}

int main(int argc, char** argv)
{
    if (argc == 3) // called with --tests or whatever
        return does_pass(std::string(argv[2]));
    else
        emulate(std::string(argv[1]));
}
