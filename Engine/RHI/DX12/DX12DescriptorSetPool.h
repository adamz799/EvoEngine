#pragma once

#include "RHI/RHITypes.h"
#include "RHI/DX12/DX12GpuDescriptorHeap.h"
#include <vector>
#include <shared_mutex>

namespace Evo {

struct DX12DescriptorSetEntry {
	RHIDescriptorSetLayoutHandle   layout;
	std::vector<uint32>            descriptorIndices;  // per-binding, in layout order
	uint16                         uGeneration = 0;
	bool                           bAlive      = false;

	// Staging cache — per-frame, avoids re-staging the same set
	uint64                                  stagingFrameFence = 0;
	DX12GpuDescriptorHeap::RingAllocation   stagedAllocation;
};

class DX12DescriptorSetPool {
public:
	RHIDescriptorSetHandle Allocate(RHIDescriptorSetLayoutHandle layout,
									std::vector<uint32>&& descriptorIndices)
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

			auto& e               = m_vEntries[index];
			e.layout              = layout;
			e.descriptorIndices   = std::move(descriptorIndices);
			e.stagingFrameFence   = 0;
			e.stagedAllocation    = {};
			e.bAlive              = true;

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
			e->descriptorIndices.clear();
			e->stagedAllocation = {};
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
