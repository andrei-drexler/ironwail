# Ironwail PluQ Frontend

## Overview

**Ironwail PluQ Frontend** is a separate executable that acts as a slim client for rendering and input handling. It communicates with the Ironwail backend via **PluQ** (a shared memory-based IPC system) for remote rendering capabilities.

## What is PluQ?

**PluQ** (pronounced "plug") is the Inter-Process Communication system designed specifically for Ironwail to enable frontend/backend separation. It uses shared memory for high-performance, zero-copy data transfer between processes.

## Architecture

### Frontend Responsibilities (Ironwail PluQ Frontend)
- **Rendering**: All OpenGL rendering, particle systems, visual effects
- **Input**: Keyboard, mouse, and controller input handling
- **Audio**: Sound and music playback
- **UI**: Menu system, console, HUD rendering

### Excluded from Frontend (Handled by Backend)
- **Game Simulation**: QuakeC VM, entity logic, physics
- **Level Loading**: Map parsing, BSP processing
- **Networking**: Network protocol handling
- **Server**: Game server logic

### Communication via PluQ

The frontend communicates with the backend through shared memory:
- **Backend → Frontend**: World state (entities, player state, game data)
- **Frontend → Backend**: Input commands (movement, view angles, buttons, impulses)

## Files

### New PluQ Frontend Files
- `main_pluq_frontend.c` - Frontend-specific entry point with branding
- `host_pluq_frontend.c` - Frontend-specific host initialization
- `stubs_pluq_frontend.c` - Stub implementations for excluded backend functions
- `Makefile.pluq_frontend` - Build configuration for frontend executable

### PluQ System Files
- `pluq.h` - PluQ IPC system header
- `pluq.c` - PluQ IPC system implementation

### Shared Files from Backend
The frontend includes many files from the backend codebase but excludes:
- Server files: `sv_*.c`
- Network files: `net_dgrm.c`, `net_loop.c`, `net_main.c`
- QuakeC VM files: `pr_*.c`
- World physics: `world.c`
- Main host: `host.c` (replaced with `host_pluq_frontend.c`)

## Building

### Requirements
- SDL2 development libraries
- OpenGL development libraries
- Standard build tools (gcc, make)
- Audio codec libraries (optional: FLAC, MP3, Vorbis)

### Build Commands

```bash
cd Quake
make -f Makefile.pluq_frontend
```

For debug build:
```bash
make -f Makefile.pluq_frontend DEBUG=1
```

Clean build:
```bash
make -f Makefile.pluq_frontend clean
```

### Output

The build produces an `ironwail-pluq-frontend` executable.

## Usage

### Running the System

1. **Start the Backend** (with headless mode):
   ```bash
   ./ironwail +pluq_mode backend +pluq_headless 1 +map e1m1
   ```

2. **Start the PluQ Frontend**:
   ```bash
   ./ironwail-pluq-frontend
   ```

### Backend Options

- `pluq_mode backend` - Enable backend mode (simulation + PluQ broadcasting)
- `pluq_headless 1` - Disable rendering/input on backend (optional)
- `+map <mapname>` - Start a map

### PluQ Frontend Behavior

- Connects automatically to backend via shared memory
- Receives world state updates every frame
- Sends input to backend
- Renders the world based on received state

## Console Commands

### PluQ System Commands (available in both backend and frontend)

- `pluq_mode <disabled|backend|frontend|both>` - Set PluQ operation mode
- `ipc_stats` - Display PluQ performance statistics
- `ipc_reset_stats` - Reset PluQ performance counters

### PluQ CVars

- `pluq_headless` - Run backend without rendering/input (0=off, 1=on)

## Implementation Details

### PluQ Frontend Initialization

1. Minimal host initialization (no server, network, or QuakeC VM)
2. Full rendering and input initialization
3. PluQ initialization in frontend mode
4. Set connection state to allow receiving data

### PluQ Frontend Frame Loop

