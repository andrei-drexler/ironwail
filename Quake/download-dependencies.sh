#!/bin/bash
# Download and build ALL dependencies for Ironwail headless mode
# Includes: SDL2, audio libs (vorbis/ogg/mp3), nng, and flatcc
# For use in environments without apt/package managers

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEPS_DIR="$SCRIPT_DIR/dependencies"
WORK_DIR="/tmp/ironwail_deps_$$"

echo "=========================================="
echo "  Ironwail Dependencies Builder"
echo "=========================================="
echo ""
echo "This script will download and BUILD from source:"
echo "  - SDL2 2.30.0 (timing/platform)"
echo "  - libvorbis 1.3.7 (audio)"
echo "  - libogg 1.3.5 (audio)"
echo "  - mpg123 1.32.3 (audio)"
echo "  - nng v2.0.0-alpha.6 (IPC transport) *built from source*"
echo "  - flatcc v0.6.1 (FlatBuffers) *built from source*"
echo ""
echo "NOTE: nng and flatcc have NO pre-built binaries available."
echo "      They will be built using cmake and make."
echo ""

# Create directories
mkdir -p "$DEPS_DIR/lib" "$DEPS_DIR/include" "$WORK_DIR"
cd "$WORK_DIR"

echo "[1/6] Downloading SDL2 runtime..."
# Try multiple sources for SDL2
SDL2_URL="https://github.com/libsdl-org/SDL/releases/download/release-2.30.0/SDL2-2.30.0.tar.gz"
echo "  Source: GitHub releases"
echo "  URL: $SDL2_URL"
if command -v wget > /dev/null; then
    wget -q "$SDL2_URL" || echo "  WARNING: wget failed, trying curl..."
fi
if [ ! -f SDL2-2.30.0.tar.gz ] && command -v curl > /dev/null; then
    curl -L -o SDL2-2.30.0.tar.gz "$SDL2_URL" || echo "  WARNING: curl failed"
fi

if [ -f SDL2-2.30.0.tar.gz ]; then
    echo "  Extracting..."
    tar xzf SDL2-2.30.0.tar.gz
    echo "  Building minimal SDL2..."
    cd SDL2-2.30.0
    ./configure --prefix="$DEPS_DIR" --disable-video-x11 --disable-video-wayland \
        --disable-audio --disable-joystick --disable-haptic 2>&1 | tail -20
    make -j4 2>&1 | tail -20
    make install
    cd ..
    echo "  ✓ SDL2 installed to $DEPS_DIR"
else
    echo "  ✗ Failed to download SDL2"
    echo ""
    echo "ALTERNATIVE: Use system packages or manual download"
    echo "  Ubuntu/Debian: apt-get install libsdl2-2.0-0 libsdl2-dev"
    echo "  Manual: Download from https://www.libsdl.org/download-2.0.php"
fi

echo ""
echo "[2/6] Downloading libvorbis..."
VORBIS_URL="https://downloads.xiph.org/releases/vorbis/libvorbis-1.3.7.tar.xz"
if command -v wget > /dev/null; then
    wget -q "$VORBIS_URL" || echo "  WARNING: Failed"
fi
if [ -f libvorbis-1.3.7.tar.xz ]; then
    tar xf libvorbis-1.3.7.tar.xz
    cd libvorbis-1.3.7
    ./configure --prefix="$DEPS_DIR" 2>&1 | tail -10
    make -j4 2>&1 | tail -10
    make install
    cd ..
    echo "  ✓ libvorbis installed"
else
    echo "  ✗ Failed to download libvorbis"
fi

echo ""
echo "[3/6] Downloading libogg..."
OGG_URL="https://downloads.xiph.org/releases/ogg/libogg-1.3.5.tar.xz"
if command -v wget > /dev/null; then
    wget -q "$OGG_URL" || echo "  WARNING: Failed"
fi
if [ -f libogg-1.3.5.tar.xz ]; then
    tar xf libogg-1.3.5.tar.xz
    cd libogg-1.3.5
    ./configure --prefix="$DEPS_DIR" 2>&1 | tail -10
    make -j4 2>&1 | tail -10
    make install
    cd ..
    echo "  ✓ libogg installed"
else
    echo "  ✗ Failed to download libogg"
fi

echo ""
echo "[4/6] Downloading mpg123..."
MPG123_URL="https://www.mpg123.de/download/mpg123-1.32.3.tar.bz2"
if command -v wget > /dev/null; then
    wget -q "$MPG123_URL" || echo "  WARNING: Failed"
fi
if [ -f mpg123-1.32.3.tar.bz2 ]; then
    tar xf mpg123-1.32.3.tar.bz2
    cd mpg123-1.32.3
    ./configure --prefix="$DEPS_DIR" 2>&1 | tail -10
    make -j4 2>&1 | tail -10
    make install
    cd ..
    echo "  ✓ mpg123 installed"
else
    echo "  ✗ Failed to download mpg123"
fi

