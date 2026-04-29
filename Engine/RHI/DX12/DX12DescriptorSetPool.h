#pragma once

#include "RHI/RHITypes.h"
#include "RHI/DX12/DX12GpuDescriptorAllocator.h"
#include <vector>
#include <shared_mutex>

namespace Evo {

struct DX12DescriptorSetEntry {
	RHIDescriptorSetLayoutHandle             layout;
	DX12GpuDescriptorAllocator::RangeAllocation allocation;
	uint16 uGeneration = 0;
	bool   bAlive      = false;
};

class DX12DescriptorSetPool {
public:
	RHIDescriptorSetHandle Allocate(RHIDescriptorSetLayoutHandle layout,
	                                DX12GpuDescriptorAllocator::RangeAllocation allocation)
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

		auto& e       = m_vEntries[index];
		e.layout       = layout;
		e.allocation   = allocation;
		e.bAlive       = true;

		RHIDescriptorSetHandle h;
		h.uHandle     = index;
		h.uGeneration = e.uGeneration;
		return h;
	}

	void Free(RHIDescriptorSetHandle handle)
	{
		std::unique_lock lock(m_Mutex);
		auto* e = LookupUnlocked(handle);
		if (!e) return;

		e->bAlive = false;
		e->uGeneration++;
		e->allocation = {};
		m_vFreeList.push_back(handle.uHandle);
	}

	DX12DescriptorSetEntry* GetEntry(RHIDescriptorSetHandle handle)
	{
		std::shared_lock lock(m_Mutex);
		return LookupUnlocked(handle);
	}

	const DX12DescriptorSetEntry* GetEntry(RHIDescriptorSetHandle handle) const
	{
		std::shared_lock lock(m_Mutex);
		return LookupUnlocked(handle);
	}

private:
	DX12DescriptorSetEntry* LookupUnlocked(RHIDescriptorSetHandle handle) const
	{
		if (handle.uHandle >= m_vEntries.size())
			return nullptr;
		auto& e = m_vEntries[handle.uHandle];
		if (!e.bAlive || e.uGeneration != handle.uGeneration)
			return nullptr;
		return const_cast<DX12DescriptorSetEntry*>(&e);
	}

	mutable std::shared_mutex                      m_Mutex;
	mutable std::vector<DX12DescriptorSetEntry>    m_vEntries;
	std::vector<uint64>                            m_vFreeList;
};

} // namespace Evo
