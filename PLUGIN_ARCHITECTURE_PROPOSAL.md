# Ironwail Plugin Architecture Proposal

## Current Problem

PluQ is currently tightly integrated with **11 scattered checks** in `host.c` alone:
- 6× `PluQ_IsBackend()`
- 3× `PluQ_IsFrontend()`
- 4× `PluQ_IsHeadless()`

This makes the codebase harder to maintain and prevents easy addition of other extensions.

## Goals

1. **Minimal core changes** - Keep Ironwail core clean
2. **Separation** - PluQ should be a loadable module
3. **Generic headless mode** - Not PluQ-specific
4. **Extension API** - Allow other plugins (AI bots, replay systems, benchmarking, etc.)
5. **Zero overhead** - No performance impact when plugins disabled

---

## Proposed Architecture

### 1. Host Operations Interface (`host_ops.h`)

Replace scattered checks with a **function pointer table**:

```c
typedef struct {
    // Lifecycle hooks
    void (*init)(void);
    void (*shutdown)(void);

    // Frame hooks
    void (*frame_begin)(void);
    void (*frame_end)(void);
    void (*pre_server_frame)(void);
    void (*post_server_frame)(void);

    // Input hooks
    qboolean (*override_input)(void);      // Return true to skip local input
    void (*process_input)(void);           // Called before game input

    // World state hooks
    void (*broadcast_state)(void);         // Called after frame render
    qboolean (*receive_state)(void);       // Return true if state received

    // Rendering hooks
    qboolean (*should_skip_render)(void);  // Return true to skip video/audio

    // Mode queries
    qboolean (*is_active)(void);
    const char* (*get_name)(void);
} host_extension_ops_t;
```

### 2. Extension Registration

Simple registration in `Host_Init()`:

```c
// host_extension.c
static host_extension_ops_t *extensions[MAX_EXTENSIONS];
static int num_extensions = 0;

void Host_RegisterExtension(host_extension_ops_t *ops)
{
    if (num_extensions < MAX_EXTENSIONS)
        extensions[num_extensions++] = ops;
}

// Example: PluQ registration
void PluQ_RegisterExtension(void)
{
    static host_extension_ops_t pluq_ops = {
        .init = PluQ_Init,
        .shutdown = PluQ_Shutdown,
        .process_input = PluQ_ProcessInputCommands,
        .broadcast_state = PluQ_BroadcastWorldState,
        .receive_state = PluQ_ReceiveWorldState,
        .should_skip_render = PluQ_IsHeadless,
        .is_active = PluQ_IsEnabled,
        .get_name = "PluQ IPC"
    };

    if (COM_CheckParm("-pluq"))
        Host_RegisterExtension(&pluq_ops);
}
```

### 3. Simplified Host Frame Loop

Replace 11 checks with **iterator pattern**:

```c
void _Host_Frame(double time)
{
    // ... existing setup ...

    // Extension hooks: frame begin
    for (int i = 0; i < num_extensions; i++)
        if (extensions[i]->frame_begin)
            extensions[i]->frame_begin();

    // Input processing
    qboolean skip_local_input = false;
    for (int i = 0; i < num_extensions; i++)
        if (extensions[i]->override_input && extensions[i]->override_input())
            skip_local_input = true;

    if (!skip_local_input)
    {
        Key_UpdateForDest();
        IN_UpdateInputMode();
        Sys_SendKeyEvents();
        IN_Commands();
    }

    // Extension input processing
    for (int i = 0; i < num_extensions; i++)
        if (extensions[i]->process_input)
            extensions[i]->process_input();

    // ... rest of frame ...

    // Server frame
    qboolean skip_server = false;
    for (int i = 0; i < num_extensions; i++)
        if (extensions[i]->override_server && extensions[i]->override_server())
            skip_server = true;

    if (sv.active && !skip_server)
        Host_ServerFrame();

    // World state broadcast
    for (int i = 0; i < num_extensions; i++)
        if (extensions[i]->broadcast_state)
            extensions[i]->broadcast_state();

    // Rendering
    qboolean skip_render = false;
    for (int i = 0; i < num_extensions; i++)
        if (extensions[i]->should_skip_render && extensions[i]->should_skip_render())
            skip_render = true;

    if (!skip_render)
    {
        SCR_UpdateScreen();
        // ... audio, particles, etc ...
    }
}
```

---

## Benefits

### For PluQ
- **Reduced from 11 checks → 1 registration call**
- All PluQ code stays in `pluq.c` / `pluq.h`
- Can be compiled out with `#ifndef ENABLE_PLUQ`

