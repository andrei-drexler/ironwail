#!/bin/bash
# Resolve ALL dependencies for Ironwail/PluQ
# Strategy:
#   1. Use apt packages when available (fastest)
#   2. Download and build from source as fallback
#   3. Always build nng 2.0 and flatcc from source (special script)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEPS_DIR="$SCRIPT_DIR/dependencies"
WORK_DIR="/tmp/ironwail_deps_$$"

echo "=========================================="
echo "  Ironwail Dependencies Resolver"
echo "=========================================="
echo ""

# Check if we have apt
HAS_APT=0
if command -v apt-get > /dev/null 2>&1; then
    HAS_APT=1
    echo "Package manager: apt-get (will prefer system packages)"
else
    echo "Package manager: NONE (will build from source)"
fi
echo ""

# Create directories
mkdir -p "$DEPS_DIR/lib" "$DEPS_DIR/include" "$WORK_DIR"

# =============================================================================
# Step 1: Install available system packages via apt
# =============================================================================

if [ $HAS_APT -eq 1 ]; then
    echo "=========================================="
    echo "  Step 1: Installing system packages"
    echo "=========================================="
    echo ""

    # Check if we have sudo
    SUDO=""
    if [ "$EUID" -ne 0 ]; then
        if command -v sudo > /dev/null 2>&1; then
            SUDO="sudo"
            echo "Using sudo for package installation..."
        else
            echo "WARNING: Not running as root and sudo not available"
            echo "         System package installation will be skipped"
            HAS_APT=0
        fi
    fi

    if [ $HAS_APT -eq 1 ]; then
        echo "Installing development packages..."
        echo ""

        PACKAGES="libsdl2-dev libvorbis-dev libogg-dev libmpg123-dev"

        # Try to install, but don't fail if some packages unavailable
        echo "Running apt-get update..."
        $SUDO apt-get update -qq 2>&1 | tail -3 || true

        for pkg in $PACKAGES; do
            echo -n "  - $pkg ... "
            if dpkg -l 2>/dev/null | grep -q "^ii  $pkg"; then
                echo "already installed"
            elif $SUDO apt-get install -y $pkg > /dev/null 2>&1; then
                echo "✓ installed"
            else
                echo "✗ failed (will build from source)"
            fi
        done

        echo ""
        echo "System packages install complete"
        echo ""
    fi
else
    echo "Skipping Step 1: No apt available"
    echo ""
fi

# =============================================================================
# Step 2: Build missing dependencies from source
# =============================================================================

echo "=========================================="
echo "  Step 2: Building missing dependencies"
echo "=========================================="
echo ""

cd "$WORK_DIR"

# Check what's missing
NEED_SDL2=0
NEED_VORBIS=0
NEED_OGG=0
NEED_MPG123=0

# Check for SDL2
if ! pkg-config --exists sdl2 2>/dev/null && ! command -v sdl2-config > /dev/null 2>&1; then
    NEED_SDL2=1
    echo "SDL2: Not found in system - will build"
else
    echo "SDL2: Found in system - using system package"
fi

# Check for vorbis
if ! pkg-config --exists vorbisfile 2>/dev/null; then
    NEED_VORBIS=1
    echo "libvorbis: Not found in system - will build"
else
    echo "libvorbis: Found in system - using system package"
fi

# Check for ogg
if ! pkg-config --exists ogg 2>/dev/null; then
    NEED_OGG=1
    echo "libogg: Not found in system - will build"
else
    echo "libogg: Found in system - using system package"
fi

# Check for mpg123
if ! pkg-config --exists libmpg123 2>/dev/null; then
    NEED_MPG123=1
    echo "mpg123: Not found in system - will build"
else
    echo "mpg123: Found in system - using system package"
fi

echo ""

# Build SDL2 if needed
if [ $NEED_SDL2 -eq 1 ]; then
    echo "[Building SDL2 2.30.0...]"
    SDL2_URL="https://github.com/libsdl-org/SDL/releases/download/release-2.30.0/SDL2-2.30.0.tar.gz"

    if command -v wget > /dev/null; then
        wget -q --show-progress -O SDL2.tar.gz "$SDL2_URL" 2>&1 || echo "wget failed"
    elif command -v curl > /dev/null; then
        curl -L --progress-bar -o SDL2.tar.gz "$SDL2_URL" || echo "curl failed"
    fi

    if [ -f SDL2.tar.gz ]; then
        tar xzf SDL2.tar.gz
        cd SDL2-2.30.0
        echo "  Configuring..."
        ./configure --prefix="$DEPS_DIR" > /dev/null 2>&1
        echo "  Building..."
        make -j$(nproc) > /dev/null 2>&1
        echo "  Installing..."
        make install > /dev/null 2>&1
        cd ..
        echo "  ✓ SDL2 built and installed"
    else
        echo "  ✗ SDL2 download failed"
    fi
    echo ""
fi

# Build libogg if needed (required before vorbis)
if [ $NEED_OGG -eq 1 ]; then
    echo "[Building libogg 1.3.5...]"
    OGG_URL="https://downloads.xiph.org/releases/ogg/libogg-1.3.5.tar.xz"

    if command -v wget > /dev/null; then
        wget -q --show-progress -O libogg.tar.xz "$OGG_URL" 2>&1 || echo "wget failed"
    elif command -v curl > /dev/null; then
        curl -L --progress-bar -o libogg.tar.xz "$OGG_URL" || echo "curl failed"
    fi

    if [ -f libogg.tar.xz ]; then
        tar xJf libogg.tar.xz
        cd libogg-1.3.5
        echo "  Configuring..."
        ./configure --prefix="$DEPS_DIR" > /dev/null 2>&1
        echo "  Building..."
        make -j$(nproc) > /dev/null 2>&1
        echo "  Installing..."
        make install > /dev/null 2>&1
        cd ..
        echo "  ✓ libogg built and installed"
    else
        echo "  ✗ libogg download failed"
    fi
    echo ""
