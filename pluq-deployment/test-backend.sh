#!/bin/bash
# Test PluQ backend mode with nng+FlatBuffers

cd "$(dirname "$0")"

echo "=== PluQ Backend Test ==="
echo "Starting headless backend with PluQ IPC..."
echo "This will broadcast game state via nng (PUB/SUB)"
echo ""

# Run backend in headless mode
../Quake/ironwail -headless -pluq +map start

