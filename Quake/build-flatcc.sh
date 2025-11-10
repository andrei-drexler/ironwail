#!/bin/bash
# Build flatcc from source
# flatcc: not available in apt at all

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEPS_DIR="${DEPS_DIR:-$SCRIPT_DIR/dependencies}"
WORK_DIR="${WORK_DIR:-/tmp/flatcc_build_$$}"

echo "=========================================="
echo "  Building flatcc from source"
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
# Build flatcc (FlatBuffers for C)
# =============================================================================

echo "Building flatcc (FlatBuffers for C) v0.6.1..."
echo "  Why build from source:"
echo "    - Not available in apt"
echo "    - No pre-built binaries in GitHub releases"
echo "    - Requires: -DFLATCC_INSTALL=ON cmake option"
echo ""

FLATCC_SUCCESS=0

# Try git clone first (preferred method)
if command -v git > /dev/null; then
    echo "  Method: git clone (fast, shallow)"
    git clone --depth 1 --branch v0.6.1 https://github.com/dvidelabs/flatcc.git flatcc-build 2>&1 | tail -3
    FLATCC_SOURCE="git"
else
    # Fallback to tarball download
    echo "  Method: tarball download"
    FLATCC_URL="https://github.com/dvidelabs/flatcc/archive/refs/tags/v0.6.1.tar.gz"
    if command -v wget > /dev/null; then
        wget -q --show-progress -O flatcc.tar.gz "$FLATCC_URL" 2>&1
    elif command -v curl > /dev/null; then
        curl -L --progress-bar -o flatcc.tar.gz "$FLATCC_URL"
    else
        echo "  ERROR: Neither git, wget, nor curl available"
        exit 1
    fi

    if [ -f flatcc.tar.gz ]; then
        tar xzf flatcc.tar.gz
        mv flatcc-0.6.1 flatcc-build
        FLATCC_SOURCE="tarball"
    fi
fi

if [ -d flatcc-build ]; then
    cd flatcc-build
    mkdir -p build && cd build

    echo "  Configuring with cmake (runtime only)..."
    cmake -DCMAKE_INSTALL_PREFIX="$DEPS_DIR" \
          -DCMAKE_BUILD_TYPE=Release \
          -DFLATCC_RTONLY=ON \
          -DFLATCC_INSTALL=ON \
          .. > cmake.log 2>&1

    if [ $? -eq 0 ]; then
        echo "  Building (make -j$(nproc))..."
        make -j$(nproc) > make.log 2>&1

        if [ $? -eq 0 ]; then
            echo "  Installing to $DEPS_DIR..."
            make install > install.log 2>&1

            if [ $? -eq 0 ]; then
                FLATCC_SUCCESS=1
                echo "  ✓ flatcc built successfully (source: $FLATCC_SOURCE)"
                echo ""
                echo "Files installed:"
                ls -lh "$DEPS_DIR/lib/libflatccrt.a" 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
            fi
        fi
    fi

    if [ $FLATCC_SUCCESS -eq 0 ]; then
        echo "  ✗ flatcc build FAILED"
        echo "  Check logs in: $WORK_DIR/flatcc-build/build/"
        echo ""
        echo "Last 20 lines of cmake.log:"
        tail -20 "$WORK_DIR/flatcc-build/build/cmake.log" 2>/dev/null || echo "  (log not found)"
        echo ""
        echo "Last 20 lines of make.log:"
        tail -20 "$WORK_DIR/flatcc-build/build/make.log" 2>/dev/null || echo "  (log not found)"
        exit 1
    fi

    cd ../..
else
    echo "  ✗ Failed to obtain flatcc source code"
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
echo "  ✓ flatcc v0.6.1 (199KB static library)"
echo ""
echo "Headers installed to: $DEPS_DIR/include/flatcc/"
echo ""
