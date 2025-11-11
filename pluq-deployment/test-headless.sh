#!/bin/bash
# Test headless PluQ backend

set -e
cd "$(dirname "$0")"

echo "=== Testing Headless PluQ Backend ==="
echo ""

# Clean up
rm -f /dev/shm/quake_pluq* 2>/dev/null || true
killall ironwail 2>/dev/null || true
sleep 1

echo "Starting headless backend..."
./ironwail -pluq-headless +map start > headless.log 2>&1 &
BACKEND_PID=$!
echo "Backend PID: $BACKEND_PID"

# Wait for initialization
sleep 3

# Check if still running
if ps -p $BACKEND_PID > /dev/null 2>&1; then
    echo "✅ Backend is running"
else
    echo "❌ Backend died"
    cat headless.log
    exit 1
fi

# Check shared memory
echo ""
echo "Checking shared memory..."
if ls /dev/shm/quake_pluq* 2>/dev/null; then
    echo "✅ Shared memory created!"
else
    echo "❌ No shared memory found"
fi

# Show backend output
echo ""
echo "=== Backend Output ==="
head -50 headless.log

# Cleanup
echo ""
echo "=== Cleanup ==="
kill $BACKEND_PID 2>/dev/null || true
wait $BACKEND_PID 2>/dev/null || true

echo ""
echo "Test complete"
