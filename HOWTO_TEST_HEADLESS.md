# How to Test PluQ Headless Components

This document provides step-by-step instructions for testing the headless backend and frontend components of PluQ.

## Prerequisites

### System Requirements
- Linux system with `/dev/shm` mounted (POSIX shared memory)
- SDL2 runtime libraries: `libSDL2-2.0.so.0`
- Additional libraries: `libvorbisfile.so.3`, `libogg`, `libFLAC`, `libmpg123`
- **PluQ IPC libraries** (REQUIRED):
  - **nng 2.0.0-dev** (IPC transport layer)
  - **flatcc 0.6.2** (FlatBuffers serialization)

### PluQ Dependencies (nng + flatcc)

**IMPORTANT**: The PluQ system uses nng and flatcc for IPC communication. These libraries:
- Must be installed via system packages or downloaded locally
- Are required for ALL PluQ functionality (backend and frontend)
- Will be installed to `dependencies/lib/` if using download script

```bash
# Download nng + flatcc (and other dependencies)
cd /home/user/PluQuake/Quake
./download-dependencies.sh

# Check if libraries were installed
ls -lh dependencies/lib/
# Should show:
#   libnng.so, libnng.so.1, libnng.so.2.0.0-dev
#   libflatccrt.a
```

### Install Other Dependencies

#### Option A: Using apt (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install -y \
    libsdl2-2.0-0 \
    libvorbis0a \
    libvorbisfile3 \
    libogg0 \
    libflac12 \
    libopus0 \
    libopusfile0 \
    libmpg123-0
```

#### Option B: Download and Build (No apt available)
```bash
cd /home/user/PluQuake/Quake
./download-dependencies.sh
```

This will download and build from source:
- SDL2 2.30.0
- libvorbis 1.3.7 + libogg 1.3.5
- mpg123 1.32.3
- nng v2.0.0-alpha.6 (built from source - NO pre-built binaries)
- flatcc v0.6.1 (built from source - NO pre-built binaries)

**Build requirements**: git, cmake, make, gcc

Libraries are installed to `dependencies/`.

#### Option C: Manual Download

If you have no internet access in the build environment:

**SDL2**: https://github.com/libsdl-org/SDL/releases/download/release-2.30.0/SDL2-2.30.0.tar.gz
**libvorbis**: https://downloads.xiph.org/releases/vorbis/libvorbis-1.3.7.tar.xz
**libogg**: https://downloads.xiph.org/releases/ogg/libogg-1.3.5.tar.xz
**mpg123**: https://www.mpg123.de/download/mpg123-1.32.3.tar.bz2

**⚠️ nng and flatcc must be built from source (NO pre-built binaries available):**
**nng**: https://github.com/nanomsg/nng/archive/refs/tags/v2.0.0-alpha.6.tar.gz
**flatcc**: https://github.com/dvidelabs/flatcc/archive/refs/tags/v0.6.1.tar.gz

Extract, build with cmake:
- SDL2/audio libs: `./configure && make && make install`
- nng/flatcc: `mkdir build && cd build && cmake .. && make && make install`

### Build Binaries
```bash
cd /home/user/PluQuake/Quake

# Build backend (main binary)
make clean && make -j4

# Build PluQ frontend
make -f Makefile.pluq_frontend clean && make -f Makefile.pluq_frontend -j4

# Build test frontend (headless, stdin/stdout)
make -f Makefile.pluq_test_frontend clean && make -f Makefile.pluq_test_frontend -j4
```

---

## Test 1: Headless Backend

**Purpose**: Verify backend runs without video/audio/input hardware

### What It Does
- Runs full Quake game logic (client + server)
- No window, no audio output, no local input
- Uses SDL2 for timing (`SDL_Delay()`) but skips all video/audio SDL calls
- Broadcasts game state via **nng IPC** (PUB socket on tcp://127.0.0.1:9002)
- Receives input commands via **nng IPC** (PULL socket on tcp://127.0.0.1:9003)
- Uses **FlatBuffers** (flatcc) for zero-copy message serialization

### Command Line Flags
```
-headless           Run without video/audio/input
-pluq               Enable PluQ IPC system
+map <mapname>      Load a map immediately
```

### Manual Test
```bash
cd /home/user/PluQuake/Quake

# Method 1: With downloaded dependencies
./run-with-downloaded-libs.sh -headless -pluq +map start

# Method 2: With system libraries (if apt-get install worked)
LD_LIBRARY_PATH=dependencies/lib:$LD_LIBRARY_PATH ./ironwail -headless -pluq +map start

