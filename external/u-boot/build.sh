#!/bin/sh
set -e

U_BOOT_VERSION="2024.10-rc1"
U_BOOT_URL="https://github.com/u-boot/u-boot/archive/refs/tags/v$U_BOOT_VERSION.tar.gz"
U_BOOT_FILE="u-boot-$U_BOOT_VERSION.tar.xz"
U_BOOT_EXTRACT_DIR="u-boot-$U_BOOT_VERSION"

mkdir -p build
cd build

# Download u-boot
if [ ! -f "$U_BOOT_FILE" ]; then
    echo "downloading u-boot..."
    curl -L --output $U_BOOT_FILE $U_BOOT_URL
fi

# Extract
if [ ! -d "$U_BOOT_EXTRACT_DIR" ]; then
    echo "extracting $U_BOOT_FILE..."
    tar -xf "$U_BOOT_FILE"

    # Apply config
    cd $U_BOOT_EXTRACT_DIR
    rm -f .config
    cp ../../u-boot.config .config
    cd ..
fi

# Build
cd $U_BOOT_EXTRACT_DIR
echo "building u-boot..."
time make -j$(nproc) CROSS_COMPILE=riscv64-unknown-linux-gnu-
cd ..

# Download opensbi
if [ ! -d "opensbi" ]; then
    echo "cloning opensbi..."
    git clone https://github.com/riscv/opensbi.git opensbi
    cd opensbi
    git checkout 70ffc3e2e690f2b7bcea456f49206b636420ef5f &> /dev/null
    cd ..
fi

# Build
cd opensbi
echo "building opensbi..."
time make CROSS_COMPILE=riscv64-unknown-linux-gnu- PLATFORM_RISCV_XLEN=64 PLATFORM_RISCV_ISA=rv64imafd_zicsr_zifencei PLATFORM=generic FW_PAYLOAD_PATH=../$U_BOOT_EXTRACT_DIR/u-boot.bin
cp build/platform/generic/firmware/fw_payload.bin ../../image.bin
