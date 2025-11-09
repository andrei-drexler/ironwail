# Build Instructions for PluQ Ironwail

## Overview

The build system now supports both **local dependencies** (in `dependencies/`) and **system-installed libraries**, with automatic detection.

## Quick Start

### Option 1: Use System Libraries (Ubuntu/Debian)

```bash
# Install all dependencies
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    libsdl2-dev \
    libvorbis-dev \
    libogg-dev \
    libmpg123-dev \
    libflac-dev

# Build
cd Quake
make clean
make -j4                                    # Backend
make -f Makefile.pluq_frontend -j4          # Frontend
make -f Makefile.pluq_test_frontend -j4     # Test frontend
```

### Option 2: Download and Build Dependencies Locally

If you don't have `apt` or prefer local dependencies:

```bash
cd Quake
./download-dependencies.sh  # Downloads SDL2, audio libs, nng, flatcc
make clean
make -j4
```

Libraries will be installed to `dependencies/` and the Makefile will auto-detect them.

### Option 3: Mix of Local and System

The build system automatically detects what's available:

```bash
# Install some dependencies via system
sudo apt-get install libsdl2-dev

# Download others locally (if needed)
./download-dependencies.sh

# Build will use system SDL2 and local nng/flatcc
make -j4
```

## Dependency Detection Logic

The `Makefile.deps` file (included by main `Makefile`) detects dependencies in this order:

### SDL2
1. Check `dependencies/lib/libSDL2*` (local)
2. Try `sdl2-config` (system)
3. Try `pkg-config sdl2` (system)
4. Fallback to `-lSDL2` (assume in system path)

### nng + flatcc (PluQ IPC)
1. Check `nng_lib/` (local, **already in repo**)
2. Check `dependencies/lib/libnng*` (local)
3. Try `pkg-config nng` (system)
4. Fallback to `-lnng -lflatccrt` (assume in system path)

### Audio Libraries
1. Check `dependencies/lib/` (local)
2. Use system libraries (via existing Makefile logic)

## What's Already Included

**nng and flatcc are ALREADY in the repository** at `Quake/nng_lib/`:
- `libnng.so` (557KB) - IPC transport
- `libflatccrt.a` (253KB) - FlatBuffers runtime

You do NOT need to download these unless the directory is missing.

## Build Output

The Makefile will print dependency detection info:

```
[DEPS] Using local nng from nng_lib/
[DEPS] Using system SDL2 via sdl2-config
[DEPS] ===== Dependency Summary =====
[DEPS] SDL2: Found
[DEPS] nng:  Found
[DEPS] ==============================
```

## Troubleshooting

### "SDL.h: No such file or directory"

SDL2 not found. Solutions:
1. Install: `sudo apt-get install libsdl2-dev`
2. Download: `./download-dependencies.sh`
3. Check `dependencies/lib/` has libSDL2 files

### "libnng.so: cannot open shared object file" (runtime error)

Set `LD_LIBRARY_PATH`:

```bash
# If using local nng_lib
export LD_LIBRARY_PATH=$PWD/nng_lib:$LD_LIBRARY_PATH
./ironwail -headless -pluq +map start

# Or use wrapper script
./run-with-downloaded-libs.sh -headless -pluq +map start
```

### "libSDL2-2.0.so.0: cannot open shared object file" (runtime error)

Install SDL2 runtime:
```bash
sudo apt-get install libsdl2-2.0-0
```

Or if you built locally:
```bash
export LD_LIBRARY_PATH=$PWD/dependencies/lib:$LD_LIBRARY_PATH
```

## Files Modified

**Minimal changes to keep close to upstream Ironwail:**

1. **`Makefile`**:
   - Added `-include Makefile.deps` at top
   - Removed hardcoded SDL configuration (now in Makefile.deps)
   - Removed hardcoded PluQ lib paths (now in Makefile.deps)

2. **`Makefile.deps`** (NEW):
   - Automatic detection of local vs system libraries
   - Fallback logic for multiple detection methods
   - Adds appropriate `-I` and `-L` flags to CFLAGS/LIBS

3. **`Makefile.pluq_frontend`**:
   - Also includes Makefile.deps
   - Same detection logic

4. **`Makefile.pluq_test_frontend`**:
   - Also includes Makefile.deps
   - Same detection logic

## Philosophy

- **Minimal changes** to original Ironwail Makefiles
- **Auto-detection** of dependencies (no manual configuration)
- **Flexible**: works with system libs, local libs, or mix
- **Clear feedback**: prints what it finds
- **Fallback**: tries multiple methods before giving up

## Running Tests

See [HOWTO_TEST_HEADLESS.md](HOWTO_TEST_HEADLESS.md) for comprehensive testing instructions.

```bash
# Quick test (requires dependencies)
cd Quake
./ironwail -headless -pluq +map start  # Backend
# (In another terminal)
./ironwail-pluq-test-frontend          # Test frontend
```

## Build Targets

| Target | Command | Output |
|--------|---------|--------|
| Backend | `make` | `ironwail` (1.1MB) |
| Frontend | `make -f Makefile.pluq_frontend` | `ironwail-pluq-frontend` (870KB) |
| Test Frontend | `make -f Makefile.pluq_test_frontend` | `ironwail-pluq-test-frontend` |
| Clean | `make clean` | Removes *.o, *.d, binaries |

## Environment Variables

| Variable | Purpose | Example |
|----------|---------|---------|
| `DEBUG=1` | Build with debug symbols | `make DEBUG=1` |
| `USE_SDL2=1` | Use SDL2 (default) | Already set in Makefile |
| `USE_CURL=1` | Enable curl support | Set in Makefile (can disable if libcurl missing) |
| `LD_LIBRARY_PATH` | Runtime library path | `export LD_LIBRARY_PATH=nng_lib:dependencies/lib` |

## Summary

The build system is now **flexible and automatic**:
- Works with system-installed libraries (via `apt-get`)
- Works with locally-downloaded libraries (via `./download-dependencies.sh`)
- Works with a mix of both
- Minimal changes to original Makefiles
- Clear error messages and detection feedback
