#pragma once

#include "RHI/RHITypes.h"
#include "RHI/DX12/DX12Common.h"
#include <vector>
#include <string>
#include <shared_mutex>

namespace Evo {

struct DX12PipelineEntry {
	ComPtr<ID3D12PipelineState>  pso;
	ComPtr<ID3D12RootSignature>  rootSignature;
	RHIPrimitiveTopology         topology = RHIPrimitiveTopology::TriangleList;
	std::string                  debugName;
	uint16                       generation = 0;
	bool                         alive      = false;
};

class DX12PipelinePool {
public:
	RHIPipelineHandle Allocate(ComPtr<ID3D12PipelineState> pso,
	                           ComPtr<ID3D12RootSignature> rootSig,
	                           RHIPrimitiveTopology topology,
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

		auto& e         = m_Entries[index];
		e.pso           = std::move(pso);
		e.rootSignature = std::move(rootSig);
		e.topology      = topology;
		e.debugName     = name;
		e.alive         = true;

		RHIPipelineHandle h;
		h.Handle     = index;
		h.generation = e.generation;
		return h;
	}

	void Free(RHIPipelineHandle handle)
	{
		std::unique_lock lock(m_Mutex);
		auto* e = LookupUnlocked(handle);
		if (!e) return;

		e->alive = false;
		e->generation++;
		e->pso.Reset();
		e->rootSignature.Reset();
		e->debugName.clear();
		m_FreeList.push_back(handle.Handle);
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
		if (handle.Handle >= m_Entries.size())
			return nullptr;
		auto& e = m_Entries[handle.Handle];
		if (!e.alive || e.generation != handle.generation)
			return nullptr;
		return const_cast<DX12PipelineEntry*>(&e);
	}

	mutable std::shared_mutex                m_Mutex;
	mutable std::vector<DX12PipelineEntry>   m_Entries;
	std::vector<uint64>                      m_FreeList;
};

} // namespace Evo
