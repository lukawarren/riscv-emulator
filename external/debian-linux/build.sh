#!/bin/sh
set -e

DEBIAN_URL="https://gitlab.com/api/v4/projects/giomasce%2Fdqib/jobs/artifacts/master/download?job=convert_riscv64-virt"
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
cp build/platform/generic/firmware/fw_payload.bin ../../image.bin