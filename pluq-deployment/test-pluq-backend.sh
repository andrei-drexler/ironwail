#!/bin/bash
# Test PluQ headless backend with -pluq-backend flag

set -e
cd "$(dirname "$0")"

echo "=== Testing PluQ Headless Backend ==="
echo ""

# Cleanup
rm -f /dev/shm/quake_pluq* 2>/dev/null || true
killall ironwail 2>/dev/null || true
sleep 1

echo "Starting headless PluQ backend..."
echo "Command: ./ironwail -pluq-backend +map start"
echo ""

# Run in background
./ironwail -pluq-backend +map start > backend.log 2>&1 &
BACKEND_PID=$!
echo "Backend PID: $BACKEND_PID"

# Wait for initialization
sleep 3

# Check if still running
if ps -p $BACKEND_PID > /dev/null 2>&1; then
    echo "✅ Backend is running!"
else
    echo "❌ Backend died"
    echo ""
    echo "=== Backend Log ==="
    cat backend.log
    exit 1
fi

# Check shared memory
echo ""
echo "Checking shared memory..."
if ls /dev/shm/quake_pluq* > /dev/null 2>&1; then
    ls -lh /dev/shm/quake_pluq*
    echo "✅ Shared memory created!"
else
    echo "❌ No shared memory found"
fi

# Show backend output
echo ""
echo "=== Backend Output (first 60 lines) ==="
head -60 backend.log

# Keep backend running for a bit
echo ""
echo "Backend running for 3 more seconds..."
sleep 3

# Cleanup
echo ""
echo "=== Cleanup ==="
kill $BACKEND_PID 2>/dev/null || true
wait $BACKEND_PID 2>/dev/null || true

echo ""
echo "✅ Test complete!"
