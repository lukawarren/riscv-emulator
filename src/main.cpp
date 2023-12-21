#include <iostream>
#include "cpu.h"

int main()
{
    try
    {
        CPU cpu(1024 * 1024);
        cpu.bus.write_file(Bus::ram_base, "../external/bin-files/rv64-ui-p-addi");
        while(1)
        {
            //cpu.trace();
            //80000200 good
            //80000204 bad for x7 (i.e. bad opcode at 80000200 - addiw)
            //8000020c bad
            if (cpu.pc == 0x80000204)
            {
                cpu.trace();
            }
            cpu.cycle();
        }
    }
    catch (std::exception& e)
    {
        std::cout << "Error: " << e.what() << std::endl;
    }
    return 0;
}
