#!/bin/bash
# Test decoupled -headless and -pluq flags

set -e
cd "$(dirname "$0")"

echo "============================================"
echo "  Testing Decoupled Headless + PluQ"
echo "============================================"
echo ""

# Cleanup
rm -f /dev/shm/quake_pluq* 2>/dev/null || true
killall ironwail 2>/dev/null || true
sleep 1

echo "[Test 1] -headless alone (no PluQ, just headless client)"
echo "Command: ./ironwail -headless +map start"
./ironwail -headless +map start > test1.log 2>&1 &
PID1=$!
sleep 2
if ps -p $PID1 >/dev/null 2>&1; then
    echo "✅ Headless mode works standalone"
    kill $PID1 2>/dev/null || true
else
    echo "❌ Headless mode failed"
    cat test1.log
fi
wait $PID1 2>/dev/null || true
sleep 1

echo ""
echo "[Test 2] -headless -pluq together (PluQ backend)"
echo "Command: ./ironwail -headless -pluq +map start"
./ironwail -headless -pluq +map start > test2.log 2>&1 &
PID2=$!
sleep 3
if ps -p $PID2 >/dev/null 2>&1; then
    echo "✅ PluQ backend mode works"
    if ls /dev/shm/quake_pluq* >/dev/null 2>&1; then
        ls -lh /dev/shm/quake_pluq*
        echo "✅ Shared memory created"
    else
        echo "❌ No shared memory"
    fi
    kill $PID2 2>/dev/null || true
else
    echo "❌ PluQ backend failed"
    cat test2.log
fi
wait $PID2 2>/dev/null || true

echo ""
echo "[Test 3] -pluq without -headless (should error)"
echo "Command: ./ironwail -pluq +map start"
if ./ironwail -pluq +map start 2>&1 | grep -q "requires -headless"; then
    echo "✅ Correctly requires -headless flag"
else
    echo "❌ Did not check for -headless requirement"
fi

rm -f /dev/shm/quake_pluq* 2>/dev/null || true
echo ""
echo "============================================"
echo "         Tests Complete!"
echo "============================================"