# Method 3: Use deployment test script
cd /home/user/PluQuake/pluq-deployment
./test-headless-final.sh
```

### Expected Output
```
Console initialized.
Found SDL version 2.X.X
PluQ: Initializing nng IPC transport...
PluQ: nng version 2.0.0-dev
PluQ: Backend mode enabled
PluQ: Listening on tcp://127.0.0.1:9001 (resources REP)
PluQ: Publishing to tcp://127.0.0.1:9002 (gameplay PUB)
PluQ: Receiving from tcp://127.0.0.1:9003 (input PULL)
Loading map start...
Headless mode active - skipping video/audio/input
```

### Verification Checklist
- [ ] Backend process starts and stays running
- [ ] Console shows "PluQ: Backend mode enabled"
- [ ] Console shows nng socket bindings (tcp://127.0.0.1:9001-9003)
- [ ] Map loads successfully ("Loading map start...")
- [ ] Process uses minimal CPU when idle (no rendering overhead)
- [ ] No window appears
- [ ] No audio output

### Common Errors

**"libnng.so: cannot open shared object file"**
→ nng library missing. Check `Quake/dependencies/lib/libnng.so` exists
→ Run `./download-dependencies.sh` to download dependencies

**"libSDL2-2.0.so.0: cannot open shared object file"**
→ Install SDL2 runtime: `sudo apt-get install libsdl2-2.0-0`
→ Or run `./download-dependencies.sh` to build from source

**"libvorbisfile.so.3: cannot open shared object file"**
→ Install Vorbis: `sudo apt-get install libvorbisfile3`
→ Or use download script

**"Failed to bind nng socket"**
→ Port already in use. Kill existing backend: `killall ironwail`

**"Cannot find pak file id1/PAK0.PAK"**
→ Need Quake game data. For testing only: `mkdir -p /tmp/quake_test/id1 && touch /tmp/quake_test/id1/pak0.pak`
→ Then use `-basedir /tmp/quake_test` flag

---

## Test 2: Headless Frontend (Test Tool)

**Purpose**: Test IPC communication using stdin/stdout instead of SDL input/rendering

### What It Does
- Minimal frontend with NO window, NO audio, NO SDL input
- Reads commands from stdin (non-blocking)
- Forwards commands to backend via **nng PUSH socket** (tcp://127.0.0.1:9003)
- Receives world state from backend via **nng SUB socket** (tcp://127.0.0.1:9002)
- Uses **FlatBuffers** to deserialize game state
- Prints state updates to stdout
- **This is a TESTING TOOL, not a production frontend**

### Build
```bash
cd /home/user/PluQuake/Quake
make -f Makefile.pluq_test_frontend clean
make -f Makefile.pluq_test_frontend -j4
```

### Prerequisites
1. Backend MUST be running first (see Test 1 above)
2. Backend must be in `-headless -pluq` mode
3. Backend should have a map loaded
4. **nng and flatcc libraries must be available** (in dependencies/lib/ or system-installed)

### Manual Test
```bash
# Terminal 1: Start backend
cd /home/user/PluQuake/Quake
LD_LIBRARY_PATH=dependencies/lib ./ironwail -headless -pluq +map e1m1

# Terminal 2: Start test frontend
cd /home/user/PluQuake/Quake
LD_LIBRARY_PATH=dependencies/lib ./ironwail-pluq-test-frontend
```

### Expected Frontend Output
```
[PluQ Test Frontend] Initializing...
[PluQ Test Frontend] Connecting to backend at tcp://127.0.0.1:9001-9003
[PluQ Test Frontend] nng sockets created
[PluQ Test Frontend] Connected successfully
[PluQ Test Frontend] Type commands (e.g., 'map e1m2', 'god', 'noclip')
[PluQ Test Frontend] ← Backend: World state received (frame 1)
[PluQ Test Frontend] ← Backend: World state received (frame 60)
```

### Interactive Testing
Once running, type commands and press Enter:
```
map e1m2           (load different map)
god                (enable god mode)
noclip             (no collision)
give h 100         (set health to 100)
quit               (exit both frontend and backend)
```

### Verification Checklist
- [ ] Frontend connects to backend (no "connection refused")
- [ ] Frontend receives world state updates (frame counter increases)
- [ ] Commands typed in frontend execute on backend
- [ ] Console output shows IPC traffic
- [ ] No SDL window appears
- [ ] No audio output
- [ ] Process exits cleanly with `quit` command

### Common Errors

**"Failed to connect to backend"**
→ Backend not running. Start backend first (see Test 1)

**"Connection refused on tcp://127.0.0.1:9002"**
→ Backend not in PluQ mode. Use `-pluq` flag

**"libnng.so: cannot open shared object file"**
→ Set LD_LIBRARY_PATH: `export LD_LIBRARY_PATH=dependencies/lib:$LD_LIBRARY_PATH`

**"SDL.h: No such file or directory" (build error)**
→ Install SDL2 dev files: `sudo apt-get install libsdl2-dev`
→ Or run `./download-dependencies.sh`

**Frontend hangs on startup**
→ Backend crashed. Check backend terminal for errors

---

## Test 3: Full IPC Flow (Headless Backend + Test Frontend)

**Purpose**: Verify complete command forwarding and state broadcasting via nng + FlatBuffers

### Automated Test
```bash
cd /home/user/PluQuake/pluq-deployment
./test-frontend-headless.sh
```

### Manual Test
```bash
# Terminal 1: Backend
cd /home/user/PluQuake/Quake
LD_LIBRARY_PATH=dependencies/lib ./ironwail -headless -pluq +map start 2>&1 | tee backend.log

