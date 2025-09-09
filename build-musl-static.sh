#!/bin/bash
# Build fully static WMSPal using musl-gcc

set -e

echo "Building fully static WMSPal with musl-gcc..."

# Check if musl-gcc is available
if ! command -v musl-gcc &> /dev/null; then
    echo "Installing musl..."
    sudo pacman -S --noconfirm musl
fi

# Create minimal static build (curl only)
mkdir -p build-musl
cd build-musl

# Download and build curl statically with musl
if [ ! -f "libcurl.a" ]; then
    echo "Building static curl..."
    wget -O curl.tar.gz "https://curl.se/download/curl-8.4.0.tar.gz"
    tar -xf curl.tar.gz
    cd curl-*
    
    CC=musl-gcc ./configure \
        --enable-static \
        --disable-shared \
        --disable-ldap \
        --disable-ldaps \
        --without-ssl \
        --without-zlib \
        --disable-ipv6
        
    make -j$(nproc)
    cp lib/.libs/libcurl.a ../
    cp -r include/curl ../
    cd ..
fi

echo "Building WMSPal with musl-gcc..."

# Compile WMSPal statically
musl-gcc -std=c11 -static -O2 \
    -I../include -Icurl \
    -DHAVE_CURL \
    ../src/main.c \
    ../src/wms.c \
    ../src/georeference.c \
    ../src/vectorize.c \
    ../src/attribution.c \
    libcurl.a \
    -o wmspal-static

echo "Static build complete!"
echo "Binary: $(pwd)/wmspal-static"

# Check dependencies
echo -e "\nDependency check:"
ldd wmspal-static 2>&1 || echo "Fully static binary (no dynamic dependencies)"

# Get binary size
echo -e "\nBinary size: $(ls -lh wmspal-static | awk '{print $5}')"

# Test basic functionality
echo -e "\nTesting binary:"
./wmspal-static --version || ./wmspal-static --help