echo ""
echo "[5/6] Building nng (nanomsg-next-generation) from source..."
echo "  Using: v2.0.0-alpha.6 (latest 2.0 pre-release)"
echo "  NOTE: No pre-built binaries available - building from source"

# Try git clone first (preferred method)
if command -v git > /dev/null; then
    echo "  Cloning from GitHub..."
    git clone --depth 1 --branch v2.0.0-alpha.6 https://github.com/nanomsg/nng.git nng-build 2>&1 | tail -3
    NNG_SOURCE="git"
else
    # Fallback to tarball download
    echo "  Downloading source tarball..."
    NNG_URL="https://github.com/nanomsg/nng/archive/refs/tags/v2.0.0-alpha.6.tar.gz"
    if command -v wget > /dev/null; then
        wget -q -O nng.tar.gz "$NNG_URL" || echo "  WARNING: Failed"
    elif command -v curl > /dev/null; then
        curl -sL -o nng.tar.gz "$NNG_URL" || echo "  WARNING: Failed"
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
    echo "  Running cmake..."
    cmake -DCMAKE_INSTALL_PREFIX="$DEPS_DIR" \
          -DBUILD_SHARED_LIBS=ON \
          -DNNG_TESTS=OFF \
          -DNNG_TOOLS=OFF \
          .. 2>&1 | tail -5
    echo "  Building with make -j4..."
    make -j4 2>&1 | tail -5
    echo "  Installing..."
    make install 2>&1 | tail -3
    cd ../..
    echo "  ✓ nng built and installed to $DEPS_DIR (source: $NNG_SOURCE)"
else
    echo "  ✗ Failed to obtain nng source code"
    echo "  ERROR: nng is required for PluQ IPC"
    echo "  Try: sudo apt-get install git cmake build-essential"
fi

echo ""
echo "[6/6] Building flatcc (FlatBuffers for C) from source..."
echo "  Using: v0.6.1 (latest stable release)"
echo "  NOTE: No pre-built binaries available - building from source"

# Try git clone first (preferred method)
if command -v git > /dev/null; then
    echo "  Cloning from GitHub..."
    git clone --depth 1 --branch v0.6.1 https://github.com/dvidelabs/flatcc.git flatcc-build 2>&1 | tail -3
    FLATCC_SOURCE="git"
else
    # Fallback to tarball download
    echo "  Downloading source tarball..."
    FLATCC_URL="https://github.com/dvidelabs/flatcc/archive/refs/tags/v0.6.1.tar.gz"
    if command -v wget > /dev/null; then
        wget -q -O flatcc.tar.gz "$FLATCC_URL" || echo "  WARNING: Failed"
    elif command -v curl > /dev/null; then
        curl -sL -o flatcc.tar.gz "$FLATCC_URL" || echo "  WARNING: Failed"
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
    echo "  Running cmake (runtime only)..."
    cmake -DCMAKE_INSTALL_PREFIX="$DEPS_DIR" \
          -DFLATCC_RTONLY=ON \
          .. 2>&1 | tail -5
    echo "  Building with make -j4..."
    make -j4 2>&1 | tail -5
    echo "  Installing..."
    make install 2>&1 | tail -3
    cd ../..
    echo "  ✓ flatcc built and installed to $DEPS_DIR (source: $FLATCC_SOURCE)"
else
    echo "  ✗ Failed to obtain flatcc source code"
    echo "  ERROR: flatcc is required for PluQ FlatBuffers serialization"
    echo "  Try: sudo apt-get install git cmake build-essential"
fi

# Cleanup
cd /
rm -rf "$WORK_DIR"

echo ""
echo "=========================================="
echo "  Installation Summary"
echo "=========================================="
echo ""
echo "Downloaded libraries: $DEPS_DIR"
echo ""
echo "Contents:"
ls -lh "$DEPS_DIR/lib/" 2>/dev/null || echo "  (none)"
echo ""
echo "=========================================="
echo "  How to Use"
echo "=========================================="
echo ""
echo "Method 1: Manual LD_LIBRARY_PATH"
echo "  export LD_LIBRARY_PATH=\"$DEPS_DIR/lib:\$LD_LIBRARY_PATH\""
echo "  ./ironwail -headless -pluq +map start"
echo ""
echo "Method 2: Use wrapper script (RECOMMENDED)"
echo "  ./run-with-downloaded-libs.sh -headless -pluq +map start"
echo ""

# Create wrapper script
cat > "$SCRIPT_DIR/run-with-downloaded-libs.sh" << 'EOFWRAPPER'
#!/bin/bash
# Run Ironwail with downloaded dependencies
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="$SCRIPT_DIR/dependencies/lib:$LD_LIBRARY_PATH"
exec "$SCRIPT_DIR/ironwail" "$@"
EOFWRAPPER
chmod +x "$SCRIPT_DIR/run-with-downloaded-libs.sh"

echo "Wrapper script created: run-with-downloaded-libs.sh"
echo ""
