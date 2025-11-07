#!/bin/bash
# Improved PluQ test script

set -e
cd "$(dirname "$0")"

echo "=== PluQ System Test v2 ==="
echo ""

# Cleanup any previous state
rm -f /dev/shm/quake_pluq* 2>/dev/null || true
killall ironwail ironwail-pluq-frontend 2>/dev/null || true
sleep 1

# Create a config file that enables PluQ backend mode
cat > test_backend.cfg <<'EOF'
pluq_mode backend
pluq_headless 1
map start
EOF

echo "Test 1: Starting backend with PluQ enabled..."
# Start backend in background with config that enables PluQ
xvfb-run -a ./ironwail +exec test_backend.cfg -dedicated &
BACKEND_PID=$!
echo "Backend PID: $BACKEND_PID"

# Wait for backend to initialize
echo "Waiting for backend to initialize..."
sleep 5

# Check if backend is running
if ! ps -p $BACKEND_PID > /dev/null 2>&1; then
    echo "❌ Backend process died"
    cat test_backend.cfg
    exit 1
fi

echo ""
echo "Test 2: Checking shared memory..."
if [ -e /dev/shm/quake_pluq* ]; then
    ls -lh /dev/shm/quake_pluq*
    echo "✅ Shared memory created!"
else
    echo "❌ Shared memory NOT found"
    echo "Checking backend output..."
    sleep 2
fi

echo ""
echo "Test 3: Checking backend status..."
ps aux | grep -E "ironwail" | grep -v grep

# Try frontend (only if shared memory exists)
if [ -e /dev/shm/quake_pluq* ]; then
    echo ""
    echo "Test 4: Starting frontend..."
    cat > test_frontend.cfg <<'EOF'
pluq_mode frontend
EOF

    timeout 5 xvfb-run -a ./ironwail-pluq-frontend +exec test_frontend.cfg 2>&1 | head -50 &
    FRONTEND_PID=$!
    sleep 3

    echo "Frontend test complete (timeout after 5s)"
else
    echo ""
    echo "Skipping frontend test (no shared memory)"
fi

echo ""
echo "=== Cleanup ==="
killall ironwail ironwail-pluq-frontend 2>/dev/null || true
rm -f test_backend.cfg test_frontend.cfg
wait 2>/dev/null || true

echo ""
echo "=== Test Complete ==="
