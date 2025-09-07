#!/bin/bash

# Test script to check if music is working
echo "Testing music functionality..."

# Start the engine in the background
./carnifex-engine -dev > engine_output.log 2>&1 &
ENGINE_PID=$!

# Wait a moment for the engine to start
sleep 3

# Check if the engine is still running
if kill -0 $ENGINE_PID 2>/dev/null; then
    echo "Engine started successfully"
    
    # Try to send music command via console
    echo "music track02" | nc -U /tmp/quake_console 2>/dev/null || echo "Console not available"
    
    # Wait a bit more
    sleep 2
    
    # Kill the engine
    kill $ENGINE_PID 2>/dev/null
    wait $ENGINE_PID 2>/dev/null
    
    echo "Engine stopped"
    
    # Check the output for music-related messages
    echo "Checking engine output for music messages:"
    grep -i "music\|track\|audio" engine_output.log || echo "No music-related messages found"
    
    # Clean up
    rm -f engine_output.log
else
    echo "Engine failed to start"
fi