1. Process input events
2. Accumulate input commands
3. Send input to backend via PluQ (`PluQ_SendInput`)
4. Receive world state from backend via PluQ (`PluQ_ReceiveWorldState`)
5. Apply received state to local rendering structures (`PluQ_ApplyReceivedState`)
6. Render frame
7. Update audio

### Stub Functions

Functions that are part of the excluded backend but may be referenced by client code are stubbed in `stubs_pluq_frontend.c`. These include:
- Server functions (SV_*)
- Network functions (NET_*)
- QuakeC VM functions (PR_*)
- Model loading functions (Mod_*)

Console commands that require server functionality print a message indicating they're not available in PluQ frontend mode.

## Benefits of PluQ Architecture

1. **Separation of Concerns**: Clear separation between simulation and presentation
2. **Network Transparency**: PluQ architecture could be adapted for network-based remote rendering
3. **Testing**: Easier to test rendering independently of game logic
4. **Performance**: Frontend can run on different hardware than backend
5. **Development**: Rendering changes don't require recompiling server code
6. **Multiple Frontends**: Multiple frontend instances can connect to one backend

## PluQ Protocol Details

See `pluq.h` and `pluq.c` for full implementation details. The PluQ system uses:
- POSIX shared memory on Unix/Linux
- Windows file mapping on Windows
- Atomic operations for synchronization
- Zero-copy entity transfer
- Approximately 8192 entities maximum
- Minimal latency (~microseconds for local communication)

### Shared Memory Layout

```c
typedef struct {
    volatile uint32_t frame_sequence;      // Frame counter
    volatile uint32_t write_in_progress;   // Synchronization flag
    pluq_frame_header_t header;            // Frame metadata
    entity_t entities[PLUQ_MAX_ENTITIES];  // Entity array
    dlight_t dlights[MAX_DLIGHTS];         // Dynamic lights
    pluq_input_cmd_t input_cmd;            // Input from frontend
    volatile uint32_t input_ready;         // Input ready flag
} pluq_shared_memory_t;
```

## Limitations

- PluQ Frontend cannot load maps or start games (backend controls this)
- Console commands that affect server state are disabled
- Save/load functionality requires backend
- Demo playback requires backend
- Currently limited to local (same machine) communication

## Future Enhancements

Potential improvements:
- Network-based PluQ (TCP/UDP) for true remote rendering
- Compressed state updates for lower bandwidth
- Multiple frontend instances with different views
- Frontend-specific graphics settings independent of backend
- PluQ relay/proxy for internet-based remote play
- Delta compression for entity updates
- Predictive rendering to hide latency

## Troubleshooting

### Frontend can't connect to backend

- Ensure backend is running with `pluq_mode backend`
- Check that both processes are running as the same user
- Verify shared memory is available (`/dev/shm` on Linux)

### Performance issues

- Check `ipc_stats` for performance metrics
- Ensure backend is running at stable framerate
- Consider using `pluq_headless 1` on backend to reduce load

### Rendering artifacts

- PluQ Frontend displays state from backend - check backend console for errors
- Ensure backend has successfully loaded the map
- Verify entity count is within `PLUQ_MAX_ENTITIES` limit (8192)

## Development

### Adding PluQ Support to New Systems

To add PluQ support to other systems:

1. Include `pluq.h`
2. Call `PluQ_Init()` during initialization
3. Call `PluQ_Initialize(PLUQ_MODE_FRONTEND)` or `PLUQ_MODE_BACKEND`
4. Backend: Call `PluQ_BroadcastWorldState()` each frame
5. Frontend: Call `PluQ_ReceiveWorldState()` and `PluQ_ApplyReceivedState()` each frame

### Extending PluQ Protocol

To add new data to PluQ:

1. Update `pluq_shared_memory_t` in `pluq.h`
2. Update `PluQ_BroadcastWorldState()` to populate new data
3. Update `PluQ_ApplyReceivedState()` to consume new data
4. Ensure structure alignment and size considerations

## Credits

- PluQ IPC System: QuakeSpasm/Ironwail developers
- Ironwail PluQ Frontend: Based on QuakeSpasm and Ironwail architecture
- Original Quake: id Software

## License

GPL v2 - See source files for full license text
