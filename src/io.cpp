#include "io.h"

std::pair<u8*, size_t> io_read_file(const std::string& filename)
{
    // Check isn't folder
    if (!std::filesystem::is_regular_file(filename))
        throw std::runtime_error(filename + " is not a file");

    // Open file
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("unable to open file" + filename);

    // Get file length
    file.seekg(0, std::ios::end);
    u64 file_length = file.tellg();
    file.seekg(0, std::ios::beg);

    // Allocate memory
    u8* buffer = new u8[file_length];
    if (!buffer)
    {
        file.close();
        throw std::runtime_error("unable to load entire file into memory");
    }

    // Read file contents into buffer
    file.read(reinterpret_cast<char*>(buffer), file_length);
    file.close();
    return { buffer, file_length };
}
