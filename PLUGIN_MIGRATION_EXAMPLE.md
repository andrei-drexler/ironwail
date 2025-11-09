# PluQ Plugin Migration Example

## Before: Tight Integration (Current)

### host.c - Scattered checks throughout
```c
#include "pluq.h"  // ❌ PluQ-specific include in core file

void _Host_Frame(double time)
{
    // ... setup ...

    // ❌ Check #1: Skip local input
    if (!PluQ_IsHeadless())
    {
        Key_UpdateForDest();
        IN_UpdateInputMode();
        Sys_SendKeyEvents();
        IN_Commands();
    }

    // ❌ Check #2: Process IPC input
    if (PluQ_IsBackend())
        PluQ_ProcessInputCommands();

    // ... more code ...

    // ❌ Check #3: Skip server in frontend mode
    if (sv.active && !PluQ_IsFrontend())
    {
        PR_SwitchQCVM(&sv.qcvm);
        Host_ServerFrame();
        PR_SwitchQCVM(NULL);
    }

    // ❌ Check #4: Receive world state (frontend)
    if (PluQ_IsFrontend())
    {
        if (PluQ_ReceiveWorldState())
            PluQ_ApplyReceivedState();
    }

    // ❌ Check #5: Broadcast world state (backend)
    if (PluQ_IsBackend())
        PluQ_BroadcastWorldState();

    // ❌ Check #6: Skip rendering
    if (!PluQ_IsHeadless())
    {
        SCR_UpdateScreen();
        CL_RunParticles();
        BGM_Update();
        S_Update(&r_refdef.vieworg, &vpn, &vright, &vup);
        CDAudio_Update();
    }

    // ❌ Check #7: Skip window title update
    if (!PluQ_IsHeadless() && cls.state != ca_dedicated)
        UpdateWindowTitle();
}

void Host_Init(void)
{
    // ... initialization ...

    // ❌ Check #8: Headless mode detection
    qboolean headless = (COM_CheckParm("-headless") != 0);

    // ❌ Checks #9-10: Skip video/audio in headless
    if (!headless)
    {
        VID_Init();
        IN_Init();
    }

    // ❌ Check #11: PluQ backend mode setup
    PluQ_Init();
    if (COM_CheckParm("-pluq"))
    {
        if (!COM_CheckParm("-headless"))
            Sys_Error("PluQ backend mode requires -headless flag");
        Cvar_Set("pluq_headless", "1");
        PluQ_SetMode(PLUQ_MODE_BACKEND);
    }
}
```

**Result**: 11 PluQ-specific checks scattered throughout host.c

---

## After: Plugin Architecture (Proposed)

### host.c - Clean, extension-agnostic
```c
#include "host_extension.h"  // ✅ Generic extension API only

void _Host_Frame(double time)
{
    // ... setup ...

    Host_Extension_FrameBegin();  // ✅ Generic hook

    // ✅ Clean abstraction - any extension can skip local input
    if (!Host_Extension_ShouldSkipLocalInput())
    {
        Key_UpdateForDest();
        IN_UpdateInputMode();
        Sys_SendKeyEvents();
        IN_Commands();
    }

    // ✅ Generic input processing hook
    Host_Extension_ProcessInput();

    // ... more code ...

    // ✅ Clean server frame handling
    if (sv.active && !Host_Extension_ShouldSkipServer())
    {
        Host_Extension_PreServerFrame();
        PR_SwitchQCVM(&sv.qcvm);
        Host_ServerFrame();
        PR_SwitchQCVM(NULL);
        Host_Extension_PostServerFrame();
    }

    // ✅ Generic world state reception
    if (Host_Extension_ReceiveWorldState())
        Host_Extension_ApplyReceivedState();

    // ✅ Generic world state broadcast
    Host_Extension_BroadcastWorldState();

    // ✅ Clean rendering abstraction
    if (!Host_Extension_ShouldSkipRendering())
    {
        Host_Extension_PreRender();
        SCR_UpdateScreen();
        CL_RunParticles();
        BGM_Update();
        S_Update(&r_refdef.vieworg, &vpn, &vright, &vup);
        CDAudio_Update();
        Host_Extension_PostRender();
    }

    Host_Extension_FrameEnd();  // ✅ Generic hook
}

void Host_Init(void)
{
    // ... initialization ...

    // ✅ Headless mode is now generic (not PluQ-specific)
    qboolean headless = (COM_CheckParm("-headless") != 0);

    if (!headless)
    {
        VID_Init();
        IN_Init();
    }

    // ✅ Single extension initialization call
    Host_Extension_Init();
}
```

**Result**: 0 PluQ-specific checks. All extension logic is generic.

---

