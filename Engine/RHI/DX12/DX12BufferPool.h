#pragma once

#include "RHI/RHITypes.h"
#include "RHI/DX12/DX12Common.h"
#include <D3D12MemAlloc.h>
#include <vector>
#include <string>
#include <shared_mutex>

namespace Evo {

struct DX12BufferEntry {
	ComPtr<ID3D12Resource>    pResource;
	D3D12MA::Allocation*      pAllocation = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS uGpuAddress = 0;
	uint64                    uSize       = 0;
	void*                     pMappedPtr  = nullptr;
	std::string               sDebugName;
	uint16                    uGeneration = 0;
	bool                      bAlive      = false;
};

class DX12BufferPool {
public:
	RHIBufferHandle Allocate(ComPtr<ID3D12Resource> resource,
	                         D3D12MA::Allocation* allocation,
	                         D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
	                         uint64 size, void* mappedPtr,
	                         const std::string& name)
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
		e.pAllocation  = allocation;
		e.uGpuAddress  = gpuAddress;
		e.uSize        = size;
		e.pMappedPtr   = mappedPtr;
		e.sDebugName   = name;
		e.bAlive       = true;

		RHIBufferHandle h;
		h.uHandle     = index;
		h.uGeneration = e.uGeneration;
		return h;
	}

	void Free(RHIBufferHandle handle)
	{
		std::unique_lock lock(m_Mutex);
		auto* e = LookupUnlocked(handle);
		if (!e) return;

		e->bAlive = false;
		e->uGeneration++;
		e->pResource.Reset();
		if (e->pAllocation) { e->pAllocation->Release(); e->pAllocation = nullptr; }
		e->uGpuAddress = 0;
		e->uSize       = 0;
		e->pMappedPtr  = nullptr;
		e->sDebugName.clear();
		m_vFreeList.push_back(handle.uHandle);
	}

	DX12BufferEntry* GetEntry(RHIBufferHandle handle)
	{
		std::shared_lock lock(m_Mutex);
		return LookupUnlocked(handle);
	}

	const DX12BufferEntry* GetEntry(RHIBufferHandle handle) const
	{
		std::shared_lock lock(m_Mutex);
		return LookupUnlocked(handle);
	}

	bool IsValid(RHIBufferHandle handle) const
	{
		std::shared_lock lock(m_Mutex);
		return LookupUnlocked(handle) != nullptr;
	}

private:
	DX12BufferEntry* LookupUnlocked(RHIBufferHandle handle) const
	{
		if (handle.uHandle >= m_vEntries.size())
			return nullptr;
		auto& e = m_vEntries[handle.uHandle];
		if (!e.bAlive || e.uGeneration != handle.uGeneration)
			return nullptr;
		return const_cast<DX12BufferEntry*>(&e);
	}

	mutable std::shared_mutex            m_Mutex;
	mutable std::vector<DX12BufferEntry> m_vEntries;
	std::vector<uint64>                  m_vFreeList;
};

} // namespace Evo
