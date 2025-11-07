#!/bin/bash
# Test script for PluQ backend and frontend

set -e

cd "$(dirname "$0")"

echo "=== PluQ System Test ==="
echo ""

# Test 1: Backend startup test
echo "Test 1: Starting backend in PluQ mode..."
timeout 5 xvfb-run -a ./ironwail +pluq_mode 1 +pluq_auto_broadcast 1 +map start 2>&1 | head -50 &
BACKEND_PID=$!
sleep 2

# Check if shared memory was created
echo ""
echo "Test 2: Checking shared memory..."
if [ -e /dev/shm/quake_pluq ]; then
    ls -lh /dev/shm/quake_pluq
    echo "✅ Shared memory created successfully"
else
    echo "❌ Shared memory NOT found"
fi

# Test 3: Frontend connection test
echo ""
echo "Test 3: Starting frontend (will connect to backend)..."
timeout 3 xvfb-run -a ./ironwail-pluq-frontend +pluq_mode 2 2>&1 | head -30 &
FRONTEND_PID=$!

sleep 2

echo ""
echo "Test 4: Checking processes..."
ps aux | grep -E "(ironwail|pluq)" | grep -v grep || echo "Processes terminated"

# Cleanup
echo ""
echo "Cleaning up..."
killall ironwail ironwail-pluq-frontend 2>/dev/null || true
wait 2>/dev/null || true

echo ""
echo "=== Test Complete ==="
