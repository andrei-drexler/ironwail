# Ironwail PluQ - Ready to Run

This folder contains Ironwail with the PluQ (Process-Level Universal Quake) IPC system for split frontend/backend rendering.

## Quick Start

### 1. Add Quake Content
Place your Quake game files in the `id1/` folder:
- `PAK0.PAK` (required - from Quake installation or shareware)
- `PAK1.PAK` (optional - registered version)

You can get the shareware version from: https://archive.org/details/quakesharewareversion

### 2. Run Standard Mode (Single Executable)
```bash
./ironwail
```

### 3. Run PluQ Split Mode (Separate Frontend/Backend)

**Terminal 1 - Start Backend:**
```bash
./ironwail +pluq_mode backend +pluq_headless 1
```

**Terminal 2 - Start Frontend:**
```bash
./ironwail-pluq-frontend
```

Or use the convenience scripts:
```bash
./run-backend.sh    # Terminal 1
./run-frontend.sh   # Terminal 2
```

## What's Included

- **ironwail** (1.1 MB) - Full Quake engine with PluQ support
- **ironwail-pluq-frontend** (870 KB) - Slim rendering-only client
- **ironwail.pak** - UI resources
- **id1/** - Place Quake content here (PAK files)

## PluQ Modes

The `ironwail` executable supports three PluQ modes:

1. **disabled** (default) - Normal Quake, no IPC
2. **backend** - Runs game simulation, broadcasts to shared memory
3. **frontend** - Receives world state via IPC, renders only
4. **both** - Single process with IPC loopback (testing)

## PluQ Console Commands

When running with PluQ enabled:
- `pluq_mode <backend|frontend|disabled|both>` - Set PluQ mode
- `pluq_headless <0|1>` - Headless mode for backend
- `ipc_stats` - Show IPC performance statistics
- `ipc_reset_stats` - Reset IPC counters

## Architecture

```
┌─────────────────┐         Shared Memory        ┌──────────────────┐
│  Backend        │    (/dev/shm/quake_pluq)     │  Frontend        │
│  (ironwail)     │◄──────────────────────────►  │  (slim client)   │
│                 │                               │                  │
│ • Game Logic    │   Entity Data (8192 max)     │ • Rendering      │
│ • Physics       │   Dynamic Lights (32 max)    │ • Audio          │
│ • AI            │   Frame Metadata              │ • Input          │
│ • Networking    │   <── 98KB/frame             │ • UI             │
└─────────────────┘                               └──────────────────┘
         │                                                 │
         └──────────── Input Commands ────────────────────┘
```

## Use Cases

**1. Remote Rendering**
- Backend on server, frontend on thin client
- Low latency local rendering of remote game

**2. Split Processing**
- Backend on high-performance CPU
- Frontend on GPU-optimized machine

**3. Development/Debugging**
- Inspect world state without affecting simulation
- Multiple frontends viewing same backend

**4. Recording/Replay**
- Backend can save IPC stream
- Frontend replays from saved stream

## System Requirements

- **Linux** (uses POSIX shared memory /dev/shm)
- **SDL2** libraries
- **OpenGL** support
- **16 MB** RAM minimum per process

## Troubleshooting

**"Failed to initialize PluQ"**
- Check /dev/shm is mounted and writable
- Ensure both processes run as same user

**"No backend connection"**
- Start backend first with `+pluq_mode backend`
- Check backend is running: `ps aux | grep ironwail`

**Frontend shows nothing**
- Backend must load a map: `+map start` or use console `map e1m1`
- Check backend console for errors

## Building from Source

```bash
cd Quake/
make clean && make -j4                           # Backend
make -f Makefile.pluq_frontend clean && make -f Makefile.pluq_frontend -j4  # Frontend
```

## License

Ironwail is licensed under the GPL v2. See source repository for details.

## More Information

- PluQ Documentation: See `PLUQ_FRONTEND_README.md` in source
- Ironwail Website: https://github.com/andrei-drexler/ironwail
- Original Quake: id Software (1996)
