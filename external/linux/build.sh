#!/bin/sh
set -e

KERNEL_VERSION=6.9.10
KERNEL_URL="https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-$KERNEL_VERSION.tar.xz"
KERNEL_FILE="linux-$KERNEL_VERSION.tar.xz"
KERNEL_EXTRACT_DIR="linux-$KERNEL_VERSION"

mkdir -p build
cd build

# Download Linux tar
if [ ! -f "$KERNEL_FILE" ]; then
    echo "downloading linux..."
    curl --output $KERNEL_FILE $KERNEL_URL
fi

# Extract
if [ ! -d "$KERNEL_EXTRACT_DIR" ]; then
    echo "extracting $KERNEL_FILE..."
    tar -xf "$KERNEL_FILE"

    # Apply config
    cd $KERNEL_EXTRACT_DIR
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
    cd ..
fi

# Build
cd opensbi
echo "building opensbi..."
make CROSS_COMPILE=riscv64-unknown-linux-gnu- PLATFORM_RISCV_XLEN=64 PLATFORM_RISCV_ISA=rv64imafd_zicsr_zifencei PLATFORM=generic FW_PAYLOAD_PATH=../$KERNEL_EXTRACT_DIR/arch/riscv/boot/Image

# Copy
cp build/platform/generic/firmware/fw_payload.bin ../../image.bin