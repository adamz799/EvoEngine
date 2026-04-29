#pragma once

#include "RHI/RHITypes.h"
#include "RHI/RHIDescriptor.h"
#include <vector>
#include <shared_mutex>

namespace Evo {

struct DX12DescriptorSetLayoutEntry {
	std::vector<RHIDescriptorBinding> vBindings;
	uint32 uTotalDescriptorCount = 0;
	uint16 uGeneration = 0;
	bool   bAlive      = false;
};

class DX12DescriptorSetLayoutPool {
public:
	RHIDescriptorSetLayoutHandle Allocate(const RHIDescriptorSetLayoutDesc& desc)
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

		auto& e = m_vEntries[index];
		e.vBindings.assign(desc.pBindings, desc.pBindings + desc.uBindingCount);
		e.uTotalDescriptorCount = 0;
		for (uint32 i = 0; i < desc.uBindingCount; ++i)
			e.uTotalDescriptorCount += desc.pBindings[i].uCount;
		e.bAlive = true;

		RHIDescriptorSetLayoutHandle h;
		h.uHandle     = index;
		h.uGeneration = e.uGeneration;
		return h;
	}

	void Free(RHIDescriptorSetLayoutHandle handle)
	{
		std::unique_lock lock(m_Mutex);
		auto* e = LookupUnlocked(handle);
		if (!e) return;

		e->bAlive = false;
		e->uGeneration++;
		e->vBindings.clear();
		m_vFreeList.push_back(handle.uHandle);
	}

	DX12DescriptorSetLayoutEntry* GetEntry(RHIDescriptorSetLayoutHandle handle)
	{
		std::shared_lock lock(m_Mutex);
		return LookupUnlocked(handle);
	}

	const DX12DescriptorSetLayoutEntry* GetEntry(RHIDescriptorSetLayoutHandle handle) const
	{
		std::shared_lock lock(m_Mutex);
		return LookupUnlocked(handle);
	}

private:
	DX12DescriptorSetLayoutEntry* LookupUnlocked(RHIDescriptorSetLayoutHandle handle) const
	{
		if (handle.uHandle >= m_vEntries.size())
			return nullptr;
		auto& e = m_vEntries[handle.uHandle];
		if (!e.bAlive || e.uGeneration != handle.uGeneration)
			return nullptr;
		return const_cast<DX12DescriptorSetLayoutEntry*>(&e);
	}

	mutable std::shared_mutex                           m_Mutex;
	mutable std::vector<DX12DescriptorSetLayoutEntry>   m_vEntries;
	std::vector<uint64>                                 m_vFreeList;
};

} // namespace Evo
