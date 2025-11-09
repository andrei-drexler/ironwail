# How to Test PluQ Headless Components

This document provides step-by-step instructions for testing the headless backend and frontend components of PluQ.

## Prerequisites

### System Requirements
- Linux system with `/dev/shm` mounted (POSIX shared memory)
- SDL2 runtime libraries: `libSDL2-2.0.so.0`
- Additional libraries: `libvorbisfile.so.3`, `libogg`, `libFLAC`, etc.
- nng library (included in `Quake/nng_lib/`)
- FlatCC runtime (included in `Quake/nng_lib/`)

### Install Dependencies (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install -y \
    libsdl2-2.0-0 \
    libvorbis0a \
    libvorbisfile3 \
    libogg0 \
    libflac12 \
    libopus0 \
    libopusfile0
```

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
- Broadcasts game state via nng IPC (PUB socket on tcp://127.0.0.1:9002)
- Receives input commands via nng IPC (PULL socket on tcp://127.0.0.1:9003)

### Command Line Flags
```
-headless           Run without video/audio/input
-pluq               Enable PluQ IPC system
+map <mapname>      Load a map immediately
```

### Manual Test
```bash
cd /home/user/PluQuake/Quake

# Method 1: Combined flags
./ironwail -headless -pluq +map start

# Method 2: Use deployment test script
cd /home/user/PluQuake/pluq-deployment
./test-headless-final.sh
```

### Expected Output
```
Console initialized.
Found SDL version 2.X.X
PluQ: Initializing IPC transport (nng 2.0)...
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

**"libSDL2-2.0.so.0: cannot open shared object file"**
→ Install SDL2 runtime: `sudo apt-get install libsdl2-2.0-0`

**"libvorbisfile.so.3: cannot open shared object file"**
→ Install Vorbis: `sudo apt-get install libvorbisfile3`

**"Failed to bind nng socket"**
→ Port already in use. Kill existing backend: `killall ironwail`

**"Cannot find pak file id1/PAK0.PAK"**
→ This is OK for testing (game won't render properly but backend runs)

---

## Test 2: Headless Frontend (Test Tool)

**Purpose**: Test IPC communication using stdin/stdout instead of SDL input/rendering

### What It Does
- Minimal frontend with NO window, NO audio, NO SDL input
- Reads commands from stdin (non-blocking)
- Forwards commands to backend via nng IPC (PUSH socket)
- Receives world state from backend via nng IPC (SUB socket)
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

### Manual Test
```bash
# Terminal 1: Start backend
cd /home/user/PluQuake/Quake
./ironwail -headless -pluq +map e1m1

# Terminal 2: Start test frontend
cd /home/user/PluQuake/Quake
./ironwail-pluq-test-frontend
```

### Expected Frontend Output
```
[PluQ Test Frontend] Initializing...
[PluQ Test Frontend] Connecting to backend at tcp://127.0.0.1:9001-9003
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

**"SDL.h: No such file or directory" (build error)**
→ Install SDL2 dev files: `sudo apt-get install libsdl2-dev`

**Frontend hangs on startup**
→ Backend crashed. Check backend terminal for errors

---

## Test 3: Full IPC Flow (Headless Backend + Test Frontend)

**Purpose**: Verify complete command forwarding and state broadcasting

### Automated Test
```bash
cd /home/user/PluQuake/pluq-deployment
./test-frontend-headless.sh
```

### Manual Test
```bash
# Terminal 1: Backend
cd /home/user/PluQuake/Quake
./ironwail -headless -pluq +map start 2>&1 | tee backend.log

# Terminal 2: Test Frontend
cd /home/user/PluQuake/Quake
echo "map e1m1" | ./ironwail-pluq-test-frontend
```

### Expected Behavior
1. Backend prints "PluQ: Backend mode enabled"
2. Frontend connects and prints "Connected successfully"
3. Frontend sends `map e1m1` command
4. Backend receives command via IPC
5. Backend loads map e1m1
6. Backend broadcasts new world state
7. Frontend receives updated state
8. Frontend prints confirmation

### Full System Verification
- [ ] Commands flow: Frontend stdin → IPC → Backend execution
- [ ] State flows: Backend game state → IPC → Frontend stdout
- [ ] Both processes run headless (no windows)
- [ ] CPU usage is minimal when idle
- [ ] Memory usage is stable (no leaks)

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
./ironwail -headless -pluq +map start

# Terminal 2: Frontend (requires X11/Wayland display)
./ironwail-pluq-frontend
```

### Expected Behavior
- SDL window opens showing Quake gameplay
- Game renders 3D graphics normally
- Input (keyboard/mouse) works
- Audio output works
- All input forwarded to backend via IPC
- Backend sends 60fps world state updates

---

## Debugging Tips

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
./test-monitor     # Shows live IPC message flow
```

### View Backend Console Output
```bash
# Run backend with output to file
./ironwail -headless -pluq +map start 2>&1 | tee backend.log

# In another terminal, watch output
tail -f backend.log
```

### Check for Memory Leaks
```bash
# Run under valgrind (slow but thorough)
valgrind --leak-check=full ./ironwail -headless -pluq +map start

# Let it run for 60 seconds, then quit
# Check for "definitely lost" or "indirectly lost" bytes
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

### PluQ IPC Channels

| Channel | Socket Type | Port | Purpose |
|---------|-------------|------|---------|
| Resources | REQ/REP | 9001 | Asset requests (textures, models) |
| Gameplay | PUB/SUB | 9002 | World state broadcasts (60fps) |
| Input | PUSH/PULL | 9003 | Command forwarding (console, input) |

### Files Modified for Headless Support

Core changes (minimized to 38 lines):
- `host.c` - 4x `if (!Host_IsHeadless())` checks
- `cl_main.c` - Apply IPC input instead of local input
- `cmd.c` - Forward console commands via IPC
- `main_sdl.c` - Set `isDedicated` flag for headless mode

All PluQ logic encapsulated in:
- `pluq.c` / `pluq.h` - Backend IPC
- `pluq_frontend.c` / `pluq_frontend.h` - Frontend IPC

---

## Success Criteria

All tests pass when:
1. ✅ Headless backend runs without SDL window/audio
2. ✅ Headless backend broadcasts game state via nng
3. ✅ Test frontend connects and receives state
4. ✅ Console commands flow from frontend to backend
5. ✅ Production frontend renders graphics normally
6. ✅ No memory leaks detected
7. ✅ No crashes during 5-minute stress test

---

## Quick Reference

```bash
# Build everything
cd /home/user/PluQuake/Quake
make -j4                                              # Backend
make -f Makefile.pluq_frontend -j4                    # Frontend
make -f Makefile.pluq_test_frontend -j4               # Test frontend

# Test headless backend
./ironwail -headless -pluq +map start

# Test with test frontend
# Terminal 1:
./ironwail -headless -pluq +map e1m1
# Terminal 2:
./ironwail-pluq-test-frontend

# Test with production frontend
# Terminal 1:
./ironwail -headless -pluq +map e1m1
# Terminal 2:
./ironwail-pluq-frontend
```
