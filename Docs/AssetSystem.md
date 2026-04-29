# Asset System

## Design Philosophy

### Handle + Generation

All engine subsystems (RHI, ECS, Asset) use the same `{uint32 index, uint16 generation}` handle pattern. This gives:

- **O(1) lookup** — direct array index, no hash map.
- **Use-after-free detection** — generation mismatch catches stale handles.
- **Stable identity** — handles survive container reallocation, unlike pointers.
- **Consistency** — `RHIBufferHandle`, `EntityHandle`, `AssetHandle` share the same mental model and code patterns.

### Worker Threads Create GPU Resources Directly

In many engines, GPU resource creation is confined to the main thread. EvoEngine takes a different approach:

- **DX12 resource creation APIs** (`CreateCommittedResource`, `CreateGraphicsPipelineState`, etc.) are **thread-safe by specification**.
- **Engine resource pools** (`DX12BufferPool`, `DX12TexturePool`, `DX12ShaderPool`, `DX12PipelinePool`) protect their internal state with `std::shared_mutex`.
- Therefore, `OnLoad` (CPU parsing) and `OnFinalize` (GPU resource creation) both execute on worker threads. The main thread `Update()` only marks state as Loaded — zero blocking work.

This simplifies the architecture (no two-phase main-thread finalization queue) and maximizes parallelism.

### Priority Queue, Not FIFO

Assets have an `AssetPriority` (Critical → Background). A `std::priority_queue` ensures that high-priority resources (e.g. a shader needed for the current frame) are loaded before low-priority background prefetch. Without priorities, a burst of background loads could delay a critical asset.

### Factory Registration by Extension

`AssetManager::RegisterFactory(".hlsl", ...)` maps file extensions to factory functions. Adding a new asset type requires zero modification to existing code — just register a new factory. The factory returns a `std::unique_ptr<Asset>`, and the manager handles lifetime from there.

---

## Architecture

```
User Code
    |
    v
AssetManager  ---- Registry (Handle -> Entry) + Path Dedup + Factory Map
    |
    |--- LoadSync(): ReadBinary -> OnLoad -> OnFinalize (calling thread)
    |
    '--- Load():
         |  Submit to AssetLoader (priority queue)
         v
    AssetLoader (N workers, N = clamp(hardware_concurrency/4, 1, 4))
         |  FileSystem::ReadBinary -> Asset::OnLoad -> Asset::OnFinalize
         |  -> completion queue
         v
    AssetManager::Update() (main thread, per frame)
         |  Drain completion queue -> mark state = Loaded
         v
    User queries via Get<T>(handle)
```

---

## Lifecycle

### State Machine

```
        Load() / LoadSync()
              |
              v
    +-------------------+
    |     Unloaded      |<--- Unload() / Cancel()
    +--------+----------+
             | Submit to loader
             v
    +-------------------+
    |      Queued       |---- Cancel() -> Unloaded
    +--------+----------+
             | worker picks up
             v
    +-------------------+
    |     Loading       |  worker: ReadBinary + OnLoad + OnFinalize
    +--------+----------+
             | done
             v
       +-----+-----+
       |           |
    success     failure
       |           |
       v           v
  +--------+  +--------+
  | Loaded |  | Failed |
  +--------+  +--------+
       ^
       |
    Reload() (success)
```

### Hook Signatures

| Hook | Thread | Purpose |
|------|--------|---------|
| `OnLoad(rawData)` | Worker | Parse raw file bytes (CPU work) |
| `OnFinalize(device)` | Worker | Create GPU resources (thread-safe) |
| `OnUnload(device)` | Main | Release all resources |
| `OnReload(rawData, device)` | Main | Hot-reload; default = unload + load + finalize |

---

## Thread Model

### Worker Count

```
N = clamp(std::thread::hardware_concurrency() / 4, 1, 4)
```

- **Minimum 1**: always have async capability.
- **Maximum 4**: asset loading is I/O-bound (disk read), not CPU-bound. More threads don't help and waste memory.
- **1/4 of cores**: leaves the majority for rendering, game logic, and the OS.

### Thread Safety

| Operation | Thread | Protected By |
|-----------|--------|-------------|
| Submit / Cancel | Any | `m_QueueMutex` / `m_CancelMutex` |
| DrainCompleted | Main | `m_CompletionMutex` (swap pattern) |
| ReadBinary | Worker | No shared state |
| OnLoad / OnFinalize | Worker | DX12 resource pools use `shared_mutex` |
| OnUnload / OnReload | Main | Caller's responsibility |
| AllocateEntry / FreeEntry | Any | `m_RegistryMutex` |

The completion queue uses the **swap pattern**: the main thread swaps `m_vCompleted` with an empty vector under a brief lock, then processes results without holding any lock.

---

## Usage Example

```cpp
// -- Setup --
AssetManager manager;
manager.Initialize(device);
manager.RegisterFactory(".hlsl", []{ return std::make_unique<ShaderAsset>(); });
manager.RegisterFactory(".obj",  []{ return std::make_unique<MeshAsset>(); });

// -- Async load --
AssetHandle shader = manager.Load("Assets/Shaders/PBR.hlsl", AssetPriority::High);
AssetHandle mesh   = manager.Load("Assets/Models/Character.obj");

// -- Main loop --
while (running) {
    manager.Update();  // drain async completions

    if (auto* s = manager.Get<ShaderAsset>(shader)) {
        // shader is ready
    }

    if (manager.GetState(mesh) == AssetState::Failed) {
        // handle error
    }
}

// -- Sync load (blocking) --
AssetHandle tex = manager.LoadSync("Assets/Textures/Default.dds");

// -- Hot-reload (editor mode) --
manager.CheckHotReload();  // call periodically

// -- Cleanup --
manager.Shutdown();
```

---

## File Structure

```
Engine/Asset/
    AssetTypes.h      -- AssetHandle, AssetState, AssetPriority
    Asset.h           -- Base class with lifecycle hooks
    AssetLoader.h     -- Thread pool + priority queue (declaration)
    AssetLoader.cpp   -- Worker loop, submit, cancel, drain
    AssetManager.h    -- Registry, factory, load/query/unload API
    AssetManager.cpp  -- All AssetManager implementation
```
