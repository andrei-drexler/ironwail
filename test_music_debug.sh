#!/bin/bash

# Test script to debug music system
echo "Testing music system initialization..."

# Start the engine and capture output
./carnifex-engine -dev > engine_debug.log 2>&1 &
ENGINE_PID=$!

# Wait for engine to start
sleep 5

# Check if engine is still running
if kill -0 $ENGINE_PID 2>/dev/null; then
    echo "Engine started successfully"
    
    # Kill the engine
    kill $ENGINE_PID 2>/dev/null
    wait $ENGINE_PID 2>/dev/null
    
    echo "Checking engine output for music system messages:"
    echo "=== Music System Initialization ==="
    grep -i "bgm\|music\|extmusic\|codec" engine_debug.log || echo "No music system messages found"
    
    echo ""
    echo "=== Audio System Messages ==="
    grep -i "audio\|sound\|sdl" engine_debug.log | head -10
    
    echo ""
    echo "=== File System Messages ==="
    grep -i "basedir\|searchpath\|carnifex" engine_debug.log | head -5
    
    # Clean up
    rm -f engine_debug.log
else
    echo "Engine failed to start"
fi
