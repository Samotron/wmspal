#!/bin/bash
# Build static WMSPal with dependencies built from source

set -e

BUILD_DIR="static-build"
PREFIX="${PWD}/${BUILD_DIR}/install"

echo "Building WMSPal with static dependencies..."
echo "Install prefix: $PREFIX"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Install essential build tools if needed
if ! command -v cmake &> /dev/null; then
    echo "Error: cmake is required but not installed"
    exit 1
fi

# Function to build a dependency from source
build_dependency() {
    local name="$1"
    local url="$2"
    local version="$3"
    local cmake_args="$4"
    
    echo "Building $name $version..."
    
    if [ ! -d "$name" ]; then
        echo "Downloading $name..."
        if [[ "$url" == *.tar.gz ]] || [[ "$url" == *.tar.bz2 ]]; then
            wget -O "$name.tar.gz" "$url"
            tar -xf "$name.tar.gz"
            mv "$name-$version" "$name" 2>/dev/null || mv "$name"* "$name"
        else
            git clone --depth 1 --branch "$version" "$url" "$name"
        fi
    fi
    
    cd "$name"
    mkdir -p build
    cd build
    
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        $cmake_args
        
    make -j$(nproc)
    make install
    cd ../..
}

# Build curl statically
if [ ! -f "$PREFIX/lib/libcurl.a" ]; then
    echo "Building curl..."
    if [ ! -d "curl" ]; then
        wget -O curl.tar.gz "https://curl.se/download/curl-8.4.0.tar.gz"
        tar -xf curl.tar.gz
        mv curl-* curl
    fi
    cd curl
    ./configure --prefix="$PREFIX" --enable-static --disable-shared --disable-ldap --disable-ldaps
    make -j$(nproc)
    make install
    cd ..
fi

# Build GEOS statically
if [ ! -f "$PREFIX/lib/libgeos_c.a" ]; then
    build_dependency "geos" "https://github.com/libgeos/geos.git" "3.12.0" "-DGEOS_BUILD_SHARED=OFF"
fi

# Build PROJ statically
if [ ! -f "$PREFIX/lib/libproj.a" ]; then
    build_dependency "proj" "https://github.com/OSGeo/PROJ.git" "9.3.0" "-DBUILD_SHARED_LIBS=OFF -DENABLE_CURL=OFF"
fi

echo "Dependencies built successfully!"

# Build WMSPal with static linking
cd ..
echo "Building WMSPal..."

# Set environment for finding static libraries
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH"
export CMAKE_PREFIX_PATH="$PREFIX:$CMAKE_PREFIX_PATH"

# Clean previous build
rm -rf build-static-final
mkdir build-static-final
cd build-static-final

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DSTATIC_LINKING=ON \
    -DCMAKE_PREFIX_PATH="$PREFIX"

make -j$(nproc)

echo "Static build complete!"
echo "Binary: $(pwd)/wmspal"

# Check if it's actually statically linked
echo -e "\nDependency check:"
ldd wmspal || echo "Fully static binary (no dynamic dependencies)"

# Get binary size
echo -e "\nBinary size: $(ls -lh wmspal | awk '{print $5}')"