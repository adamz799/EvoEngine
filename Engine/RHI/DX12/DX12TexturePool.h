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
    ComPtr<ID3D12Resource> pResource;
    std::string            sDebugName;
    uint16                 uGeneration = 0;
    bool                   bAlive      = false;
    RHIBarrierState        barrierState;
    RHIFormat              rhiFormat   = RHIFormat::Unknown;
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
                              RHIFormat rhiFormat = RHIFormat::Unknown,
                              const RHIBarrierState& initialBarrier = {})
    {
        std::unique_lock lock(m_Mutex);

        uint64 index;
        if (!m_vFreeList.empty()) {
            index = m_vFreeList.back();
            m_vFreeList.pop_back();
        } else {
            index = m_vEntries.size();
            m_vEntries.emplace_back();
        }

        auto& e        = m_vEntries[index];
        e.pResource    = std::move(resource);
        e.sDebugName   = name;
        e.bAlive       = true;
        e.barrierState = initialBarrier;
        e.rhiFormat    = rhiFormat;

        RHITextureHandle h;
        h.uHandle     = index;
        h.uGeneration = e.uGeneration;
        return h;
    }

    /// Free a slot. Bumps generation so stale handles are rejected.
    void Free(RHITextureHandle handle)
    {
        std::unique_lock lock(m_Mutex);
        auto* e = LookupUnlocked(handle);
        if (!e) return;

        e->bAlive = false;
        e->uGeneration++;
        e->pResource.Reset();
        e->sDebugName.clear();
        e->barrierState = {};
        e->rhiFormat    = RHIFormat::Unknown;
        m_vFreeList.push_back(handle.uHandle);
    }

    /// Get the native resource pointer directly. Returns nullptr if invalid.
    ID3D12Resource* GetResource(RHITextureHandle handle) const
    {
        std::shared_lock lock(m_Mutex);
        auto* e = LookupUnlocked(handle);
        return e ? e->pResource.Get() : nullptr;
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
        if (handle.uHandle >= m_vEntries.size())
            return nullptr;
        auto& e = m_vEntries[handle.uHandle];
        if (!e.bAlive || e.uGeneration != handle.uGeneration)
            return nullptr;
        return const_cast<DX12TextureEntry*>(&e);
    }

    mutable std::shared_mutex         m_Mutex;
    mutable std::vector<DX12TextureEntry> m_vEntries;
    std::vector<uint64>               m_vFreeList;
};

} // namespace Evo
