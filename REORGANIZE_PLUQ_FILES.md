# Reorganize PluQ Files for Clarity

## Current Structure (Confusing)

```
Main Binary:
  pluq.c (532 lines)          - Main PluQ backend code
  pluq.h (124 lines)          - Public API

Frontend Binary:
  main_pluq_frontend.c        - Frontend entry point
  host_pluq_frontend.c        - Frontend Host_Init
  stubs_pluq_frontend.c       - Stubbed server functions

Generated (FlatBuffers):
  pluq_builder.h
  pluq_reader.h
  pluq_verifier.h
```

**Problems:**
1. `pluq.c` contains both backend AND some frontend code
2. Frontend code scattered across 3 files
3. Not clear what goes where
4. Naming inconsistent: `host_pluq_frontend.c` vs `pluq_frontend.c`

---

## Proposed Structure (Clear)

```
Main Binary:
  pluq.c                      - Backend IPC code ONLY
  pluq.h                      - Shared definitions

Frontend Binary:
  pluq_frontend.c             - Frontend PluQ code (receive state, send input)
  pluq_frontend.h             - Frontend-specific API
  main_pluq_frontend.c        - Frontend entry point (minimal, just main())
  stubs_pluq_frontend.c       - Server stubs (keep as-is)

Generated:
  pluq_builder.h
  pluq_reader.h
  pluq_verifier.h
```

---

## File Responsibilities

### For Main Binary

#### `pluq.c` - Backend IPC Only
```c
// What it contains:
- PluQ_Init()
- PluQ_Enable() / PluQ_Disable()
- PluQ_InitializeSockets()  // Backend sockets: REP, PUB, PULL
- PluQ_Shutdown()
- PluQ_ProcessInputCommands()    // Receive from frontend
- PluQ_BroadcastWorldState()     // Send to frontend
- Backend transport functions

// What it does NOT contain:
- Frontend socket initialization (moved to pluq_frontend.c)
- Frontend receive/apply functions (moved to pluq_frontend.c)
- Frontend input sending (moved to pluq_frontend.c)
```

#### `pluq.h` - Shared Definitions
```c
// What it contains:
- Channel URLs (tcp://127.0.0.1:900x)
- FlatBuffers includes
- Shared structs (pluq_context_t, pluq_stats_t, etc.)
- Backend API (for main binary)
- Forward declarations for frontend (if needed)

// Backend API:
void PluQ_Init(void);
void PluQ_Enable(void);
qboolean PluQ_IsEnabled(void);
void PluQ_BroadcastWorldState(void);
void PluQ_ProcessInputCommands(void);
```

---

### For Frontend Binary

#### `pluq_frontend.c` - Frontend PluQ Code (NEW)
```c
// Consolidates frontend-specific code from:
// - host_pluq_frontend.c (Host_Init logic)
// - main_pluq_frontend.c (frame loop logic)
// - pluq.c (frontend receive functions)

// What it contains:
- PluQ_Frontend_Init()           // Initialize frontend sockets (REQ, SUB, PUSH)
- PluQ_Frontend_Shutdown()
- PluQ_Frontend_ReceiveFrame()   // Receive from backend (SUB socket)
- PluQ_Frontend_ApplyState()     // Apply received state to cl.*
- PluQ_Frontend_SendInput()      // Send input to backend (PUSH socket)
- Frontend socket management
- Frontend-specific context

// Example structure:
static pluq_frontend_context_t {
    nng_socket resources_req;
    nng_socket gameplay_sub;
    nng_socket input_push;
    qboolean initialized;
} frontend_ctx;

void PluQ_Frontend_Init(void)
{
    // Connect to backend sockets
    nng_req0_open(&frontend_ctx.resources_req);
    nng_dialer_create(..., PLUQ_URL_RESOURCES);

    nng_sub0_open(&frontend_ctx.gameplay_sub);
    nng_sub0_socket_subscribe(...);
    nng_dialer_create(..., PLUQ_URL_GAMEPLAY);

    nng_push0_open(&frontend_ctx.input_push);
    nng_dialer_create(..., PLUQ_URL_INPUT);
}

void PluQ_Frontend_FrameLoop(void)
{
    // Main frontend loop
    while (1) {
        if (PluQ_Frontend_ReceiveFrame())
            PluQ_Frontend_ApplyState();

        CL_UpdateInput();
        PluQ_Frontend_SendInput();

        SCR_UpdateScreen();
    }
}
```

#### `pluq_frontend.h` - Frontend API (NEW)
```c
// Frontend-specific API
void PluQ_Frontend_Init(void);
void PluQ_Frontend_Shutdown(void);
void PluQ_Frontend_FrameLoop(void);
qboolean PluQ_Frontend_ReceiveFrame(void);
void PluQ_Frontend_ApplyState(void);
void PluQ_Frontend_SendInput(usercmd_t *cmd);
```

