#!/bin/bash
# Download and extract runtime dependencies for Ironwail headless mode
# For use in environments without apt/package managers

set -e

DEPS_DIR="$(dirname "$0")/deps_downloaded"
WORK_DIR="/tmp/ironwail_deps_$$"

echo "=========================================="
echo "  Ironwail Dependencies Downloader"
echo "=========================================="
echo ""

# Create directories
mkdir -p "$DEPS_DIR/lib" "$DEPS_DIR/include" "$WORK_DIR"
cd "$WORK_DIR"

echo "[1/4] Downloading SDL2 runtime..."
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
echo "[2/4] Downloading libvorbis..."
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
echo "[3/4] Downloading libogg..."
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
echo "[4/4] Downloading mpg123..."
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

# Cleanup
cd /
rm -rf "$WORK_DIR"

echo ""
echo "=========================================="
echo "  Installation Summary"
echo "=========================================="
echo ""
echo "Libraries installed to: $DEPS_DIR"
echo ""
ls -lh "$DEPS_DIR/lib/" 2>/dev/null || echo "No libraries found"
echo ""
echo "To use these libraries, run:"
echo "  export LD_LIBRARY_PATH=\"$DEPS_DIR/lib:\$LD_LIBRARY_PATH\""
echo "  ./ironwail -headless -pluq +map start"
echo ""
echo "Or use the wrapper script:"
echo "  ./run-with-downloaded-libs.sh -headless -pluq +map start"
echo ""

# Create wrapper script
cat > "$(dirname "$0")/run-with-downloaded-libs.sh" << 'EOFWRAPPER'
#!/bin/bash
# Run Ironwail with downloaded dependencies
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="$SCRIPT_DIR/deps_downloaded/lib:$SCRIPT_DIR/nng_lib:$LD_LIBRARY_PATH"
exec "$SCRIPT_DIR/ironwail" "$@"
EOFWRAPPER
chmod +x "$(dirname "$0")/run-with-downloaded-libs.sh"

echo "=========================================="
echo ""
echo "NOTE: If downloads failed (no internet access),"
echo "you can use the stub libraries instead:"
echo "  ./run-with-stubs.sh -headless -pluq +map start"
echo ""
echo "The stub libraries provide minimal functionality"
echo "sufficient for headless backend testing."
echo ""