# Terminal 2: Test Frontend
cd /home/user/PluQuake/Quake
echo "map e1m1" | LD_LIBRARY_PATH=dependencies/lib ./ironwail-pluq-test-frontend
```

### Expected Behavior
1. Backend prints "PluQ: Backend mode enabled"
2. Backend prints "PluQ: nng version 2.0.0-dev"
3. Frontend connects and prints "Connected successfully"
4. Frontend sends `map e1m1` command (serialized via FlatBuffers)
5. Backend receives command via nng PULL socket
6. Backend deserializes FlatBuffer and loads map e1m1
7. Backend broadcasts new world state (serialized via FlatBuffers)
8. Frontend receives updated state via nng SUB socket
9. Frontend prints confirmation

### Full System Verification
- [ ] Commands flow: Frontend stdin → FlatBuffers → nng PUSH → Backend execution
- [ ] State flows: Backend game state → FlatBuffers → nng PUB → Frontend stdout
- [ ] Both processes run headless (no windows)
- [ ] CPU usage is minimal when idle
- [ ] Memory usage is stable (no leaks)
- [ ] FlatBuffers messages are compact (check `ipc_stats` command)

---

## Test 4: Production Frontend (With SDL)

**Purpose**: Test full graphical frontend with actual rendering

### Build
```bash
cd /home/user/PluQuake/Quake
make -f Makefile.pluq_frontend clean
make -f Makefile.pluq_frontend -j4
```

### Manual Test
```bash
# Terminal 1: Backend
LD_LIBRARY_PATH=dependencies/lib ./ironwail -headless -pluq +map start

# Terminal 2: Frontend (requires X11/Wayland display)
LD_LIBRARY_PATH=dependencies/lib ./ironwail-pluq-frontend
```

### Expected Behavior
- SDL window opens showing Quake gameplay
- Game renders 3D graphics normally
- Input (keyboard/mouse) works
- Audio output works
- All input forwarded to backend via nng IPC
- Backend sends 60fps world state updates via nng PUB socket
- FlatBuffers used for efficient serialization

---

## Debugging Tips

### Check nng/flatcc Libraries
```bash
# Verify nng library exists
ls -lh Quake/dependencies/lib/libnng.so*
# Expected: libnng.so, libnng.so.1, libnng.so.2.0.0-dev

# Verify flatcc runtime exists
ls -lh Quake/dependencies/lib/libflatccrt.a
# Expected: libflatccrt.a (static library, ~253KB)

# Check nng version in binary
strings Quake/dependencies/lib/libnng.so | grep "2.0.0"
```

### Check IPC Connections
```bash
# List nng sockets (backend should show 3 listening sockets)
ss -tlnp | grep ironwail

# Expected output:
# tcp   LISTEN 0.0.0.0:9001  (REQ/REP - resources)
# tcp   LISTEN 0.0.0.0:9002  (PUB/SUB - gameplay)
# tcp   LISTEN 0.0.0.0:9003  (PUSH/PULL - input)
```

### Monitor IPC Traffic
```bash
cd /home/user/PluQuake/pluq-deployment
LD_LIBRARY_PATH=../Quake/dependencies/lib ./test-monitor     # Shows live nng message flow
```

### View Backend Console Output
```bash
# Run backend with output to file
LD_LIBRARY_PATH=dependencies/lib ./ironwail -headless -pluq +map start 2>&1 | tee backend.log

# In another terminal, watch output
tail -f backend.log
```

### Check for Memory Leaks
```bash
# Run under valgrind (slow but thorough)
LD_LIBRARY_PATH=dependencies/lib valgrind --leak-check=full ./ironwail -headless -pluq +map start

