#include <iostream>
#include "cpu.h"

int main()
{
    CPU cpu(1024 * 1024);
    cpu.bus.write_file(Bus::ram_base, "../external/bin-files/rv64-ui-p-addi");
    while(1)
    {
    cpu.trace();
    cpu.cycle();
    }
    return 0;
}
