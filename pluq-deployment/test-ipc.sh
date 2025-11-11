#!/bin/bash
# Test PluQ IPC: Backend + Monitor

cd "$(dirname "$0")"

echo "=== PluQ IPC Test ==="
echo "This test will:"
echo "1. Start the backend (headless Quake broadcasting frames)"
echo "2. Start the monitor (receives and displays frame data)"
echo ""
echo "Press Ctrl+C in the monitor window to stop both processes"
echo ""

# Clean up any existing IPC sockets
rm -f /tmp/quake_pluq_*

# Get script directory for paths
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Note: No LD_LIBRARY_PATH needed - nng and flatcc are statically linked

# Start backend in background (it creates the IPC endpoint)
echo "Starting backend..."
"$SCRIPT_DIR/../Quake/ironwail" -basedir "$SCRIPT_DIR" -headless -pluq +map start &
BACKEND_PID=$!

# Give backend time to initialize and create IPC endpoints
sleep 2

# Start test monitor
echo "Starting monitor..."
echo ""
./test-monitor

# Cleanup
kill $BACKEND_PID 2>/dev/null
rm -f /tmp/quake_pluq_*

