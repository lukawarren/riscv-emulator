#pragma once
#include "common.h"

std::pair<u8*, size_t> io_read_file(const std::string& filename);
std::tuple<u8*, size_t, int> io_map_file(const std::string& filename);
void io_flush_file(u8* local_address, size_t length);
void io_unmap_file(u8* address, size_t length, int fd);
