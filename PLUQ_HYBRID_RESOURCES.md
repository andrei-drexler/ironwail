# PluQ Hybrid Resource Loading Architecture

## Overview

PluQ supports **hybrid resource loading** where frontends can choose how to obtain resources:
- **Local loading** - Load from pak files (fast, requires filesystem)
- **Remote loading** - Request from backend via IPC (portable, no filesystem needed)
- **Hybrid** - Mix both approaches (cache locally, request missing resources)

## Protocol Flow

### 1. Map Load Start

**Backend → Frontend (via Resources channel):**
```
MapStart {
    mapname: "e1m1"
    num_textures: 47
    num_models: 12
    resources: [
        { id: 1, type: Texture, name: "wall01", size: 16384 }
        { id: 2, type: Texture, name: "floor03", size: 32768 }
        { id: 3, type: Model, name: "player.mdl", size: 4096 }
        ...
    ]
}
```

### 2. Frontend Decides How to Load

**Smart Frontend (has pak files):**
```c
void OnMapStart(MapStart msg) {
    for (resource in msg.resources) {
        if (CanLoadLocally(resource.name)) {
            // Load from local pak files
            LoadFromPak(resource.name, resource.id);
        } else {
            // Request from backend
            RequestResource(resource.id, resource.type);
        }
    }
}
```

**Dumb Frontend (no pak files):**
```c
void OnMapStart(MapStart msg) {
    for (resource in msg.resources) {
        // Always request from backend
        RequestResource(resource.id, resource.type);
    }
}
```

**Web Frontend (no filesystem):**
```c
async void OnMapStart(MapStart msg) {
    for (resource in msg.resources) {
        // Request and cache in IndexedDB
        if (!IsCached(resource.id)) {
            await RequestAndCache(resource.id);
        }
    }
}
```

### 3. Resource Request (Frontend → Backend)

**Frontend sends:**
```
ResourceRequest {
    resource_type: Texture
    resource_id: 1
    resource_name: "wall01"  // Optional
}
```

**Backend replies:**
```
ResourceMessage {
    type: Texture
    data: Texture {
        id: 1
        name: "wall01"
        width: 128
        height: 128
        format: RGBA
        pixels: [...]  // 128x128x4 = 65536 bytes
    }
}
```

### 4. Gameplay Channel (Always Uses IDs)

**Backend broadcasts every frame:**
```
GameplayFrame {
    entities: [
        Entity {
            origin: (100, 200, 50)
            angles: (0, 90, 0)
            model_id: 3        // ← Just the ID!
            texture_id: 1      // ← Just the ID!
            ...
        }
    ]
}
```

**Frontend renders using cached/loaded resources:**
```c
void RenderEntity(Entity entity) {
    Model* model = GetCachedModel(entity.model_id);
    Texture* texture = GetCachedTexture(entity.texture_id);

    // Render with OpenGL/DirectX/Vulkan
    DrawModel(model, texture, entity.origin, entity.angles);
}
```

## Use Cases

### Use Case 1: Unity Frontend (Smart)
```
Unity has Quake pak files in StreamingAssets/
→ MapStart received
→ Unity loads textures/models from pak files locally
→ Converts to Unity Texture2D and Mesh
→ Caches in memory
→ Gameplay channel just sends entity IDs
→ Unity renders using cached Unity assets
```

### Use Case 2: Web Frontend (Dumb)
```
Web browser has NO filesystem access
→ MapStart received with resource list
→ Frontend requests ALL resources via IPC
→ Caches in IndexedDB for future sessions
→ Gameplay channel sends entity IDs
→ WebGL renders using cached resources
```

### Use Case 3: Mobile Frontend (Hybrid)
```
Mobile app saves storage by not including pak files
→ MapStart received
→ Requests resources from backend
→ Caches in app storage
→ On next launch, uses cached resources
→ Only requests new/changed resources
```

### Use Case 4: Unreal Frontend (Smart + Custom)
```
Unreal has its own asset pipeline
→ MapStart received
→ Maps Quake resource IDs to Unreal asset paths
→ Loads high-res replacement textures
→ Uses custom shaders/materials
→ Gameplay channel sends IDs
→ Unreal renders with enhanced graphics
```

## Bandwidth Comparison

**Traditional (All Resources via IPC):**
```
Map load: ~50MB of textures/models/BSP data
Gameplay: ~10KB per frame (entity state)
```

**Hybrid (Local Loading):**
```
Map load: ~10KB (just resource lookup table)
Gameplay: ~10KB per frame (entity state)
```

**Savings: 99.98% reduction in map load bandwidth!**

## Implementation Modes

### Mode 1: Bulk Push (Simple Frontend)
```
Backend automatically sends all resources
Frontend just receives and caches
No requests needed
Good for: Simple/dumb frontends
```

### Mode 2: On-Demand Pull (Smart Frontend)
```
Frontend receives resource list
Frontend requests only what it needs
Backend replies to each request
Good for: Optimized/caching frontends
```

### Mode 3: Hybrid (Best of Both)
```
Backend pushes common resources
Frontend requests rare/missing resources
Progressive loading with fallbacks
Good for: Production frontends
```

## API Examples

### Backend (C):
```c
// Send resource lookup table
PluQ_SendMapStart("e1m1", resource_lookup);

// Wait for resource requests
ResourceRequest req;
while (PluQ_ReceiveResourceRequest(&req)) {
    void* data = LoadResource(req.resource_id);
    PluQ_SendResource(req.resource_id, data);
}

// Or push all resources automatically
PluQ_BulkPushResources(resource_list);
```

### Frontend (C#/Unity):
```csharp
void OnMapStart(MapStart msg) {
    foreach (var resource in msg.Resources) {
        if (File.Exists($"Assets/Quake/{resource.Name}")) {
            // Load locally
            LoadLocal(resource);
        } else {
            // Request from backend
            await RequestResource(resource.Id);
        }
    }
}

void OnGameplayFrame(GameplayFrame frame) {
    foreach (var entity in frame.Entities) {
        var model = modelCache[entity.ModelId];
        DrawModel(model, entity.Origin, entity.Angles);
    }
}
```

## Benefits

✅ **Flexibility** - Frontends choose local vs remote loading
✅ **Performance** - Local loading is much faster
✅ **Portability** - Dumb frontends work without pak files
✅ **Bandwidth** - Only send what's needed
✅ **Caching** - Frontends can cache resources
✅ **Progressive** - Start with remote, optimize to local
✅ **Testing** - Easy to test with minimal frontends

## Migration Path

**Phase 1: Full Remote (MVP)**
```
Backend sends all resources via IPC
Simple frontend implementation
Easy to test and debug
```

**Phase 2: Add Local Loading**
```
Frontend checks for pak files
Falls back to remote if missing
Best of both worlds
```

**Phase 3: Optimize**
```
Smart caching strategies
Predictive loading
Resource versioning
```
