#!/bin/bash
# Download and build ALL dependencies for Ironwail headless mode
# Includes: SDL2, audio libs (vorbis/ogg/mp3), nng, and flatcc
# For use in environments without apt/package managers

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEPS_DIR="$SCRIPT_DIR/deps_downloaded"
WORK_DIR="/tmp/ironwail_deps_$$"

echo "=========================================="
echo "  Ironwail Dependencies Downloader"
echo "=========================================="
echo ""
echo "This script will download and build:"
echo "  - SDL2 2.30.0 (timing/platform)"
echo "  - libvorbis 1.3.7 (audio)"
echo "  - libogg 1.3.5 (audio)"
echo "  - mpg123 1.32.3 (audio)"
echo "  - nng 2.0.0-dev (IPC transport)"
echo "  - flatcc 0.6.2 (FlatBuffers compiler)"
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
echo "[5/6] Downloading nng (nanomsg-next-generation) 2.0.0-dev..."
echo "  NOTE: nng may already exist in $SCRIPT_DIR/nng_lib/"
if [ -f "$SCRIPT_DIR/nng_lib/libnng.so" ]; then
    echo "  ✓ Found existing nng libraries in nng_lib/ - skipping download"
else
    NNG_URL="https://github.com/nanomsg/nng/archive/refs/tags/v2.0.0-dev.tar.gz"
    if command -v wget > /dev/null; then
        wget -q -O nng-2.0.0-dev.tar.gz "$NNG_URL" || echo "  WARNING: Failed"
    elif command -v curl > /dev/null; then
        curl -L -o nng-2.0.0-dev.tar.gz "$NNG_URL" || echo "  WARNING: Failed"
    fi

    if [ -f nng-2.0.0-dev.tar.gz ]; then
        tar xzf nng-2.0.0-dev.tar.gz
        cd nng-2.0.0-dev
        mkdir -p build && cd build
        cmake -DCMAKE_INSTALL_PREFIX="$DEPS_DIR" \
              -DBUILD_SHARED_LIBS=ON \
              -DNNG_TESTS=OFF \
              -DNNG_TOOLS=OFF \
              .. 2>&1 | tail -10
        make -j4 2>&1 | tail -10
        make install
        cd ../..
        echo "  ✓ nng installed to $DEPS_DIR"
    else
        echo "  ✗ Failed to download nng"
        echo "  ERROR: nng is required for PluQ IPC"
    fi
fi

echo ""
echo "[6/6] Downloading flatcc (FlatBuffers compiler) 0.6.2..."
echo "  NOTE: flatcc runtime may already exist in $SCRIPT_DIR/nng_lib/libflatccrt.a"
if [ -f "$SCRIPT_DIR/nng_lib/libflatccrt.a" ]; then
    echo "  ✓ Found existing flatcc runtime in nng_lib/ - skipping download"
else
    FLATCC_URL="https://github.com/dvidelabs/flatcc/archive/refs/tags/v0.6.2.tar.gz"
    if command -v wget > /dev/null; then
        wget -q -O flatcc-0.6.2.tar.gz "$FLATCC_URL" || echo "  WARNING: Failed"
    elif command -v curl > /dev/null; then
        curl -L -o flatcc-0.6.2.tar.gz "$FLATCC_URL" || echo "  WARNING: Failed"
    fi

    if [ -f flatcc-0.6.2.tar.gz ]; then
        tar xzf flatcc-0.6.2.tar.gz
        cd flatcc-0.6.2
        mkdir -p build && cd build
        cmake -DCMAKE_INSTALL_PREFIX="$DEPS_DIR" \
              -DFLATCC_RTONLY=ON \
              .. 2>&1 | tail -10
        make -j4 2>&1 | tail -10
        make install
        cd ../..
        echo "  ✓ flatcc runtime installed to $DEPS_DIR"
    else
        echo "  ✗ Failed to download flatcc"
        echo "  ERROR: flatcc is required for PluQ FlatBuffers serialization"
    fi
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
echo "Existing PluQ libraries: $SCRIPT_DIR/nng_lib/"
echo ""
echo "Downloaded:"
ls -lh "$DEPS_DIR/lib/" 2>/dev/null || echo "  (none)"
echo ""
echo "Existing PluQ libs:"
ls -lh "$SCRIPT_DIR/nng_lib/" 2>/dev/null || echo "  (missing - please build nng and flatcc)"
echo ""
echo "=========================================="
echo "  How to Use"
echo "=========================================="
echo ""
echo "Method 1: Manual LD_LIBRARY_PATH"
echo "  export LD_LIBRARY_PATH=\"$DEPS_DIR/lib:$SCRIPT_DIR/nng_lib:\$LD_LIBRARY_PATH\""
echo "  ./ironwail -headless -pluq +map start"
echo ""
echo "Method 2: Use wrapper script (RECOMMENDED)"
echo "  ./run-with-downloaded-libs.sh -headless -pluq +map start"
echo ""

# Create wrapper script
cat > "$SCRIPT_DIR/run-with-downloaded-libs.sh" << 'EOFWRAPPER'
#!/bin/bash
# Run Ironwail with downloaded dependencies + existing nng/flatcc
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="$SCRIPT_DIR/deps_downloaded/lib:$SCRIPT_DIR/nng_lib:$LD_LIBRARY_PATH"
exec "$SCRIPT_DIR/ironwail" "$@"
EOFWRAPPER
chmod +x "$SCRIPT_DIR/run-with-downloaded-libs.sh"

echo "Wrapper script created: run-with-downloaded-libs.sh"
echo ""
echo "=========================================="
echo ""
echo "IMPORTANT: PluQ IPC Dependencies"
echo ""
echo "The PluQ system requires:"
echo "  - nng 2.0.0-dev (in nng_lib/libnng.so)"
echo "  - flatcc 0.6.2 runtime (in nng_lib/libflatccrt.a)"
echo ""
echo "These are ALREADY included in the repository in Quake/nng_lib/"
echo "You do NOT need to download them unless the directory is missing."
echo ""
