# Fix: Remove IsFrontend() Checks from Shared Code

## Problem

Currently there are **5 `IsFrontend()` checks in shared code**:
- `host.c`: Lines 1277, 1285, 1300
- `cl_main.c`: Lines 838, 845

This is **architecturally wrong** because:
1. Frontend is a **separate binary** (`ironwail-pluq-frontend`)
2. Built from separate Makefile (`Makefile.pluq_frontend`)
3. Uses separate entry point (`main_pluq_frontend.c`)
4. Server code is **not even linked** in frontend binary

The shared code (`host.c`, `cl_main.c`) should **never** check for frontend mode.

---

## Solution: Two Separate Binaries, Zero Frontend Checks

### Binary 1: ironwail (Main Binary)
**Modes**:
- Normal: Standard Quake
- Backend: `-headless -pluq`

**Checks Allowed**:
- ✅ `IsHeadless()` - Generic headless mode
- ✅ `IsBackend()` - Backend IPC mode
- ❌ `IsFrontend()` - NEVER

### Binary 2: ironwail-pluq-frontend (Frontend Binary)
**Mode**: Frontend only (no flags needed)

**Checks Allowed**:
- ❌ No mode checks needed - it's always frontend

---

## Required Code Changes

### 1. host.c - Remove 3 checks

#### Change 1: Line 1277 (Remove entirely)
```c
// BEFORE (WRONG):
if (PluQ_IsFrontend())
{
    // Some frontend-specific logic
}

// AFTER (CORRECT):
// Remove this check entirely
// Frontend binary uses host_pluq_frontend.c instead of this code path
```

#### Change 2: Line 1285 (Simplify)
```c
// BEFORE (WRONG):
if (sv.active && !PluQ_IsFrontend())
{
    PR_SwitchQCVM(&sv.qcvm);
    Host_ServerFrame();
    PR_SwitchQCVM(NULL);
}

// AFTER (CORRECT):
if (sv.active)  // Frontend binary has sv.active=false, no check needed
{
    PR_SwitchQCVM(&sv.qcvm);
    Host_ServerFrame();
    PR_SwitchQCVM(NULL);
}
```

**Why this works**:
- Main binary: `sv.active` can be true, runs server normally
- Backend mode: `sv.active` is true, runs server
- Frontend binary: `sv.active` is **always false** (no server linked!)

#### Change 3: Line 1300 (Remove entirely)
```c
// BEFORE (WRONG):
if (PluQ_IsFrontend())
{
    if (PluQ_ReceiveWorldState())
        PluQ_ApplyReceivedState();
}

// AFTER (CORRECT):
// Remove this check entirely
// Frontend uses different frame loop in main_pluq_frontend.c
```

---

### 2. cl_main.c - Remove 2 checks

#### Change 4: Lines 838-841 (Refactor)
```c
// BEFORE (WRONG):
if (PluQ_IsFrontend())
    PluQ_SendInput(&cmd);
else
    CL_SendMove(&cmd);

// AFTER (CORRECT):
// In main binary:
if (PluQ_IsBackend())
    return;  // Backend doesn't send commands, receives via IPC

CL_SendMove(&cmd);  // Normal mode sends to server
```

For frontend binary, **override in separate file** or use **weak symbols**:
```c
// In cl_pluq_frontend.c (frontend build only):
void CL_SendMove(usercmd_t *cmd)
{
    PluQ_SendInput(cmd);  // Frontend always sends via IPC
}
```

#### Change 5: Line 845 (Simplify)
```c
// BEFORE (WRONG):
if (!PluQ_IsFrontend())
{
    // Some logic
}

// AFTER (CORRECT):
// Remove check - frontend uses different code path
```

---

## Alternative: Use Separate Source Files

Instead of shared code with checks, use **separate files**:

### Main Binary Build
```makefile
# Uses these files:
- host.c (normal + backend)
- cl_main.c (normal + backend)
```

### Frontend Binary Build
```makefile
# Uses these files:
- host_pluq_frontend.c (already exists!)
- cl_main.c (with PluQ stubs overriding CL_SendMove)
  OR
- cl_pluq_frontend.c (new file with frontend-specific client code)
```

This way:
- **Zero** mode checks in shared code
- **Zero** `#ifdef` conditionals
- **Build-time** separation instead of runtime checks

---

## Benefits

### 1. Cleaner Architecture
```
Main Binary:
  if (IsBackend())  ✅ OK - this binary can be backend
  if (!IsBackend()) ✅ OK - this binary can be normal

Frontend Binary:
  // No checks needed! ✅
```

### 2. Impossible States Become Unrepresentable
```c
// Frontend binary:
Host_ServerFrame();  // ❌ Linker error - symbol not found!
```

Frontend **cannot** accidentally run server code because it's **not linked**.

### 3. Smaller Frontend Binary
- No server code: ~200KB smaller
- No QuakeC VM: ~50KB smaller
- No networking: ~30KB smaller
- **Total**: ~280KB smaller binary

### 4. Build-Time Safety
```
Main binary:    Can call SV_*(), PR_*(), NET_*()  ✅
Frontend binary: Cannot call SV_*(), PR_*(), NET_*()  ❌ Link error
```

---

## Migration Path

### Phase 1: Remove Checks (Safe Refactor)
1. Remove line 1277 check from host.c
2. Simplify line 1285 in host.c (remove `!IsFrontend()`)
3. Remove line 1300 check from host.c
4. Refactor cl_main.c to use `IsBackend()` instead of `IsFrontend()`
5. Test both binaries

### Phase 2: Verify Separation
1. Build main binary: `make`
2. Build frontend binary: `make -f Makefile.pluq_frontend`
3. Test main binary in normal mode
4. Test main binary in backend mode (`-headless -pluq`)
5. Test frontend binary with backend

### Phase 3: Remove IsFrontend() Function (Optional)
Once all checks are removed:
```c
// In pluq.h - mark as deprecated
qboolean PluQ_IsFrontend(void) __attribute__((deprecated));
```

Or remove entirely if not used elsewhere.

---

## Testing

### Test 1: Main Binary Normal Mode
```bash
./ironwail
# Should work normally, no PluQ
```

### Test 2: Main Binary Backend Mode
```bash
./ironwail -headless -pluq +map start
# Should run server, broadcast state, no rendering
```

### Test 3: Frontend Binary
```bash
./ironwail-pluq-frontend
# Should connect to backend, receive state, render
```

### Test 4: Verify Server Not Linked
```bash
nm ironwail-pluq-frontend | grep SV_Frame
# Should output nothing - symbol not present
```

---

## Summary

**Current (Wrong)**:
- 5 `IsFrontend()` checks in shared code
- Runtime mode detection
- Confusing code paths

**Corrected (Right)**:
- 0 `IsFrontend()` checks in shared code
- Build-time separation
- Each binary knows its purpose

**Main binary** should only have:
- `IsHeadless()` - Generic headless mode
- `IsBackend()` - Backend IPC mode

**Frontend binary**:
- No mode checks at all
- Always knows it's frontend
- Uses separate entry point and initialization