fi

# Build libvorbis if needed
if [ $NEED_VORBIS -eq 1 ]; then
    echo "[Building libvorbis 1.3.7...]"
    VORBIS_URL="https://downloads.xiph.org/releases/vorbis/libvorbis-1.3.7.tar.xz"

    if command -v wget > /dev/null; then
        wget -q --show-progress -O libvorbis.tar.xz "$VORBIS_URL" 2>&1 || echo "wget failed"
    elif command -v curl > /dev/null; then
        curl -L --progress-bar -o libvorbis.tar.xz "$VORBIS_URL" || echo "curl failed"
    fi

    if [ -f libvorbis.tar.xz ]; then
        tar xJf libvorbis.tar.xz
        cd libvorbis-1.3.7
        echo "  Configuring..."
        PKG_CONFIG_PATH="$DEPS_DIR/lib/pkgconfig:$PKG_CONFIG_PATH" \
            ./configure --prefix="$DEPS_DIR" > /dev/null 2>&1
        echo "  Building..."
        make -j$(nproc) > /dev/null 2>&1
        echo "  Installing..."
        make install > /dev/null 2>&1
        cd ..
        echo "  ✓ libvorbis built and installed"
    else
        echo "  ✗ libvorbis download failed"
    fi
    echo ""
fi

# Build mpg123 if needed
if [ $NEED_MPG123 -eq 1 ]; then
    echo "[Building mpg123 1.32.3...]"
    MPG123_URL="https://www.mpg123.de/download/mpg123-1.32.3.tar.bz2"

    if command -v wget > /dev/null; then
        wget -q --show-progress -O mpg123.tar.bz2 "$MPG123_URL" 2>&1 || echo "wget failed"
    elif command -v curl > /dev/null; then
        curl -L --progress-bar -o mpg123.tar.bz2 "$MPG123_URL" || echo "curl failed"
    fi

    if [ -f mpg123.tar.bz2 ]; then
        tar xjf mpg123.tar.bz2
        cd mpg123-1.32.3
        echo "  Configuring..."
        ./configure --prefix="$DEPS_DIR" > /dev/null 2>&1
        echo "  Building..."
        make -j$(nproc) > /dev/null 2>&1
        echo "  Installing..."
        make install > /dev/null 2>&1
        cd ..
        echo "  ✓ mpg123 built and installed"
    else
        echo "  ✗ mpg123 download failed"
    fi
    echo ""
fi

# Cleanup
cd /
rm -rf "$WORK_DIR"

# =============================================================================
# Step 3: Build nng 2.0 and flatcc (always from source)
# =============================================================================

echo "=========================================="
echo "  Step 3: Building PluQ IPC dependencies"
echo "=========================================="
echo ""
echo "nng and flatcc are ALWAYS built from source:"
echo "  - nng: apt has 1.7.2, but PluQ code requires 2.0.x API"
echo "  - flatcc: not available in apt"
echo ""

# Export DEPS_DIR and WORK_DIR for the build script
export DEPS_DIR
export WORK_DIR

# Call the specialized build script
if [ -f "$SCRIPT_DIR/build-nng-flatcc.sh" ]; then
    "$SCRIPT_DIR/build-nng-flatcc.sh"
else
    echo "ERROR: build-nng-flatcc.sh not found"
    echo "Expected at: $SCRIPT_DIR/build-nng-flatcc.sh"
    exit 1
fi

# =============================================================================
# Final Summary and Wrapper Script
# =============================================================================

echo ""
echo "=========================================="
echo "  Dependency Resolution Complete"
echo "=========================================="
echo ""
echo "Dependencies directory: $DEPS_DIR"
echo ""

# Check what was installed
echo "Installed libraries:"
if [ -d "$DEPS_DIR/lib" ] && [ "$(ls -A $DEPS_DIR/lib 2>/dev/null)" ]; then
    ls -lh "$DEPS_DIR/lib/" 2>/dev/null | grep -E "\.so|\.a" | awk '{print "  " $9 " (" $5 ")"}'
else
    echo "  (using system packages only)"
fi
echo ""

# Create wrapper script
cat > "$SCRIPT_DIR/run-with-downloaded-libs.sh" << 'EOFWRAPPER'
#!/bin/bash
# Run Ironwail with locally-built dependencies
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="$SCRIPT_DIR/dependencies/lib:$LD_LIBRARY_PATH"
export PKG_CONFIG_PATH="$SCRIPT_DIR/dependencies/lib/pkgconfig:$PKG_CONFIG_PATH"
exec "$SCRIPT_DIR/ironwail" "$@"
EOFWRAPPER
chmod +x "$SCRIPT_DIR/run-with-downloaded-libs.sh"

echo "Runtime wrapper created: run-with-downloaded-libs.sh"
echo ""
echo "=========================================="
echo "  Next Steps"
echo "=========================================="
echo ""
echo "1. Build Ironwail:"
echo "   cd $SCRIPT_DIR"
echo "   make clean && make -j$(nproc)"
echo ""
echo "2. Run (if using local dependencies):"
echo "   ./run-with-downloaded-libs.sh -headless -pluq +map start"
echo ""
echo "3. Or set LD_LIBRARY_PATH manually:"
echo "   export LD_LIBRARY_PATH=$DEPS_DIR/lib:\$LD_LIBRARY_PATH"
echo "   ./ironwail -headless -pluq +map start"
echo ""
