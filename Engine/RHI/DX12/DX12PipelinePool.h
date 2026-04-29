#pragma once

#include "RHI/RHITypes.h"
#include "RHI/DX12/DX12Common.h"
#include <vector>
#include <string>
#include <shared_mutex>

namespace Evo {

struct DX12PipelineEntry {
	ComPtr<ID3D12PipelineState>  pPso;
	ComPtr<ID3D12RootSignature>  pRootSignature;
	RHIPrimitiveTopology         topology = RHIPrimitiveTopology::TriangleList;
	uint32                       uPushConstantSize = 0;
	uint32                       uDescriptorTableRootOffset = 0;
	std::string                  sDebugName;
	uint16                       uGeneration = 0;
	bool                         bAlive      = false;
};

class DX12PipelinePool {
public:
	RHIPipelineHandle Allocate(ComPtr<ID3D12PipelineState> pso,
	                           ComPtr<ID3D12RootSignature> rootSig,
	                           RHIPrimitiveTopology topology,
	                           uint32 pushConstantSize,
	                           uint32 descTableRootOffset,
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

		auto& e          = m_vEntries[index];
		e.pPso           = std::move(pso);
		e.pRootSignature = std::move(rootSig);
		e.topology       = topology;
		e.uPushConstantSize = pushConstantSize;
		e.uDescriptorTableRootOffset = descTableRootOffset;
		e.sDebugName     = name;
		e.bAlive         = true;

		RHIPipelineHandle h;
		h.uHandle     = index;
		h.uGeneration = e.uGeneration;
		return h;
	}

	void Free(RHIPipelineHandle handle)
	{
		std::unique_lock lock(m_Mutex);
		auto* e = LookupUnlocked(handle);
		if (!e) return;

		e->bAlive = false;
		e->uGeneration++;
		e->pPso.Reset();
		e->pRootSignature.Reset();
		e->sDebugName.clear();
		m_vFreeList.push_back(handle.uHandle);
	}

	DX12PipelineEntry* GetEntry(RHIPipelineHandle handle)
	{
		std::shared_lock lock(m_Mutex);
		return LookupUnlocked(handle);
	}

	const DX12PipelineEntry* GetEntry(RHIPipelineHandle handle) const
	{
		std::shared_lock lock(m_Mutex);
		return LookupUnlocked(handle);
	}

	bool IsValid(RHIPipelineHandle handle) const
	{
		std::shared_lock lock(m_Mutex);
		return LookupUnlocked(handle) != nullptr;
	}

private:
	DX12PipelineEntry* LookupUnlocked(RHIPipelineHandle handle) const
	{
		if (handle.uHandle >= m_vEntries.size())
			return nullptr;
		auto& e = m_vEntries[handle.uHandle];
		if (!e.bAlive || e.uGeneration != handle.uGeneration)
			return nullptr;
		return const_cast<DX12PipelineEntry*>(&e);
	}

	mutable std::shared_mutex                m_Mutex;
	mutable std::vector<DX12PipelineEntry>   m_vEntries;
	std::vector<uint64>                      m_vFreeList;
};

} // namespace Evo
