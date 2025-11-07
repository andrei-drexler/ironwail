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

# Start test monitor in background
echo "Starting monitor..."
./test-monitor &
MONITOR_PID=$!

# Give monitor time to connect
sleep 1

# Start backend
echo "Starting backend..."
echo ""
../Quake/ironwail -headless -pluq +map start

# Cleanup
kill $MONITOR_PID 2>/dev/null
rm -f /tmp/quake_pluq_*

