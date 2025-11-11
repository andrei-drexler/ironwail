#!/bin/bash
# Test script for headless PluQ frontend (IPC testing)

cd "$(dirname "$0")"/..

echo "==========================================="
echo "PluQ Headless Frontend Test"
echo "==========================================="
echo ""
echo "This tests the full IPC flow:"
echo "  Frontend (stdin) → IPC → Backend → IPC → Frontend (stdout)"
echo ""
echo "Prerequisites:"
echo "  1. Backend must be running:"
echo "     ./ironwail -headless -pluq +map e1m1"
echo ""
echo "Starting headless test frontend..."
echo ""

cd Quake

# Run test frontend
./ironwail-pluq-test-frontend
