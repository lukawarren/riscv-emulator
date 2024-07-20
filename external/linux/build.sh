#!/bin/sh
set -e

mkdir -p build
cd build

KERNEL_URL="https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.9.10.tar.xz"
KERNEL_FILE="linux-6.9.10.tar.xz"
KERNEL_EXTRACT_DIR="linux-6.9.10"

# Download Linux tar
if [ ! -f "$KERNEL_FILE" ]; then
    echo "downloading linux..."
    curl --output $KERNEL_FILE $KERNEL_URL
fi

# Extract
if [ ! -d "$KERNEL_EXTRACT_DIR" ]; then
    echo "extracting $KERNEL_FILE..."
    tar -xf "$KERNEL_FILE"

    # Apply patches
    cd $KERNEL_EXTRACT_DIR
    echo "patching linux..."
    patch -p1 < ../../patches/linux/0001-add-simple-uart.patch

    # Apply config
    rm -f .config
    cp ../../linux.config .config

    cd ..
fi

# Build
cd $KERNEL_EXTRACT_DIR
echo "building linux..."
time make -j$(nproc) ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- all
cd ../

# Download opensbi
if [ ! -d "opensbi" ]; then
    echo "cloning opensbi..."
    git clone https://github.com/riscv/opensbi.git opensbi
    cd opensbi
    git checkout 70ffc3e2e690f2b7bcea456f49206b636420ef5f &> /dev/null

    # Apply patches
    echo "patching opensbi..."
    git apply ../../patches/opensbi/0001-add-simple-uart.patch

    cd ..
fi

# Build
cd opensbi
echo "building opensbi..."
make CROSS_COMPILE=riscv64-unknown-linux-gnu- PLATFORM_RISCV_XLEN=64 PLATFORM_RISCV_ISA=rv64imafd_zicsr_zifencei PLATFORM=generic FW_PAYLOAD_PATH=../$KERNEL_EXTRACT_DIR/arch/riscv/boot/Image
