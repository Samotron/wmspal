# WMSPal Build Guide

## Quick Start (System Dependencies - Recommended)

The fastest way to build WMSPal is with system packages:

```bash
# Install dependencies (Arch Linux)
sudo pacman -S geos proj curl

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Result**: Fast build (~30 seconds), ~30MB dependencies, professional WKT output

## Static Linking Support

WMSPal includes comprehensive static linking support for creating portable binaries.

### Option 1: Full Static Build from Source

For a completely self-contained binary, use the automated static build script:

```bash
./build-static.sh
```

This script:
- Downloads and builds curl, GEOS, and PROJ from source
- Creates fully static libraries with no dynamic dependencies
- Takes ~15-20 minutes but produces a portable binary
- Binary location: `static-build-final/wmspal`

### Option 2: CMake Static Build Options

For more control over the static build process:

```bash
# Static linking with system libraries (if available)
mkdir build-static && cd build-static
cmake .. -DCMAKE_BUILD_TYPE=Release -DSTATIC_LINKING=ON
make -j$(nproc)

# Musl static build (requires musl-gcc)
cmake .. -DCMAKE_BUILD_TYPE=Release -DMUSL_STATIC=ON -DCMAKE_C_COMPILER=musl-gcc
make -j$(nproc)
```

### Static Library Locations

After running `build-static.sh`, static libraries are available at:
- `static-build/install/lib/libcurl.a` - HTTP client functionality
- `static-build/install/lib/libgeos.a` & `libgeos_c.a` - Geometry operations
- `static-build/install/lib/libproj.a` - Coordinate system transformations

### Build Options Summary

| Option | Build Time | Dependencies | Binary Size | Use Case |
|--------|------------|--------------|-------------|----------|
| System packages | ~30 seconds | ~30MB | ~50KB | Development/most users |
| Static from source | ~20 minutes | 0 (bundled) | ~15MB | Distribution/deployment |
| Musl static | ~25 minutes | 0 (bundled) | ~12MB | Embedded/containers |

## Dependencies

### Required
- **curl**: HTTP client for downloading WMS tiles
- **CMake**: Build system (≥3.20)
- **C11 compiler**: GCC or Clang

### Optional (with graceful fallback)
- **GEOS**: Advanced geometry operations and WKT output
  - Without: Basic world file georeferencing only
  - With: Professional WKT coordinate reference systems
- **PROJ**: Coordinate system transformations  
  - Without: Limited projection support
  - With: Full EPSG database and transformation capabilities

### Runtime Dependencies (Dynamic Builds Only)
- libcurl (~2MB)
- libgeos (~10MB) - optional
- libproj (~15MB) - optional

## Minimal Build

For environments where dependencies are not available:

```bash
# Build with only curl (basic functionality)
gcc -O3 -Iinclude src/*.c -o wmspal -lcurl -lm
```

**Result**: Basic WMS downloading with world file georeferencing

## Usage

```bash
./wmspal --url "http://example.com/wms" \
         --layer "layer_name" \
         --bbox "-180,-90,180,90" \
         --output "output.png" \
         --vectorize \
         --attribution
```

## Options

- `-u, --url`: WMS service URL
- `-l, --layer`: Layer name to download  
- `-b, --bbox`: Bounding box (minx,miny,maxx,maxy)
- `-s, --srs`: Spatial reference system (default: EPSG:4326)
- `-w, --width`: Image width in pixels (default: 256)
- `-h, --height`: Image height in pixels (default: 256)
- `-f, --format`: Image format (default: image/png)
- `-o, --output`: Output file name
- `-v, --vectorize`: Vectorize the georeferenced image
- `-a, --attribution`: Apply attribution using GetFeatureInfo

## Architecture Support

WMSPal builds on all major architectures:
- x86_64 (tested)
- ARM64 (should work)  
- i386 (should work)

Static builds are particularly useful for cross-platform distribution and containerized deployments.