#!/bin/bash
# Ironwail PluQ Backend Launcher
# Runs the backend in headless mode, broadcasting world state via PluQ IPC

echo "========================================"
echo "Starting Ironwail PluQ Backend"
echo "========================================"
echo ""
echo "Backend will:"
echo "  - Run game simulation (physics, AI, networking)"
echo "  - Broadcast world state to shared memory"
echo "  - Run headless (no rendering)"
echo ""
echo "After backend starts, launch frontend with:"
echo "  ./run-frontend.sh"
echo ""
echo "Press Ctrl+C to stop backend"
echo "========================================"
echo ""

# Run backend in headless PluQ mode
# +pluq_mode backend = Enable PluQ backend mode
# +pluq_headless 1 = No rendering (headless)
# +map start = Auto-load start map (optional)
./ironwail +pluq_mode backend +pluq_headless 1 +map start
