# Complete Refactor: Separate Concerns

## Two Issues to Fix

1. **`IsBackend()` should be `IsEnabled()`** - PluQ is just on/off
2. **`PluQ_IsHeadless()` should be `Host_IsHeadless()`** - Headless is generic

---

## Issue 1: PluQ Mode is Overcomplicated

### Current (Wrong)
```c
// pluq.h - PluQ-specific mode enum
typedef enum {
    PLUQ_MODE_DISABLED,
    PLUQ_MODE_BACKEND,   // Only mode used
    PLUQ_MODE_FRONTEND,  // Never used (separate binary!)
    PLUQ_MODE_BOTH       // Dead code
} pluq_mode_t;

qboolean PluQ_IsBackend(void);  // Misleading name
```

### Fixed (Simple Boolean)
```c
// pluq.c - Internal state
static qboolean pluq_enabled = false;

// pluq.h - Clean API
qboolean PluQ_IsEnabled(void);  // Already exists!
void PluQ_Enable(void);
void PluQ_Disable(void);
```

---

## Issue 2: Headless in Wrong Place

### Current (Wrong)
```c
// pluq.h - Headless is NOT PluQ-specific!
qboolean PluQ_IsHeadless(void);

// pluq.c
qboolean PluQ_IsHeadless(void) {
    return (COM_CheckParm("-headless") != 0);
}
```

**Problems:**
- Headless mode is **generic** - useful for servers, benchmarks, testing
- Has nothing to do with PluQ specifically
- PluQ just happens to use it
- Creates false dependency

### Fixed (Generic)
```c
// host.c - Headless is a host feature
static qboolean host_headless = false;

void Host_Init(void)
{
    // Detect headless mode early
    host_headless = (COM_CheckParm("-headless") != 0);

    // Skip video/audio init if headless
    if (!host_headless)
    {
        VID_Init();
        IN_Init();
    }
    // ...
}

qboolean Host_IsHeadless(void)
{
    return host_headless;
}
```

---

## Complete Refactor

### Step 1: Move Headless to host.c

```c
// host.c - Add at top
static qboolean host_headless = false;

qboolean Host_IsHeadless(void)
{
    return host_headless;
}

void Host_Init(void)
{
    // Early headless detection
    host_headless = (COM_CheckParm("-headless") != 0);

    if (host_headless)
        Con_Printf("Running in headless mode (no video/audio)\n");

    // ... rest of init

    // Skip hardware init in headless
    if (!host_headless)
    {
        VID_Init();
        IN_Init();
    }

    // PluQ is separate concern
    PluQ_Init();
    if (COM_CheckParm("-pluq"))
    {
        if (!host_headless)
            Sys_Error("PluQ requires -headless flag");
        PluQ_Enable();
    }
}
```

### Step 2: Simplify PluQ to Boolean

```c
// pluq.c
static qboolean pluq_enabled = false;

qboolean PluQ_IsEnabled(void)
{
    return pluq_enabled && pluq_initialized;
}

void PluQ_Enable(void)
{
    if (!pluq_initialized)
    {
        Con_Printf("Enabling PluQ IPC...\n");
        // Initialize sockets, etc.
        PluQ_Initialize();
    }
    pluq_enabled = true;
}

void PluQ_Disable(void)
{
    pluq_enabled = false;
}
```

### Step 3: Update All Callsites

```c
// host.c - Replace PluQ_IsBackend() with PluQ_IsEnabled()
if (PluQ_IsEnabled())
    PluQ_ProcessInputCommands();

if (PluQ_IsEnabled())
    PluQ_BroadcastWorldState();

// Replace PluQ_IsHeadless() with Host_IsHeadless()
if (!Host_IsHeadless())
{
    Key_UpdateForDest();
    IN_UpdateInputMode();
    // ...
}

if (!Host_IsHeadless())
{
    SCR_UpdateScreen();
    // ...
}
```

```c
// cl_main.c - Replace PluQ_IsBackend() with PluQ_IsEnabled()
if (!PluQ_IsEnabled())
    CL_SendMove(&cmd);
```

### Step 4: Clean Up Headers

```c
// quakedef.h or host.h - Generic headless support
qboolean Host_IsHeadless(void);

// pluq.h - Simplified PluQ API
// Remove:
// - typedef enum pluq_mode_t
// - PluQ_IsBackend()
// - PluQ_IsFrontend()
// - PluQ_IsHeadless()
// - PluQ_SetMode()

// Keep/Add:
qboolean PluQ_IsEnabled(void);
void PluQ_Enable(void);
void PluQ_Disable(void);
```

---

## Benefits

### 1. Separation of Concerns

**Before:**
```
PluQ owns headless mode ✗
```

**After:**
```
Host owns headless mode ✓
PluQ is just IPC on/off ✓
```

### 2. Reusability

