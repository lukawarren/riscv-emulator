cmake_minimum_required(VERSION 3.20)
project(riscv-emulator)

# Set C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Specify source files and include directories
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

# Add the executable target
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Enable warnings, flags, etc.
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Werror -Wall -Wextra -pedantic -Wno-unused-parameter -O3 -g -fdiagnostics-color=always)
endif()

# Dependencies
add_subdirectory(external/riscv-disassembler)
target_link_libraries(${PROJECT_NAME} PRIVATE rvasm)

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    external/riscv-disassembler/src
)
