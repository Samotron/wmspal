#!/bin/bash

echo "Building WMSPal with vcpkg..."

# Set paths
VCPKG_ROOT="/home/samotron/vcpkg"
PROJECT_ROOT="/home/samotron/dev/wmspal"

# Configure CMAKE
cd "$PROJECT_ROOT"
mkdir -p build
cd build

echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DCMAKE_BUILD_TYPE=Release

echo "Building project..."
make -j$(nproc)

echo "Build complete!"
ls -la wmspal