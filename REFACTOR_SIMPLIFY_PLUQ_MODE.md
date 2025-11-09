# Refactor: Simplify PluQ Mode to Boolean

## Problem

Current mode system is overcomplicated for separate binaries:

```c
typedef enum {
    PLUQ_MODE_DISABLED,
    PLUQ_MODE_BACKEND,   // Only used in main binary
    PLUQ_MODE_FRONTEND,  // Never used in main binary!
    PLUQ_MODE_BOTH       // Dead code, never set
} pluq_mode_t;
```

**Issues:**
1. `IsBackend()` is misleading - main binary doesn't have backend/frontend distinction
2. `PLUQ_MODE_FRONTEND` never used in main binary (frontend is separate executable)
3. `PLUQ_MODE_BOTH` is dead code
4. Enum complexity for simple on/off state

## Solution: Simple Boolean

### For Main Binary

Replace enum with boolean:

```c
// pluq.h - BEFORE
typedef enum {
    PLUQ_MODE_DISABLED,
    PLUQ_MODE_BACKEND,
    PLUQ_MODE_FRONTEND,
    PLUQ_MODE_BOTH
} pluq_mode_t;

qboolean PluQ_IsBackend(void);
qboolean PluQ_IsFrontend(void);

// pluq.h - AFTER
// Simple: PluQ is either enabled or disabled
static qboolean pluq_enabled = false;

// Clear API
qboolean PluQ_IsEnabled(void);  // Already exists!
void PluQ_Enable(void);
void PluQ_Disable(void);
```

### Code Changes

#### 1. Replace IsBackend() with IsEnabled()

```c
// host.c - BEFORE
if (PluQ_IsBackend())
    PluQ_ProcessInputCommands();

if (PluQ_IsBackend())
    PluQ_BroadcastWorldState();

// host.c - AFTER
if (PluQ_IsEnabled())
    PluQ_ProcessInputCommands();

if (PluQ_IsEnabled())
    PluQ_BroadcastWorldState();
```

#### 2. Invert logic in cl_main.c

```c
// cl_main.c - BEFORE
if (!PluQ_IsBackend())
    CL_SendMove(&cmd);

// cl_main.c - AFTER
if (!PluQ_IsEnabled())
    CL_SendMove(&cmd);
```

Much clearer: "If PluQ is not enabled, send moves normally"

#### 3. Simplify initialization

```c
// host.c - BEFORE
if (COM_CheckParm("-pluq"))
{
    if (!COM_CheckParm("-headless"))
        Sys_Error("PluQ backend mode requires -headless flag");
    Cvar_Set("pluq_headless", "1");
    PluQ_SetMode(PLUQ_MODE_BACKEND);
}

// host.c - AFTER
if (COM_CheckParm("-pluq"))
{
    if (!COM_CheckParm("-headless"))
        Sys_Error("PluQ requires -headless flag");
    Cvar_Set("pluq_headless", "1");
    PluQ_Enable();
}
```

#### 4. Simplify pluq.c

```c
// pluq.c - BEFORE
static pluq_mode_t pluq_mode = PLUQ_MODE_DISABLED;

qboolean PluQ_IsBackend(void) {
    return pluq_mode == PLUQ_MODE_BACKEND || pluq_mode == PLUQ_MODE_BOTH;
}
qboolean PluQ_IsFrontend(void) {
    return pluq_mode == PLUQ_MODE_FRONTEND;
}
qboolean PluQ_IsEnabled(void) {
    return pluq_initialized && pluq_mode != PLUQ_MODE_DISABLED;
}

// pluq.c - AFTER
static qboolean pluq_enabled = false;

qboolean PluQ_IsEnabled(void) {
    return pluq_enabled && pluq_initialized;
}

void PluQ_Enable(void) {
    if (!pluq_initialized)
        PluQ_Initialize();
    pluq_enabled = true;
}

void PluQ_Disable(void) {
    pluq_enabled = false;
}
```

### Frontend Binary (Separate)

Frontend binary has its own simple flag:

```c
// main_pluq_frontend.c
static const qboolean is_pluq_frontend = true;  // Always true in frontend

// Or just hardcode:
void CL_SendMove(usercmd_t *cmd)
{
    PluQ_SendInput(cmd);  // Frontend always uses IPC
}
```

No mode enum needed!

## Benefits

### 1. Clarity
```c
// BEFORE (confusing)
if (PluQ_IsBackend())  // What's a "backend"?

// AFTER (clear)
if (PluQ_IsEnabled())  // Is PluQ IPC enabled?
```

### 2. Simplicity
- **Before**: 4-value enum, 3 mode check functions
- **After**: 1 boolean, 1 check function

### 3. Correctness
- **Before**: Modes that don't make sense (FRONTEND in main binary)
- **After**: Only states that can actually occur

### 4. Less Code
```c
// Remove:
- typedef enum (4 values)
- PluQ_IsBackend()
- PluQ_IsFrontend()
- PluQ_SetMode()
- PLUQ_MODE_BOTH handling

// Keep:
- PluQ_IsEnabled() (already exists!)
- PluQ_Enable()
- PluQ_Disable()
```

## Alternative: Keep Enum for Frontend Binary

If we want frontend binary to share some pluq.c code:

```c
// pluq.h
typedef enum {
    PLUQ_MODE_DISABLED,
    PLUQ_MODE_ENABLED      // Single enabled mode
} pluq_mode_t;

// Or just use boolean everywhere
```

But honestly, a boolean is clearest.

## Migration Steps

1. **Replace all `IsBackend()` → `IsEnabled()`** (6 occurrences)
2. **Remove `IsFrontend()`** (already done!)
3. **Replace `SetMode(BACKEND)` → `Enable()`** (1 occurrence)
4. **Simplify pluq.c mode tracking** (boolean instead of enum)
5. **Update pluq.h** (remove enum, simplify API)
6. **Test**: Verify `-pluq` flag still works

## Summary

**Main Binary:**
- ✅ `PluQ_IsEnabled()` - Is IPC active?
- ✅ `PluQ_IsHeadless()` - Is rendering disabled?
- ❌ ~~`PluQ_IsBackend()`~~ - Misleading
- ❌ ~~`PluQ_IsFrontend()`~~ - Wrong (removed)

**Frontend Binary:**
- No mode checks needed
- Always knows it's frontend
- Can hardcode IPC usage

This matches the actual architecture: main binary has PluQ on/off, frontend binary is always IPC-based.
