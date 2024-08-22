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
# Must have -ffloat-store (or /fp:precise) for RV64F's IEEE 754
if(MSVC)
    message(WARNING "Untested on MSVC - RV64FD might be inaccurate")
    target_compile_options(${PROJECT_NAME} PRIVATE /fp:precise /W4)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang
    target_compile_options(${PROJECT_NAME} PRIVATE -Werror -Wall -Wextra -pedantic -Wno-unused-parameter -O3 -g -fdiagnostics-color=always)
    message(WARNING "You are using Clang, so -ffloat-store is not supported. Do not expect accurate RV64FD emulation!")
else()
    # Probably GCC
    target_compile_options(${PROJECT_NAME} PRIVATE -ffloat-store -Werror -Wall -Wextra -pedantic -Wno-unused-parameter -O3 -g -fdiagnostics-color=always)
endif()

# Dependencies
add_subdirectory(external/riscv-disassembler)
target_link_libraries(${PROJECT_NAME} PRIVATE rvasm)

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    external/riscv-disassembler/src
    external/dbg-macro/
)

# PCH
target_precompile_headers(${PROJECT_NAME} PRIVATE include/common.h)
target_precompile_headers(${PROJECT_NAME} PRIVATE include/jit/llvm.h)

# LLVM
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(${PROJECT_NAME} PRIVATE LLVM)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE LLVM-18)
endif()
