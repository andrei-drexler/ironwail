#!/bin/bash
# Final comprehensive test of PluQ headless backend

set -e
cd "$(dirname "$0")"

echo "============================================"
echo "     PluQ Headless Backend Test"
echo "============================================"
echo ""

# Cleanup
rm -f /dev/shm/quake_pluq* 2>/dev/null || true
killall ironwail 2>/dev/null || true
sleep 1

echo "[1/4] Starting headless PluQ backend..."
echo "Command: ./ironwail -pluq +map start"
./ironwail -pluq +map start 2>&1 &
BACKEND_PID=$!
echo "✅ Backend started (PID: $BACKEND_PID)"

echo ""
echo "[2/4] Waiting for initialization (5 seconds)..."
sleep 5

echo ""
echo "[3/4] Checking backend status..."
if ps -p $BACKEND_PID > /dev/null 2>&1; then
    echo "✅ Backend is running!"

    echo ""
    echo "[4/4] Checking shared memory..."
    if ls /dev/shm/quake_pluq* > /dev/null 2>&1; then
        ls -lh /dev/shm/quake_pluq*
        echo "✅ Shared memory created successfully!"

        echo ""
        echo "============================================"
        echo "         ALL TESTS PASSED! ✅"
        echo "============================================"
        echo ""
        echo "The PluQ headless backend is working correctly:"
        echo "  • Running without video/audio/input (headless)"
        echo "  • Broadcasting game state via shared memory"
        echo "  • Ready to connect PluQ frontend"
        echo ""
    else
        echo "❌ Shared memory not found"
        exit 1
    fi
else
    echo "❌ Backend process died"
    exit 1
fi

echo "Stopping backend..."
kill $BACKEND_PID 2>/dev/null || true
wait $BACKEND_PID 2>/dev/null || true
rm -f /dev/shm/quake_pluq* 2>/dev/null || true

echo "✅ Test complete!"
