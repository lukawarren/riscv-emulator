#include <iostream>
#include <cstring>
#include <vector>
#include "cpu.h"

bool does_pass(const std::string& filename)
{
    try
    {
        CPU cpu(100 * 1024 * 1024);
        cpu.bus.write_file(Bus::ram_base, filename);
        while(1)
        {
            cpu.trace();
            cpu.do_cycle();
        }
    }
    catch (std::string& s)
    {
        return (s == "pass");
    }
}

int main(int argc, char** argv)
{
    return does_pass(std::string(argv[1]));
}