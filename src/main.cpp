#include "cpu.h"
#include "io.h"
#include "jit/jit.h"

static void print_usage(char** argv)
{
    std::cerr << "usage: " << argv[0] << " [--test] [--jit] [--image FILE] [--blk FILE] [--initramfs FILE]" << std::endl;
}

int main(int argc, char** argv)
{
    typedef std::pair<std::string, std::optional<std::string>> Arg;
    std::array<Arg, 5> args = {{
        { "--test",         "n" },
        { "--image",        std::nullopt },
        { "--blk",          std::nullopt },
        { "--initramfs",    std::nullopt },
        { "--jit",          std::nullopt }
    }};

    // Parse argc
    int valid_skip = -1;
    for (int i = 1; i < argc; i++)
    {
        bool found = false;
        for (size_t j = 0; j < args.size(); ++j)
        {
            if (std::string(argv[i]) == args[j].first)
            {
                if (args[j].first == "--test" || args[j].first == "--jit")
                    args[j].second = "y";
                else
                {
                    args[j].second = std::string(argv[i + 1]);
                    valid_skip = i + 1;
                }

                found = true;
            }
        }

        if (!found && i != valid_skip)
        {
            std::cerr << "unknown argument: " << argv[i] << std::endl;
            print_usage(argv);
            return 1;
        }
    }
    const bool test_mode = args[0].second == "y";

    // Validate args
    if (!args[1].second.has_value())
    {
        std::cerr << "no image file specified" << std::endl;
        print_usage(argv);
        return 1;
    }

    // Set up CPU
    CPU cpu(
        test_mode ? (16 * 1024 * 1024) : (2UL * 1024 * 1024 * 1024),
        test_mode,
        args[3].second.has_value(),
        args[2].second
    );

    // Load main kernel / program / image
    std::ignore = cpu.bus.write_file(Bus::programs_base, *args[1].second);

    if (args[3].second.has_value())
    {
        // Load initramfs - this is the address QEMU uses and decompression
        // will fail on Debian testing if it isn't this, even if the would-be
        // address is otherwise properly aligned...
        const size_t address = 0xa0200000;
        const size_t size = cpu.bus.write_file(address, *args[3].second);
        if (size != (0xa25f03a6 - address))
        {
            throw std::runtime_error("initramfs size conflicts with the"
                " value in the DTB - you will have to modify the .dts file"
                " and the value in code (directly above) too");
        }
    }

    // Interpreter
    if (!args[4].second.has_value())
    {
        // Enter emulation loop
        const auto emulate = [&]<bool test_mode>()
        {
            while(1)
            {
                if constexpr(test_mode)
                    cpu.trace();

                cpu.do_cycle();
                cpu.bus.clock(cpu);

                const std::optional<CPU::PendingTrap> trap = cpu.get_pending_trap();
                if (trap.has_value())
                    cpu.handle_trap(trap->cause, trap->info, trap->is_interrupt);
            }
        };
        if (test_mode)
            emulate.operator()<true>();
        else
            emulate.operator()<false>();
    }

    // JIT
    else
    {
        JIT::init();
        while(true)
        {
            JIT::run_next_frame(cpu);
            cpu.bus.clock(cpu, true);
            cpu.mcycle.increment(cpu);
            cpu.minstret.increment(cpu);
            cpu.time.increment(cpu);
        }
    }
}
