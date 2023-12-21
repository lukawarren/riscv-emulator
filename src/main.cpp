#include <iostream>
#include <cstring>
#include <filesystem>
#include <vector>
#include "cpu.h"

namespace fs = std::filesystem;

bool does_pass(const std::string& filename);

int passes = 0;

int main()
{
    std::vector<std::string> files = {};
    for (const auto& entry : fs::directory_iterator("../external/bin-files")) {
        if (fs::is_regular_file(entry.path())) {
            files.emplace_back(entry.path());
        }
    }

    for (const auto& file : files)
        if (!does_pass(file))
        {
            std::cout << std::dec << passes << " passes out of " << files.size() << std::endl;
            return 1;
        }

    return 0;
}

bool does_pass(const std::string& filename)
{
    try
    {
        CPU cpu(1024 * 1024);
        cpu.bus.write_file(Bus::ram_base, filename);
        while(1)
        {
            cpu.trace();
            cpu.cycle();
        }
    }
    catch (std::exception& e)
    {
        if (strcmp(e.what(), "pass") == 0)
        {
            std::cout << "pass" << std::endl;
            ++passes;
            return true;
        }
        else
        {
            std::cout << filename << " error: " << e.what() << std::endl;
            return false;
        }
    }
}