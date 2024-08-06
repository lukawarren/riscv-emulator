#!/bin/sh
set -e

# See https://people.debian.org/~gio/dqib/
DEBIAN_URL="https://gitlab.com/giomasce/dqib/-/jobs/7280338136/artifacts/download?file_type=archive"
DEBIAN_FILE="debian.zip"

mkdir -p build
cd build

# Setup Debian
if [ ! -d "debian" ]; then
    # Download
    echo "downloading Debian..."
    curl -L --output $DEBIAN_FILE $DEBIAN_URL

    # Extract
    rm -rf debian
    unzip debian.zip -d .
    mv dqib_riscv64-virt/ debian/

    # Convert qcow2 to raw img
    cd debian
    qemu-img convert -f qcow2 -O raw image.qcow2 image.img
    rm image.qcow2

    # Copy
    mv initrd ../../
    mv image.img ../../rootfs.img

    cd ..

    # Remove zip
    rm $DEBIAN_FILE
fi

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
make CROSS_COMPILE=riscv64-unknown-linux-gnu- PLATFORM_RISCV_XLEN=64 PLATFORM_RISCV_ISA=rv64imafd_zicsr_zifencei PLATFORM=generic FW_PAYLOAD_PATH=../debian/kernel
mv build/platform/generic/firmware/fw_payload.bin ../../image.bin

# Clean up
cd ../../
rm -rf build
mkdir output
mv image.bin output
mv initrd output
mv rootfs.img output