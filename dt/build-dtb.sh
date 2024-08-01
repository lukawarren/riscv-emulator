#!/bin/sh
set -e

INPUT_FILE="emulator.dts"
OUTPUT_FILE="../include/dtb.h"

# Compile
dtc -I dts -O dtb -o temp.dtb "$INPUT_FILE"

# Convert to raw bytes
hexdump -v -e '1/1 "0x%02X,"' temp.dtb > temp_dtb.h

# Format the header file
{
    echo "// This file is auto-generated from dt/build-dtb.sh"
    echo "#pragma once"
    echo "#include \"common.h\""
    echo
    echo "const u8 DTB[] = {"
    cat temp_dtb.h
    echo "};"
} > "$OUTPUT_FILE"

# Clean up temporary files
rm temp.dtb temp_dtb.h
echo "Header file generated: $OUTPUT_FILE"
