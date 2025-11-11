#!/bin/bash
# Ironwail PluQ Frontend Launcher
# Runs the slim rendering client that connects to backend via PluQ IPC

echo "========================================"
echo "Starting Ironwail PluQ Frontend"
echo "========================================"
echo ""
echo "Frontend will:"
echo "  - Receive world state from backend via shared memory"
echo "  - Render graphics and play audio"
echo "  - Send input to backend"
echo ""
echo "Make sure backend is running first!"
echo "  (Start with ./run-backend.sh in another terminal)"
echo ""
echo "Press Ctrl+C or use Quake menu to quit"
echo "========================================"
echo ""

# Check if backend is running
if ! pgrep -f "ironwail.*pluq_mode backend" > /dev/null; then
    echo "WARNING: Backend doesn't appear to be running!"
    echo "Start it first with: ./run-backend.sh"
    echo ""
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Run frontend
# The frontend automatically connects to PluQ shared memory
./ironwail-pluq-frontend
