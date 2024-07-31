#include "cpu.h"
#include "io.h"

typedef std::pair<std::string, std::optional<std::string>> Arg;
static std::array<Arg, 4> args = {{
    { "--test",  "n" },
    { "--image", std::nullopt },
    { "--dts",   std::nullopt },
    { "--blk",   std::nullopt }
}};

static void print_usage(char** argv)
{
    std::cerr << "usage: " << argv[0] << " [--test] [--image FILE] [--dts FILE]" << std::endl;
}

int main(int argc, char** argv)
{
    // Parse argc
    for (int i = 1; i < argc; i++)
    {
        for (size_t j = 0; j < args.size(); ++j)
        {
            if (std::string(argv[i]) == args[j].first)
            {
                if (args[j].first == "--test")
                    args[j].second = "y";
                else
                    args[j].second = std::string(argv[i + 1]);
            }
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
    else if (!test_mode && !args[2].second.has_value())
    {
        // Need a DTS if we're running Linux, etc.
        std::cerr << "no DTS file specified" << std::endl;
        print_usage(argv);
        return 1;
    }

    u8* dtb = nullptr;
    size_t dtb_size = 0;

    if (args[2].second.has_value())
    {
        // Attempt to compile device tree
        std::string command = "dtc -I dts -O dtb " + *args[2].second + " -o /tmp/riscv-emulator.dtb";
        int result = system(command.c_str());
        if (result != 0)
            throw std::runtime_error("failed to compile device tree - do you have dtc installed?");

        // Load dtb
        std::pair<u8*, size_t> file = io_read_file("/tmp/riscv-emulator.dtb");
        dtb = file.first;
        dtb_size = file.second;
    }

    try
    {
        // Set up CPU
        CPU cpu(
            128 * 1024 * 1024,
            test_mode,
            args[3].second,
            dtb,
            dtb_size
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
    catch (std::exception& e)
    {
        // Genuine exception
        std::cout << "unhandled exception: " << e.what() << std::endl;
    }
    catch (std::string& s)
    {
        // Test pass (or fail!)
        return (s == "pass");
    }
}
