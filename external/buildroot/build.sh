#!/bin/sh
set -e

BUILDROOT_VERSION=2024.02.4
BUILDROOT_URL="https://buildroot.org/downloads/buildroot-$BUILDROOT_VERSION.tar.gz"
BUILDROOT_FILE="buildroot-$BUILDROOT_VERSION.tar.xz"
BUILDROOT_EXTRACT_DIR="buildroot-$BUILDROOT_VERSION"

mkdir -p build
cd build

# Download Linux tar
if [ ! -f "$BUILDROOT_FILE" ]; then
    echo "downloading buildroot..."
    curl --output $BUILDROOT_FILE $BUILDROOT_URL
fi

# Extract
if [ ! -d "$BUILDROOT_EXTRACT_DIR" ]; then
    echo "extracting $BUILDROOT_FILE..."
    tar -xf "$BUILDROOT_FILE"
    cd $BUILDROOT_EXTRACT_DIR

    # Apply config
    rm -f .config
    cp ../../buildroot.config .config
    cd ..
fi

# Build
cd $BUILDROOT_EXTRACT_DIR
make
cd ../../

# Copy
cp build/$BUILDROOT_EXTRACT_DIR/output/images/rootfs.cpio .