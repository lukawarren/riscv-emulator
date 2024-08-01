#include "cpu.h"
#include "io.h"

typedef std::pair<std::string, std::optional<std::string>> Arg;
static std::array<Arg, 3> args = {{
    { "--test",  "n" },
    { "--image", std::nullopt },
    { "--blk",   std::nullopt }
}};

static void print_usage(char** argv)
{
    std::cerr << "usage: " << argv[0] << " [--test] [--image FILE] [--blk FILE]" << std::endl;
}

int main(int argc, char** argv)
{
    // Parse argc
    int valid_skip = -1;
    for (int i = 1; i < argc; i++)
    {
        bool found = false;
        for (size_t j = 0; j < args.size(); ++j)
        {
            if (std::string(argv[i]) == args[j].first)
            {
                if (args[j].first == "--test")
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

    try
    {
        // Set up CPU
        CPU cpu(
            128 * 1024 * 1024,
            test_mode,
            args[2].second
        );
        cpu.bus.write_file(Bus::programs_base, *args[1].second);

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
        if (test_mode) emulate.operator()<true>();
        else           emulate.operator()<false>();

    }
    catch (std::string& s)
    {
        // Test pass (or fail!)
        return (s == "pass");
    }
}