#### `main_pluq_frontend.c` - Minimal Entry Point
```c
// Just main() and basic setup
// Most logic moves to pluq_frontend.c

int main(int argc, char **argv)
{
    host_parms = /* ... */;

    Host_Init_Frontend();  // Slim Host_Init
    PluQ_Frontend_Init();  // PluQ frontend sockets

    PluQ_Frontend_FrameLoop();  // Main loop (from pluq_frontend.c)

    PluQ_Frontend_Shutdown();
    return 0;
}
```

#### `host_pluq_frontend.c` - Maybe Merge?
**Option A**: Keep it, but simplify (just Host_Init logic)
**Option B**: Merge into `main_pluq_frontend.c` (since it's just init)
**Option C**: Merge into `pluq_frontend.c` as `PluQ_Frontend_HostInit()`

**Recommendation**: Option C - merge into `pluq_frontend.c`

---

## Migration Plan

### Step 1: Create `pluq_frontend.c` and `pluq_frontend.h`

Extract frontend code from:
- `pluq.c` → Move `PluQ_Frontend_*` functions
- `host_pluq_frontend.c` → Move Host_Init logic
- `main_pluq_frontend.c` → Move frame loop logic

### Step 2: Update `pluq.c` (Backend Only)

Remove frontend-specific code:
- ~~`PluQ_Frontend_ReceiveFrame()`~~
- ~~`PluQ_Frontend_SendInput()`~~
- ~~`PluQ_ReceiveWorldState()` (frontend version)~~
- ~~Frontend socket initialization~~

### Step 3: Update `main_pluq_frontend.c`

Simplify to just call:
- `Host_Init_Frontend()` (or merge)
- `PluQ_Frontend_Init()`
- `PluQ_Frontend_FrameLoop()`

### Step 4: Update Makefiles

```makefile
# Makefile (main binary)
OBJS += pluq.o

# Makefile.pluq_frontend (frontend binary)
OBJS += pluq_frontend.o main_pluq_frontend.o stubs_pluq_frontend.o
```

### Step 5: Clean Up (Optional)

Consider removing/merging `host_pluq_frontend.c` since most logic is now in `pluq_frontend.c`

---

## Benefits

### 1. Clear Separation
```
pluq.c           - Backend: Broadcast state, receive input
pluq_frontend.c  - Frontend: Receive state, send input
```

### 2. Single Responsibility
- Each file has one clear purpose
- No mixing of backend/frontend code
- Easy to understand what's what

### 3. Easier Maintenance
```
// Want to change frontend behavior?
→ Edit pluq_frontend.c

// Want to change backend behavior?
→ Edit pluq.c
```

### 4. Better Naming
```
Before:
  host_pluq_frontend.c  ← Not clear it's PluQ-specific

After:
  pluq_frontend.c       ← Obviously PluQ frontend code
```

### 5. Simpler Build
```makefile
# Main binary (backend)
OBJS = pluq.o

# Frontend binary
OBJS = pluq_frontend.o main_pluq_frontend.o stubs_pluq_frontend.o
```

---

## Comparison

| Aspect | Before | After |
|--------|--------|-------|
| **Backend code** | pluq.c (mixed) | pluq.c (backend only) |
| **Frontend code** | Scattered (3 files) | pluq_frontend.c (consolidated) |
| **Entry point** | main_pluq_frontend.c (bloated) | main_pluq_frontend.c (minimal) |
| **Clarity** | Mixed responsibilities | Clear separation |
| **Files** | 4 files (confusing) | 4 files (clear purpose) |

---

## Example: Clear File Structure

```
Quake/
├── pluq.c                      Backend IPC (broadcasts, receives input)
├── pluq.h                      Shared definitions and backend API
├── pluq_frontend.c             Frontend IPC (receives, sends input)
├── pluq_frontend.h             Frontend API
├── main_pluq_frontend.c        Entry point (just main)
├── stubs_pluq_frontend.c       Server stubs
├── pluq_builder.h              Generated FlatBuffers
├── pluq_reader.h               Generated FlatBuffers
└── pluq_verifier.h             Generated FlatBuffers
```

**Reading the file list tells you exactly what each file does!**

---

## Summary

**Current Problem**: Frontend code scattered, backend/frontend mixed

**Solution**:
- `pluq.c` = Backend only
- `pluq_frontend.c` = Frontend only
- Clear separation, single responsibility

**Next Step**: Create `pluq_frontend.c` and consolidate frontend code

This matches your intuition: PluQ frontend code should be in `pluq_frontend.c`!
