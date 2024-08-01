#include "io.h"

std::pair<u8*, size_t> io_read_file(const std::string& filename)
{
    // Check isn't folder
    if (!std::filesystem::is_regular_file(filename))
        throw std::runtime_error(filename + " is not a file");

    // Open file
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("unable to open file " + filename);

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

std::tuple<u8*, size_t, int> io_map_file(const std::string& filename)
{
    int fd = open(filename.c_str(), O_RDWR);
    if (fd < 0)
        throw std::runtime_error("failed to load file " + filename);

    struct stat info;
    if (fstat(fd, &info) < 0)
        throw std::runtime_error("failed to determine file size for " + filename);

    u8* buffer = (u8*)mmap(0, info.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buffer == MAP_FAILED)
        throw std::runtime_error("failed to mmap " + filename);

    return { buffer, info.st_size, fd };
}

void io_flush_file(u8* local_address, size_t length)
{
    if (msync((void*)local_address, length, MS_SYNC) < 0)
        throw std::runtime_error("msync error " + std::to_string(errno));
}

void io_unmap_file(u8* address, size_t length, int fd)
{
    close(fd);
    if (munmap((void*)address, length) < 0)
        throw std::runtime_error("failed to munmap file");
}
