#!/bin/bash

# Carnifex Engine - Build Script
# Builds the modified IronWail engine for independent game development

echo "🎮 Carnifex Engine - Build Script"
echo "================================="

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "❌ Error: CMakeLists.txt not found"
    echo "   Run this script in the ironwail directory"
    exit 1
fi

echo "🔧 Building Carnifex Engine..."

# Clean previous build
echo "   Cleaning previous build..."
rm -rf CMakeFiles/ CMakeCache.txt Makefile cmake_install.cmake

# Configure with CMake
echo "   Configuring with CMake..."
cmake .

if [ $? -eq 0 ]; then
    echo "   ✅ CMake configuration successful"
    
    # Build the engine
    echo "   Building engine..."
    make -j$(nproc)
    
    if [ $? -eq 0 ]; then
        echo "   ✅ Build successful"
        
        # Executable is already named correctly
        if [ -f "carnifex-engine" ]; then
            echo "   ✅ Executable created: carnifex-engine"
        fi
        
        echo ""
        echo "🎉 Carnifex Engine built successfully!"
        echo ""
        echo "📁 Files created:"
        echo "   - carnifex-engine (main executable)"
        echo "   - CARNIFEX_CREDITS.md (credits and acknowledgments)"
        echo ""
        echo "🚀 To test the engine:"
        echo "   ./carnifex-engine"
        echo ""
        echo "📚 Features:"
        echo "   - No dependency on original Quake files"
        echo "   - Independent game development support"
        echo "   - All IronWail performance optimizations preserved"
        echo "   - All original credits maintained"
        echo ""
        echo "🎮 Ready for Carnifex game development!"
        
    else
        echo "   ❌ Build failed"
        exit 1
    fi
    
else
    echo "   ❌ CMake configuration failed"
    exit 1
fi
