#!/bin/sh
set -e

INPUT_FILE="emulator.dts"
OUTPUT_FILE="../include/dtb.h"

# Compile
dtc -I dts -O dtb -o temp-initrd.dtb initrd.dts
dtc -I dts -O dtb -o temp-simple.dtb simple.dts

# Convert to raw bytes
hexdump -v -e '1/1 "0x%02X,"' temp-initrd.dtb > temp_initrd.h
hexdump -v -e '1/1 "0x%02X,"' temp-simple.dtb > temp_simple.h

# Format the header file
{
    echo "// This file is auto-generated from dt/build-dtb.sh"
    echo "#pragma once"
    echo "#include \"common.h\""
    echo
    echo "const u8 DTB_INITRD[] = {"
    cat temp_initrd.h
    echo "};"
    echo "const u8 DTB_SIMPLE[] = {"
    cat temp_simple.h
    echo "};"
} > "$OUTPUT_FILE"

# Clean up temporary files
rm temp*
echo "Header file generated: $OUTPUT_FILE"
