# Corrected PluQ Architecture: Separate Binaries

## ❌ PROBLEM: Current Implementation

Currently has **5 `IsFrontend()` checks in shared code**:
- `host.c`: 3 checks
- `cl_main.c`: 2 checks

This defeats the purpose of having separate binaries!

---

## ✅ CORRECT Architecture: Two Separate Binaries

### Binary 1: ironwail (Main Binary)

**Entry Point**: `main_sdl.c`
**Modes**:
- **Normal**: Standard Quake (no flags)
- **Backend**: `-headless -pluq` (headless + IPC broadcasting)

**Code Paths**:
```c
// host.c - ONLY backend checks, NO frontend checks
void _Host_Frame(double time)
{
    // Skip local input in headless mode
    if (!IsHeadless())  // ✅ OK - headless is generic
    {
        Key_UpdateForDest();
        IN_UpdateInputMode();
        Sys_SendKeyEvents();
    }

    // Process IPC input (backend only)
    if (IsBackend())  // ✅ OK - this binary can be backend
        PluQ_ProcessInputCommands();

    // Run server (always in main binary)
    if (sv.active)  // ✅ No frontend check needed!
        Host_ServerFrame();

    // Broadcast state (backend only)
    if (IsBackend())  // ✅ OK
        PluQ_BroadcastWorldState();

    // Skip rendering in headless
    if (!IsHeadless())  // ✅ OK
        SCR_UpdateScreen();
}
```

**Checks Allowed**:
- ✅ `IsHeadless()` - Generic feature
- ✅ `IsBackend()` - This binary can be backend
- ❌ `IsFrontend()` - NEVER! Frontend is separate binary

---

### Binary 2: ironwail-frontend (Frontend Binary)

**Entry Point**: `main_pluq_frontend.c`
**Mode**: Frontend only (always)

**Initialization**: `host_pluq_frontend.c`
```c
void Host_Init_Frontend(void)
{
    // Simplified init - no server, no progs
    COM_Init();
    Host_InitLocal();
    CL_Init();       // Client rendering only
    VID_Init();
    IN_Init();

    // SKIP these (stubbed in stubs_pluq_frontend.c):
    // PR_Init()     - No QuakeC
    // Mod_Init()    - No model loading
    // NET_Init()    - No networking
    // SV_Init()     - No server

    PluQ_SetMode(PLUQ_MODE_FRONTEND);  // Always frontend
}
```

**Frame Loop**: Uses regular `_Host_Frame()` but:
- `sv.active` is always false (no server)
- Always receives state from backend
- Always sends input to backend
- No need for `IsFrontend()` checks!

**Stubbed Functions**: `stubs_pluq_frontend.c`
```c
void SV_Frame(void) { }           // Server doesn't run
void NET_SendPacket(...) { }      // No networking
void PR_ExecuteProgram(...) { }   // No QuakeC VM
```

---

## 🔧 Required Changes

### 1. Remove IsFrontend() checks from host.c

**Current (WRONG)**:
```c
// Line 1277
if (PluQ_IsFrontend())
    /* ... */

// Line 1285
if (sv.active && !PluQ_IsFrontend())
    Host_ServerFrame();

// Line 1300
if (PluQ_IsFrontend())
{
    if (PluQ_ReceiveWorldState())
        PluQ_ApplyReceivedState();
}
```

**Corrected**:
```c
// Remove line 1277 check - not needed

// Line 1285 - No frontend check needed!
if (sv.active)  // Frontend binary has sv.active=false always
    Host_ServerFrame();

// Line 1300 - Frontend uses different main loop!
// This code path never executes in frontend binary
// Remove the check entirely
```

### 2. Remove IsFrontend() checks from cl_main.c

**Current (WRONG)**:
```c
// Line 838
if (PluQ_IsFrontend())
    PluQ_SendInput(&cmd);
else
    CL_SendMove(&cmd);

// Line 845
if (!PluQ_IsFrontend())
    /* ... */
```

**Corrected**: Frontend should override `CL_SendMove()` in its build:
```c
// In main binary (cl_main.c):
void CL_SendCmd(void)
{
    // Backend mode doesn't send to network
    if (IsBackend())
        return;  // Input comes from IPC

    // Normal mode sends to server
    CL_SendMove(&cmd);
}

// In frontend binary (use weak symbols or separate file):
void CL_SendMove(usercmd_t *cmd)
{
    // Override: Send to backend via IPC
    PluQ_SendInput(cmd);
}
```

---

## 📊 Comparison

| Aspect | Current (Wrong) | Corrected (Separate Binaries) |
|--------|----------------|-------------------------------|
| **IsFrontend checks** | 5 in shared code | 0 in shared code |
| **Architecture** | Single binary, 3 modes | Two binaries, 2 modes each |
| **Mode detection** | Runtime flag checking | Build-time separation |
| **Server code in frontend** | Linked but skipped | Not linked at all |
| **Code sharing** | Confusing conditionals | Clean separation |

---

## 🏗️ Build Configuration

### Makefile for Main Binary
```makefile
ironwail: $(OBJS_MAIN)
    $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Uses main_sdl.c, host.c, full server code
OBJS_MAIN = main_sdl.o host.o sv_main.o pr_exec.o ...
```

### Makefile for Frontend Binary
```makefile
ironwail-frontend: $(OBJS_FRONTEND)
    $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Uses main_pluq_frontend.c, host_pluq_frontend.c, stubs
OBJS_FRONTEND = main_pluq_frontend.o host_pluq_frontend.o \
                cl_main.o r_main.o gl_*.o \
                stubs_pluq_frontend.o pluq.o
# NO server objects: sv_*.o, pr_*.o
```

---

## ✅ Benefits of Separation

### 1. **Cleaner Code**
- No confusing mode checks
- Each binary has single purpose
- Easier to understand and maintain

### 2. **Smaller Frontend**
- Doesn't link server code (~200KB smaller)
- Faster startup
- Less memory usage

### 3. **Build-Time Safety**
- Frontend can't accidentally run server
- Compile errors if frontend uses server functions
- No runtime mode confusion

### 4. **Simpler Plugin Architecture**
Main binary only needs:
- `IsHeadless()` - Generic feature
- `IsBackend()` - Backend mode detection

Frontend binary:
- No mode checks at all!
- Always knows it's frontend

---

## 🎯 Summary

**The main ironwail binary should have**:
- ✅ `IsHeadless()` checks (generic headless mode)
- ✅ `IsBackend()` checks (backend IPC mode)
- ❌ NO `IsFrontend()` checks (frontend is separate binary)

**The frontend binary (`ironwail-frontend`)**:
- Uses `main_pluq_frontend.c` as entry point
- Uses `host_pluq_frontend.c` for initialization
- Doesn't compile server code at all
- No mode checks needed - it's always frontend

This aligns with the original design intent visible in the separate files!
