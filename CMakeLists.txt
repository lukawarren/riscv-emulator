cmake_minimum_required(VERSION 3.20)
project(riscv-emulator)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable warnings, flags, etc.
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Werror -Wall -Wextra -pedantic -Wno-unused-parameter -O3 -g -fdiagnostics-color=always)
endif()

# Specify source files and include directories
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

# Add the executable target
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
