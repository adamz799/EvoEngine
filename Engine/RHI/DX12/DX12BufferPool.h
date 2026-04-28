#pragma once

#include "RHI/RHITypes.h"
#include "RHI/DX12/DX12Common.h"
#include <D3D12MemAlloc.h>
#include <vector>
#include <string>
#include <shared_mutex>

namespace Evo {

struct DX12BufferEntry {
	ComPtr<ID3D12Resource>    resource;
	D3D12MA::Allocation*      allocation = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
	uint64                    size       = 0;
	void*                     mappedPtr  = nullptr;
	std::string               debugName;
	uint16                    generation = 0;
	bool                      alive      = false;
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
		if (!m_FreeList.empty()) {
			index = m_FreeList.back();
			m_FreeList.pop_back();
		} else {
			index = m_Entries.size();
			m_Entries.emplace_back();
		}

		auto& e       = m_Entries[index];
		e.resource    = std::move(resource);
		e.allocation  = allocation;
		e.gpuAddress  = gpuAddress;
		e.size        = size;
		e.mappedPtr   = mappedPtr;
		e.debugName   = name;
		e.alive       = true;

		RHIBufferHandle h;
		h.Handle     = index;
		h.generation = e.generation;
		return h;
	}

	void Free(RHIBufferHandle handle)
	{
		std::unique_lock lock(m_Mutex);
		auto* e = LookupUnlocked(handle);
		if (!e) return;

		e->alive = false;
		e->generation++;
		e->resource.Reset();
		if (e->allocation) { e->allocation->Release(); e->allocation = nullptr; }
		e->gpuAddress = 0;
		e->size       = 0;
		e->mappedPtr  = nullptr;
		e->debugName.clear();
		m_FreeList.push_back(handle.Handle);
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
		if (handle.Handle >= m_Entries.size())
			return nullptr;
		auto& e = m_Entries[handle.Handle];
		if (!e.alive || e.generation != handle.generation)
			return nullptr;
		return const_cast<DX12BufferEntry*>(&e);
	}

	mutable std::shared_mutex            m_Mutex;
	mutable std::vector<DX12BufferEntry> m_Entries;
	std::vector<uint64>                  m_FreeList;
};

} // namespace Evo
