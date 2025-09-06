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
rm -rf build-artifacts/CMakeFiles/ build-artifacts/CMakeCache.txt build-artifacts/Makefile build-artifacts/cmake_install.cmake build-artifacts/carnifex-engine

# Check for available compilers
echo "   Checking available compilers..."
CLANG_AVAILABLE=false
GCC_AVAILABLE=false

if command -v clang >/dev/null 2>&1; then
    CLANG_AVAILABLE=true
    echo "   ✅ Clang found: $(clang --version | head -n1)"
fi

if command -v gcc >/dev/null 2>&1; then
    GCC_AVAILABLE=true
    echo "   ✅ GCC found: $(gcc --version | head -n1)"
fi

# Configure with CMake using Clang as default, GCC as fallback
echo "   Configuring with CMake..."
if [ "$CLANG_AVAILABLE" = true ]; then
    echo "   🔨 Using Clang as primary compiler"
    cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -B build-artifacts .
    CMAKE_RESULT=$?
    
    if [ $CMAKE_RESULT -ne 0 ] && [ "$GCC_AVAILABLE" = true ]; then
        echo "   ⚠️  Clang configuration failed, falling back to GCC"
        echo "   🔨 Using GCC as fallback compiler"
        cmake -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -B build-artifacts .
        CMAKE_RESULT=$?
    fi
elif [ "$GCC_AVAILABLE" = true ]; then
    echo "   🔨 Using GCC (Clang not available)"
    cmake -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -B build-artifacts .
    CMAKE_RESULT=$?
else
    echo "   ❌ No suitable compiler found (neither Clang nor GCC)"
    exit 1
fi

if [ $CMAKE_RESULT -eq 0 ]; then
    echo "   ✅ CMake configuration successful"
    
    # Build the engine
    echo "   Building engine..."
    make -C build-artifacts -j$(nproc)
    
    if [ $? -eq 0 ]; then
        echo "   ✅ Build successful"
        
        # Move executable to build-artifacts directory
        if [ -f "carnifex-engine" ]; then
            mv carnifex-engine build-artifacts/
            echo "   ✅ Executable created: build-artifacts/carnifex-engine"
        fi
        
        echo ""
        echo "🎉 Carnifex Engine built successfully!"
        echo ""
        echo "📁 Files created:"
        echo "   - build-artifacts/carnifex-engine (main executable)"
        echo "   - docs/CARNIFEX_CREDITS.md (credits and acknowledgments)"
        echo ""
        echo "🔧 Compiler used:"
        if [ "$CLANG_AVAILABLE" = true ] && [ $CMAKE_RESULT -eq 0 ]; then
            echo "   - Clang (primary compiler)"
        elif [ "$GCC_AVAILABLE" = true ]; then
            echo "   - GCC (fallback compiler)"
        fi
        echo ""
        echo "🚀 To test the engine:"
        echo "   ./build-artifacts/carnifex-engine"
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
