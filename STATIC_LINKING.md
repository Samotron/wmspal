# Static Linking Guide for WMSPal

WMSPal provides several options for static linking to create portable binaries without external dependencies.

## Current Dynamic Linking Status

The default build uses dynamic linking with these dependencies:
- **libcurl** (HTTP requests)
- **libgeos_c + libgeos** (geometry operations) 
- **libproj** (coordinate projections)
- Plus ~40 transitive dependencies (SSL, compression, etc.)

Dynamic binary size: ~24KB + system dependencies

## Static Linking Options

### Option 1: CMake Static Build (Recommended)

Build with dependencies from source for maximum compatibility:

```bash
# Build all dependencies statically from source
./build-static.sh
```

This script:
1. Downloads and builds curl, GEOS, and PROJ from source
2. Creates static libraries (.a files) 
3. Links everything statically into WMSPal
4. Results in a ~10-15MB fully portable binary

**Pros:** 
- Includes all features (GEOS + PROJ support)
- Professional-grade dependencies
- Portable across Linux distributions

**Cons:** 
- Longer build time (~10-15 minutes)
- Larger binary size

### Option 2: Musl Static Build (Minimal)

Build minimal static binary with musl-gcc:

```bash
# Build minimal static binary (curl only)
./build-musl-static.sh
```

This creates a minimal build:
- Static curl compiled with musl
- No GEOS/PROJ dependencies
- Basic WMS download functionality only
- Results in ~2-3MB fully static binary

**Pros:**
- Very small binary size
- Fast build (~2-3 minutes)
- True static binary (no libc dependency)

**Cons:**
- Limited functionality (no geometry/projection features)
- Requires musl-gcc

### Option 3: CMake Static Flag

Use CMake's built-in static linking:

```bash
mkdir build-static
cd build-static
cmake .. -DSTATIC_LINKING=ON
make
```

**Note:** This requires static versions of system libraries, which may not be available on all distributions.

## Choosing the Right Option

| Use Case | Recommended Option |
|----------|-------------------|
| **Development/Testing** | Dynamic build (default) |
| **Production Deployment** | Option 1 (build-static.sh) |
| **Embedded/Minimal Systems** | Option 2 (musl static) |
| **Quick Portable Binary** | Option 1 (build-static.sh) |

## Build Scripts

### build-static.sh
- Downloads curl 8.4.0, GEOS 3.12.0, PROJ 9.3.0
- Builds from source with static linking
- Full-featured WMSPal binary
- ~10-15MB final size

### build-musl-static.sh  
- Uses musl-gcc for static compilation
- Minimal curl dependency only
- ~2-3MB final size
- Basic WMS functionality only

## Verifying Static Linking

Check if binary is statically linked:

```bash
# Dynamic binary shows many dependencies
ldd wmspal

# Static binary shows "not a dynamic executable" or minimal deps
ldd wmspal-static
```

## Distribution

Static binaries can be distributed without requiring users to install dependencies:

```bash
# Copy static binary to any Linux system
scp wmspal-static user@remote:/usr/local/bin/wmspal
```

## Troubleshooting

**Build fails with missing dependencies:**
- Install build essentials: `sudo pacman -S base-devel cmake git wget`

**Large binary size:**
- Use Option 2 (musl) for minimal build
- Strip debugging symbols: `strip wmspal-static`

**Runtime errors on other systems:**
- Ensure target system architecture matches (x86_64)
- For maximum compatibility, build on older distribution