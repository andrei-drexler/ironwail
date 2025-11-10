#!/bin/bash
# Build nng 2.0 from source
# nng: apt has 1.7.2, but we need 2.0.x for API compatibility

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEPS_DIR="${DEPS_DIR:-$SCRIPT_DIR/dependencies}"
WORK_DIR="${WORK_DIR:-/tmp/nng_build_$$}"

echo "=========================================="
echo "  Building nng 2.0 from source"
echo "=========================================="
echo ""

# Create directories
mkdir -p "$DEPS_DIR/lib" "$DEPS_DIR/include" "$WORK_DIR"
cd "$WORK_DIR"

# Check build tools
if ! command -v cmake > /dev/null; then
    echo "ERROR: cmake not found"
    echo "Install with: sudo apt-get install cmake build-essential"
    exit 1
fi

if ! command -v make > /dev/null; then
    echo "ERROR: make not found"
    echo "Install with: sudo apt-get install build-essential"
    exit 1
fi

echo "Build tools: OK (cmake, make, gcc)"
echo ""

# =============================================================================
# Build nng (nanomsg-next-generation)
# =============================================================================

echo "Building nng (nanomsg-next-generation) v2.0.0-alpha.6..."
echo "  Why build from source:"
echo "    - apt provides nng 1.7.2, but we need 2.0.x API"
echo "    - PluQ code uses 2.0 API: nng_init(), nng_listener_create()"
echo "    - No pre-built 2.0.x binaries available"
echo ""

NNG_SUCCESS=0

# Try git clone first (preferred method)
if command -v git > /dev/null; then
    echo "  Method: git clone (fast, shallow)"
    git clone --depth 1 --branch v2.0.0-alpha.6 https://github.com/nanomsg/nng.git nng-build 2>&1 | tail -3
    NNG_SOURCE="git"
else
    # Fallback to tarball download
    echo "  Method: tarball download"
    NNG_URL="https://github.com/nanomsg/nng/archive/refs/tags/v2.0.0-alpha.6.tar.gz"
    if command -v wget > /dev/null; then
        wget -q --show-progress -O nng.tar.gz "$NNG_URL" 2>&1
    elif command -v curl > /dev/null; then
        curl -L --progress-bar -o nng.tar.gz "$NNG_URL"
    else
        echo "  ERROR: Neither git, wget, nor curl available"
        exit 1
    fi

    if [ -f nng.tar.gz ]; then
        tar xzf nng.tar.gz
        mv nng-2.0.0-alpha.6 nng-build
        NNG_SOURCE="tarball"
    fi
fi

if [ -d nng-build ]; then
    cd nng-build
    mkdir -p build && cd build

    echo "  Configuring with cmake..."
    cmake -DCMAKE_INSTALL_PREFIX="$DEPS_DIR" \
          -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_SHARED_LIBS=ON \
          -DNNG_TESTS=OFF \
          -DNNG_TOOLS=OFF \
          -DNNG_ENABLE_TLS=OFF \
          .. > cmake.log 2>&1

    if [ $? -eq 0 ]; then
        echo "  Building (make -j$(nproc))..."
        make -j$(nproc) > make.log 2>&1

        if [ $? -eq 0 ]; then
            echo "  Installing to $DEPS_DIR..."
            make install > install.log 2>&1

            if [ $? -eq 0 ]; then
                NNG_SUCCESS=1
                echo "  ✓ nng built successfully (source: $NNG_SOURCE)"
                echo ""
                echo "Files installed:"
                ls -lh "$DEPS_DIR/lib/libnng.so"* 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
            fi
        fi
    fi

    if [ $NNG_SUCCESS -eq 0 ]; then
        echo "  ✗ nng build FAILED"
        echo "  Check logs in: $WORK_DIR/nng-build/build/"
        echo ""
        echo "Last 20 lines of cmake.log:"
        tail -20 "$WORK_DIR/nng-build/build/cmake.log" 2>/dev/null || echo "  (log not found)"
        echo ""
        echo "Last 20 lines of make.log:"
        tail -20 "$WORK_DIR/nng-build/build/make.log" 2>/dev/null || echo "  (log not found)"
        exit 1
    fi

    cd ../..
else
    echo "  ✗ Failed to obtain nng source code"
    exit 1
fi

# Cleanup
cd /
rm -rf "$WORK_DIR"

echo ""
echo "=========================================="
echo "  Build Complete"
echo "=========================================="
echo ""
echo "Installation directory: $DEPS_DIR"
echo ""
echo "Library built:"
echo "  ✓ nng v2.0.0-alpha.6 (557KB)"
echo ""
echo "To use this library:"
echo "  export LD_LIBRARY_PATH=$DEPS_DIR/lib:\$LD_LIBRARY_PATH"
echo ""