### For Ironwail Core
- **host.c**: Remove all PluQ-specific includes and checks
- **cl_main.c**: Remove PluQ input routing
- **Generic**: Headless mode becomes a standard feature, not PluQ-specific

### For Future Extensions
- **AI Bots**: Hook into input processing
- **Demo Playback**: Override input and server frames
- **Benchmarking**: Hook frame timing, skip rendering
- **Networking**: Alternative network backends
- **Scripting**: Lua/Python hooks into game loop

---

## Migration Path

### Phase 1: Create Extension API (2-3 hours)
1. Create `host_extension.h` and `host_extension.c`
2. Define `host_extension_ops_t` interface
3. Add registration functions

### Phase 2: Refactor PluQ (1-2 hours)
1. Create `PluQ_RegisterExtension()` in `pluq.c`
2. Move all PluQ mode checks into callbacks
3. Remove PluQ includes from `host.c`

### Phase 3: Update Host Loop (1-2 hours)
1. Replace PluQ checks with extension iterator calls
2. Test with PluQ enabled/disabled
3. Verify no performance regression

### Phase 4: Cleanup (30 mins)
1. Remove `PluQ_Is*()` calls from host.c
2. Update documentation
3. Create example extension template

---

## Advanced: DLL/Shared Library Plugins

For true runtime loading (optional):

```c
// Plugin descriptor
typedef struct {
    int api_version;
    const char *name;
    const char *author;
    void (*register_hooks)(host_extension_ops_t *ops);
} plugin_descriptor_t;

// In each plugin .so/.dll
PLUGIN_EXPORT plugin_descriptor_t plugin_info = {
    .api_version = 1,
    .name = "PluQ IPC",
    .author = "Ironwail Team",
    .register_hooks = PluQ_RegisterExtension
};

// Host loader
void Host_LoadPlugin(const char *path)
{
    void *handle = dlopen(path, RTLD_NOW);
    plugin_descriptor_t *desc = dlsym(handle, "plugin_info");

    if (desc->api_version == PLUGIN_API_VERSION)
    {
        host_extension_ops_t ops = {0};
        desc->register_hooks(&ops);
        Host_RegisterExtension(&ops);
    }
}
```

---

## Comparison

| Aspect | Current (Integrated) | Proposed (Plugin) |
|--------|---------------------|-------------------|
| **PluQ checks in host.c** | 11 | 0 |
| **Core file modifications** | 4 files | 1 file (registration only) |
| **Compile-time coupling** | Tight | Loose |
| **Runtime overhead** | Zero | Negligible (function pointers) |
| **Extension support** | PluQ only | Any plugin |
| **Headless mode** | PluQ-specific | Generic feature |

---

## Testing Strategy

1. **Baseline**: Run current PluQ tests, record results
2. **Refactor**: Implement plugin architecture
3. **Validation**: Re-run all tests, verify identical behavior
4. **Benchmark**: Measure frame time overhead (should be <0.1%)
5. **Disable test**: Compile without ENABLE_PLUQ, verify clean build

---

## Implementation Priority

### Must Have (Core)
- [ ] Extension registration API
- [ ] Frame lifecycle hooks
- [ ] Input override hooks
- [ ] Rendering skip hooks

### Should Have (PluQ)
- [ ] PluQ migration to plugin model
- [ ] Remove PluQ checks from host.c
- [ ] Separate headless mode flag handling

### Nice to Have (Future)
- [ ] DLL loading support
- [ ] Plugin hot-reload
- [ ] Extension configuration API
- [ ] Inter-plugin communication

---

## Questions to Consider

1. **Static vs Dynamic**: Should plugins be compiled-in or runtime-loaded?
   - **Recommendation**: Start with static (compile-time), add DLL support later

2. **Performance**: Is function pointer overhead acceptable?
   - **Analysis**: ~1-2ns per call, negligible compared to frame time

3. **Compatibility**: Can this work on all platforms (Windows, Linux, macOS)?
   - **Yes**: Function pointers are standard C, DLL loading is platform-specific but supported

4. **Backward Compatibility**: How to support old build system?
   - **Solution**: Keep PluQ integration as fallback with `#ifndef USE_HOST_EXTENSIONS`

---

## Next Steps

1. Review and approve architecture
2. Create proof-of-concept for one hook type
3. Migrate PluQ incrementally
4. Document plugin API for future extensions
5. Create example extension (e.g., simple frame logger)