### pluq.c - Implements extension interface
```c
#include "host_extension.h"

// Extension operations table
static host_extension_ops_t pluq_extension_ops = {
    .name = "PluQ IPC",
    .version = "2.0",

    // Lifecycle
    .init = PluQ_Init_Internal,
    .shutdown = PluQ_Shutdown,

    // Input
    .should_skip_local_input = PluQ_IsHeadless,
    .process_input = PluQ_ProcessInputCommands,

    // Server
    .should_skip_server = PluQ_IsFrontend,

    // World state
    .broadcast_world_state = PluQ_BroadcastWorldState,
    .receive_world_state = PluQ_ReceiveWorldState,
    .apply_received_state = PluQ_ApplyReceivedState,

    // Rendering
    .should_skip_rendering = PluQ_IsHeadless,

    // Status
    .is_active = PluQ_IsEnabled,
};

// Called during Host_Extension_Init()
static void PluQ_Init_Internal(void)
{
    // Existing PluQ_Init() code here...
    Cvar_RegisterVariable(&pluq_headless);
    nng_init(NULL);
    Con_Printf("PluQ IPC system ready (nng 2.0 + FlatBuffers)\n");

    // Check for -pluq flag
    if (COM_CheckParm("-pluq"))
    {
        if (!COM_CheckParm("-headless"))
            Sys_Error("PluQ backend mode requires -headless flag");

        Cvar_Set("pluq_headless", "1");
        PluQ_SetMode(PLUQ_MODE_BACKEND);
    }
}

// Auto-registration at startup
void PluQ_RegisterExtension(void) __attribute__((constructor));
void PluQ_RegisterExtension(void)
{
    Host_RegisterExtension(&pluq_extension_ops);
}
```

---

## Code Reduction Summary

### Before (Integrated)
| File | Lines with PluQ | PluQ Includes |
|------|----------------|---------------|
| host.c | 25+ lines | 1 include |
| cl_main.c | 12+ lines | 1 include |
| main_sdl.c | 3 lines | 0 includes |
| **Total** | **40+ lines** | **2 includes** |

### After (Plugin)
| File | Lines with Extensions | Extension Includes |
|------|---------------------|-------------------|
| host.c | 12 generic calls | 1 include (host_extension.h) |
| cl_main.c | 0 (moved to pluq.c) | 0 includes |
| main_sdl.c | 0 | 0 includes |
| **Total** | **12 generic calls** | **1 include** |

**Net Change**: -28 PluQ-specific lines, -1 include

---

## Benefits

### 1. Separation of Concerns
- **host.c**: Generic game loop, no knowledge of PluQ
- **pluq.c**: All PluQ logic in one place
- **Extension API**: Clean interface between them

### 2. Testability
```c
// Can now test PluQ in isolation
void test_pluq_input_processing(void)
{
    host_extension_ops_t *ops = &pluq_extension_ops;
    ops->init();
    ops->process_input();
    assert(commands_processed == 5);
    ops->shutdown();
}
```

### 3. Future Extensions
Other extensions can use the same hooks:

```c
// AI bot extension
static host_extension_ops_t bot_extension_ops = {
    .name = "AI Bots",
    .version = "1.0",
    .process_input = Bot_GenerateInputCommands,
};

// Replay system
static host_extension_ops_t replay_extension_ops = {
    .name = "Demo Replay",
    .version = "1.0",
    .should_skip_local_input = Replay_IsPlaying,
    .process_input = Replay_InjectInputFromFile,
    .should_skip_server = Replay_IsPlaying,
};

// Benchmark tool
static host_extension_ops_t benchmark_extension_ops = {
    .name = "Benchmark",
    .version = "1.0",
    .should_skip_rendering = Benchmark_IsRunning,
    .post_server_frame = Benchmark_RecordFrameTime,
};
```

### 4. Compilation Options
```makefile
# Compile without PluQ
CFLAGS += -DDISABLE_PLUQ

# Or with selective extensions
CFLAGS += -DENABLE_PLUQ -DENABLE_AI_BOTS
```

---

## Migration Checklist

### Phase 1: Extension API ✅
- [x] Create host_extension.h
- [x] Create host_extension.c
- [x] Add to Makefile

### Phase 2: Refactor host.c
- [ ] Replace `PluQ_IsHeadless()` → `Host_Extension_ShouldSkipRendering()`
- [ ] Replace `PluQ_IsBackend()` → `Host_Extension_BroadcastWorldState()`
- [ ] Replace `PluQ_IsFrontend()` → `Host_Extension_ShouldSkipServer()`
- [ ] Replace `PluQ_ProcessInputCommands()` → `Host_Extension_ProcessInput()`
- [ ] Remove `#include "pluq.h"` from host.c

### Phase 3: Refactor pluq.c
- [ ] Create `pluq_extension_ops` table
- [ ] Implement `PluQ_RegisterExtension()`
- [ ] Move flag parsing into init callback
- [ ] Keep existing PluQ API for backward compatibility

### Phase 4: Testing
- [ ] Test with -pluq flag
- [ ] Test without -pluq flag
- [ ] Test headless mode alone
- [ ] Verify all IPC tests still pass

### Phase 5: Documentation
- [ ] Update PLUGIN_ARCHITECTURE_PROPOSAL.md
- [ ] Create EXTENSION_API.md
- [ ] Add example extension template

---

## Backward Compatibility

Keep existing PluQ API functions as wrappers:

```c
// Old API (deprecated but functional)
qboolean PluQ_IsBackend(void)
{
    return pluq_mode == PLUQ_MODE_BACKEND;
}

qboolean PluQ_IsHeadless(void)
{
    return COM_CheckParm("-headless") != 0;
}
```

This allows gradual migration and doesn't break external code.