Headless mode can now be used for:
- ✅ PluQ backend
- ✅ Dedicated servers
- ✅ Benchmarking
- ✅ Automated testing
- ✅ CI/CD pipelines

Without depending on PluQ!

### 3. Clarity

```c
// BEFORE (confusing)
if (PluQ_IsBackend())        // What's a backend?
if (!PluQ_IsHeadless())      // Why is headless in PluQ?

// AFTER (clear)
if (PluQ_IsEnabled())        // Is IPC enabled?
if (!Host_IsHeadless())      // Is rendering disabled?
```

### 4. Correct Dependencies

```
Before:
  host.c → pluq.h (for headless check!) ✗

After:
  host.c → (owns headless)
  pluq.c → (optional, for IPC only) ✓
```

---

## File Changes Summary

### host.c
```diff
+ static qboolean host_headless = false;
+ qboolean Host_IsHeadless(void) { return host_headless; }

  void Host_Init(void)
  {
+     host_headless = (COM_CheckParm("-headless") != 0);
-     qboolean headless = (COM_CheckParm("-headless") != 0);

-     if (!headless)
+     if (!host_headless)
          VID_Init();

-     PluQ_SetMode(PLUQ_MODE_BACKEND);
+     PluQ_Enable();
  }

  void _Host_Frame(double time)
  {
-     if (!PluQ_IsHeadless())
+     if (!Host_IsHeadless())
          Key_UpdateForDest();

-     if (PluQ_IsBackend())
+     if (PluQ_IsEnabled())
          PluQ_ProcessInputCommands();

-     if (PluQ_IsBackend())
+     if (PluQ_IsEnabled())
          PluQ_BroadcastWorldState();

-     if (!PluQ_IsHeadless())
+     if (!Host_IsHeadless())
          SCR_UpdateScreen();
  }
```

### cl_main.c
```diff
  void CL_SendCmd(void)
  {
-     if (!PluQ_IsBackend())
+     if (!PluQ_IsEnabled())
          CL_SendMove(&cmd);
  }
```

### pluq.c
```diff
- static pluq_mode_t pluq_mode = PLUQ_MODE_DISABLED;
+ static qboolean pluq_enabled = false;
- static cvar_t pluq_headless = {"pluq_headless", "0", CVAR_NONE};

- qboolean PluQ_IsBackend(void) { return pluq_mode == PLUQ_MODE_BACKEND; }
- qboolean PluQ_IsFrontend(void) { return pluq_mode == PLUQ_MODE_FRONTEND; }
- qboolean PluQ_IsHeadless(void) { return COM_CheckParm("-headless") != 0; }

+ qboolean PluQ_IsEnabled(void) { return pluq_enabled && pluq_initialized; }
+ void PluQ_Enable(void) { ... }
+ void PluQ_Disable(void) { pluq_enabled = false; }
```

### pluq.h
```diff
- typedef enum {
-     PLUQ_MODE_DISABLED,
-     PLUQ_MODE_BACKEND,
-     PLUQ_MODE_FRONTEND,
-     PLUQ_MODE_BOTH
- } pluq_mode_t;

- qboolean PluQ_IsBackend(void);
- qboolean PluQ_IsFrontend(void);
- qboolean PluQ_IsHeadless(void);
- void PluQ_SetMode(pluq_mode_t mode);

+ qboolean PluQ_IsEnabled(void);
+ void PluQ_Enable(void);
+ void PluQ_Disable(void);
```

### quakedef.h
```diff
+ // Generic headless mode (no PluQ dependency)
+ qboolean Host_IsHeadless(void);
```

---

## Migration Checklist

- [ ] Add `Host_IsHeadless()` to host.c
- [ ] Add `host_headless` static variable
- [ ] Replace all `PluQ_IsHeadless()` → `Host_IsHeadless()` (4 occurrences)
- [ ] Replace all `PluQ_IsBackend()` → `PluQ_IsEnabled()` (4 occurrences)
- [ ] Simplify `pluq.c` mode tracking (enum → boolean)
- [ ] Remove `pluq_headless` cvar (unused)
- [ ] Remove `PluQ_SetMode()`, add `PluQ_Enable()`
- [ ] Clean up `pluq.h` API
- [ ] Test with `-headless` flag alone (should work without `-pluq`)
- [ ] Test with `-headless -pluq` flags (should enable both)

---

## Result: Clean Architecture

```
Ironwail Core:
  ├─ Host (owns generic features)
  │   ├─ Headless mode (-headless flag)
  │   ├─ Dedicated mode (-dedicated flag)
  │   └─ Frame loop
  │
  └─ PluQ (optional IPC extension)
      ├─ Enable/Disable (simple boolean)
      ├─ Broadcast state (when enabled)
      └─ Receive input (when enabled)
```

**Dependencies:**
- ✅ Host is independent
- ✅ PluQ depends on host, not vice versa
- ✅ Headless works without PluQ
- ✅ PluQ requires headless (checked at enable time)

This is how it should be!