# Let it run for 60 seconds, then quit
# Check for "definitely lost" or "indirectly lost" bytes
```

### Verify FlatBuffers Schema
```bash
# Check generated FlatBuffers headers
ls -lh Quake/pluq_builder.h
ls -lh Quake/flatbuffers_common_builder.h

# These are auto-generated from pluq.fbs schema
```

---

## Architecture Notes

### Why Headless Backend Uses SDL2

The `-headless` flag skips SDL video/audio/input calls but **still initializes SDL**:
- `SDL_Init(0)` - Initialize library (no subsystems)
- `SDL_Delay()` - Frame rate limiting (used in main loop)
- `SDL_GetVersion()` - Version detection
- `SDL_Quit()` - Cleanup at exit

This means headless backend REQUIRES `libSDL2-2.0.so.0` even though no window appears.

### PluQ IPC Channels (nng + FlatBuffers)

| Channel | Socket Type | Port | Purpose | Message Format |
|---------|-------------|------|---------|----------------|
| Resources | REQ/REP | 9001 | Asset requests (textures, models) | FlatBuffers ResourceRequest/Response |
| Gameplay | PUB/SUB | 9002 | World state broadcasts (60fps) | FlatBuffers FrameUpdate |
| Input | PUSH/PULL | 9003 | Command forwarding (console, input) | FlatBuffers InputCommand |

**nng** provides the transport layer (TCP sockets, pattern matching).
**FlatBuffers** provides zero-copy serialization (compact binary format).

### Why nng 2.0 and flatcc 0.6.2?

- **nng 2.0.0-dev**: Latest nng with improved performance, smaller library size
- **flatcc 0.6.2**: Lightweight FlatBuffers implementation in pure C (no C++ dependency)
- Both libraries can be installed locally to `dependencies/lib/` or system-wide

### Files Modified for Headless Support

Core changes (minimized to 38 lines):
- `host.c` - 4x `if (!Host_IsHeadless())` checks
- `cl_main.c` - Apply IPC input instead of local input
- `cmd.c` - Forward console commands via IPC (FlatBuffers)
- `main_sdl.c` - Set `isDedicated` flag for headless mode

All PluQ logic encapsulated in:
- `pluq.c` / `pluq.h` - Backend IPC (nng + FlatBuffers)
- `pluq_frontend.c` / `pluq_frontend.h` - Frontend IPC (nng + FlatBuffers)

---

## Success Criteria

All tests pass when:
1. ✅ Headless backend runs without SDL window/audio
2. ✅ Headless backend broadcasts game state via nng PUB socket
3. ✅ Test frontend connects via nng SUB socket and receives state
4. ✅ Console commands flow from frontend to backend via nng PUSH/PULL
5. ✅ FlatBuffers messages serialize/deserialize correctly
6. ✅ Production frontend renders graphics normally
7. ✅ No memory leaks detected
8. ✅ No crashes during 5-minute stress test
9. ✅ nng and flatcc libraries load correctly

---

## Quick Reference

```bash
# Build everything
cd /home/user/PluQuake/Quake
make -j4                                              # Backend
make -f Makefile.pluq_frontend -j4                    # Frontend
make -f Makefile.pluq_test_frontend -j4               # Test frontend

# Download dependencies (if needed)
./download-dependencies.sh

# Test headless backend (nng + flatcc required)
LD_LIBRARY_PATH=dependencies/lib ./ironwail -headless -pluq +map start

# Test with test frontend
# Terminal 1:
LD_LIBRARY_PATH=dependencies/lib ./ironwail -headless -pluq +map e1m1
# Terminal 2:
LD_LIBRARY_PATH=dependencies/lib ./ironwail-pluq-test-frontend

# Test with production frontend
# Terminal 1:
LD_LIBRARY_PATH=dependencies/lib ./ironwail -headless -pluq +map e1m1
# Terminal 2:
LD_LIBRARY_PATH=dependencies/lib ./ironwail-pluq-frontend

# Use wrapper script (automatically sets LD_LIBRARY_PATH)
./run-with-downloaded-libs.sh -headless -pluq +map start
```

## Library Locations

**PluQ IPC libraries** (REQUIRED, already in repo):
- `/home/user/PluQuake/Quake/dependencies/lib/libnng.so*`
- `/home/user/PluQuake/Quake/dependencies/lib/libflatccrt.a`

**Downloaded dependencies** (optional, built by download-dependencies.sh):
- `/home/user/PluQuake/Quake/dependencies/lib/libSDL2*.so*`
- `/home/user/PluQuake/Quake/dependencies/lib/libvorbis*.so*`
- `/home/user/PluQuake/Quake/dependencies/lib/libogg*.so*`
- `/home/user/PluQuake/Quake/dependencies/lib/libmpg123*.so*`
