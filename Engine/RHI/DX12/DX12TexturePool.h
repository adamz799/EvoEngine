#pragma once

#include "RHI/RHITypes.h"
#include "RHI/RHIResourcePool.h"
#include "RHI/DX12/DX12Common.h"
#include <vector>
#include <string>
#include <shared_mutex>

namespace Evo {

/// One slot in the texture pool.
struct DX12TextureEntry {
    ComPtr<ID3D12Resource> resource;
    std::string            debugName;
    uint16                 generation = 0;
    bool                   alive      = false;
    RHIBarrierState        barrierState;
};

/// Concrete texture pool. O(1) lookup, generation-based use-after-free protection.
///
/// Thread safety:
///   Reads  (GetResource, GetEntry, IsValid) — shared_lock
///   Writes (Allocate, Free)                 — unique_lock
class DX12TexturePool {
public:
    /// Allocate a slot, register a resource, return a handle.
    RHITextureHandle Allocate(ComPtr<ID3D12Resource> resource,
                              const std::string& name,
                              const RHIBarrierState& initialBarrier = {})
    {
        std::unique_lock lock(m_Mutex);

        uint64 index;
        if (!m_FreeList.empty()) {
            index = m_FreeList.back();
            m_FreeList.pop_back();
        } else {
            index = m_Entries.size();
            m_Entries.emplace_back();
        }

        auto& e        = m_Entries[index];
        e.resource     = std::move(resource);
        e.debugName    = name;
        e.alive        = true;
        e.barrierState = initialBarrier;

        RHITextureHandle h;
        h.Handle     = index;
        h.generation = e.generation;
        return h;
    }

    /// Free a slot. Bumps generation so stale handles are rejected.
    void Free(RHITextureHandle handle)
    {
        std::unique_lock lock(m_Mutex);
        auto* e = LookupUnlocked(handle);
        if (!e) return;

        e->alive = false;
        e->generation++;
        e->resource.Reset();
        e->debugName.clear();
        e->barrierState = {};
        m_FreeList.push_back(handle.Handle);
    }

    /// Get the native resource pointer directly. Returns nullptr if invalid.
    ID3D12Resource* GetResource(RHITextureHandle handle) const
    {
        std::shared_lock lock(m_Mutex);
        auto* e = LookupUnlocked(handle);
        return e ? e->resource.Get() : nullptr;
    }

    /// Get the full entry (for barrier state, debug name, etc.). Returns nullptr if invalid.
    DX12TextureEntry* GetEntry(RHITextureHandle handle)
    {
        std::shared_lock lock(m_Mutex);
        return LookupUnlocked(handle);
    }

    const DX12TextureEntry* GetEntry(RHITextureHandle handle) const
    {
        std::shared_lock lock(m_Mutex);
        return LookupUnlocked(handle);
    }

    bool IsValid(RHITextureHandle handle) const
    {
        std::shared_lock lock(m_Mutex);
        return LookupUnlocked(handle) != nullptr;
    }

private:
    DX12TextureEntry* LookupUnlocked(RHITextureHandle handle) const
    {
        if (handle.Handle >= m_Entries.size())
            return nullptr;
        auto& e = m_Entries[handle.Handle];
        if (!e.alive || e.generation != handle.generation)
            return nullptr;
        return const_cast<DX12TextureEntry*>(&e);
    }

    mutable std::shared_mutex         m_Mutex;
    mutable std::vector<DX12TextureEntry> m_Entries;
    std::vector<uint64>               m_FreeList;
};

} // namespace Evo
