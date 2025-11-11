#!/bin/bash
# Test PluQ Input Command Processing

cd "$(dirname "$0")"

echo "=== PluQ Input Command Test ==="
echo "This test will:"
echo "1. Start the backend (headless Quake)"
echo "2. Send console commands via test-command"
echo "3. Verify backend receives and processes them"
echo ""

# Clean up any existing IPC sockets
rm -f /tmp/quake_pluq_*

# Set library path
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="$SCRIPT_DIR/../Quake/nng_lib:$LD_LIBRARY_PATH"

# Start backend in background with output to log
echo "Starting backend..."
"$SCRIPT_DIR/ironwail" -basedir "$SCRIPT_DIR" -headless -pluq +map start > backend.log 2>&1 &
BACKEND_PID=$!

echo "Backend PID: $BACKEND_PID"

# Give backend time to initialize
echo "Waiting for backend to initialize..."
sleep 3

# Check if backend is still running
if ! kill -0 $BACKEND_PID 2>/dev/null; then
    echo "ERROR: Backend failed to start!"
    cat backend.log
    exit 1
fi

echo "Backend started successfully"
echo ""

# Send test commands
echo "=== Sending Test Commands ==="
echo ""

echo "Test 1: Sending 'version' command..."
./test-command "version"
sleep 0.5

echo ""
echo "Test 2: Sending 'status' command..."
./test-command "status"
sleep 0.5

echo ""
echo "Test 3: Sending 'sv_gravity 200' command..."
./test-command "sv_gravity 200"
sleep 0.5

echo ""
echo "Test 4: Sending 'map e1m1' command..."
./test-command "map e1m1"
sleep 2

echo ""
echo "=== Backend Output ==="
echo "Checking backend.log for received commands..."
echo ""

# Show backend output (filter for PluQ messages)
grep -i "pluq.*received command" backend.log || echo "No command receipts found in log"

echo ""
echo "=== Full Backend Log (last 50 lines) ==="
tail -50 backend.log

# Cleanup
echo ""
echo "Stopping backend..."
kill $BACKEND_PID 2>/dev/null
wait $BACKEND_PID 2>/dev/null

rm -f /tmp/quake_pluq_*

echo ""
echo "Test complete!"
