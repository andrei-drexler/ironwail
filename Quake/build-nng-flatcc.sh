#!/bin/bash
# Build nng and flatcc from source (wrapper script)
# This script calls the separate build scripts for nng and flatcc
#
# nng: apt has 1.7.2, but we need 2.0.x for API compatibility
# flatcc: not available in apt at all

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export DEPS_DIR="${DEPS_DIR:-$SCRIPT_DIR/dependencies}"

echo "=========================================="
echo "  Building nng and flatcc from source"
echo "=========================================="
echo ""
echo "This will build:"
echo "  1. nng v2.0.0-alpha.6 (IPC transport)"
echo "  2. flatcc v0.6.1 (FlatBuffers)"
echo ""

# Create dependencies directory
mkdir -p "$DEPS_DIR/lib" "$DEPS_DIR/include"

# Build nng
if [ -f "$SCRIPT_DIR/build-nng.sh" ]; then
    echo "Step 1/2: Building nng..."
    echo ""
    "$SCRIPT_DIR/build-nng.sh"
else
    echo "ERROR: build-nng.sh not found at $SCRIPT_DIR/build-nng.sh"
    exit 1
fi

echo ""

# Build flatcc
if [ -f "$SCRIPT_DIR/build-flatcc.sh" ]; then
    echo "Step 2/2: Building flatcc..."
    echo ""
    "$SCRIPT_DIR/build-flatcc.sh"
else
    echo "ERROR: build-flatcc.sh not found at $SCRIPT_DIR/build-flatcc.sh"
    exit 1
fi

echo ""
echo "=========================================="
echo "  Build Summary"
echo "=========================================="
echo ""
echo "Installation directory: $DEPS_DIR"
echo ""
echo "Libraries built (both static):"
echo "  ✓ nng v2.0.0-alpha.6"
echo "  ✓ flatcc v0.6.1"
echo ""
echo "Files created:"
ls -lh "$DEPS_DIR/lib/libnng.a" 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
ls -lh "$DEPS_DIR/lib/libflatccrt.a" 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
echo ""
echo "Headers installed to: $DEPS_DIR/include/"
echo ""
echo "These static libraries will be linked directly into ironwail binary."
echo ""
