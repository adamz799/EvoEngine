#pragma once

#include "RHI/RHITypes.h"
#include <vector>
#include <string>
#include <shared_mutex>

namespace Evo {

struct DX12ShaderEntry {
	std::vector<uint8> vBytecode;
	RHIShaderStage     stage = RHIShaderStage::Vertex;
	std::string        sDebugName;
	uint16             uGeneration = 0;
	bool               bAlive      = false;
};

class DX12ShaderPool {
public:
	RHIShaderHandle Allocate(const void* bytecodeData, uint64 bytecodeSize,
							 RHIShaderStage stage, const std::string& name)
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

		auto& e     = m_vEntries[index];
		auto* src   = static_cast<const uint8*>(bytecodeData);
		e.vBytecode.assign(src, src + bytecodeSize);
		e.stage     = stage;
		e.sDebugName = name;
		e.bAlive    = true;

		RHIShaderHandle h;
		h.uHandle     = index;
		h.uGeneration = e.uGeneration;
		return h;
	}

	void Free(RHIShaderHandle handle)
	{
		std::unique_lock lock(m_Mutex);
		auto* e = LookupUnlocked(handle);
		if (!e) return;

		e->bAlive = false;
		e->uGeneration++;
		e->vBytecode.clear();
		e->sDebugName.clear();
		m_vFreeList.push_back(handle.uHandle);
	}

	DX12ShaderEntry* GetEntry(RHIShaderHandle handle)
	{
		std::shared_lock lock(m_Mutex);
		return LookupUnlocked(handle);
	}

	const DX12ShaderEntry* GetEntry(RHIShaderHandle handle) const
	{
		std::shared_lock lock(m_Mutex);
		return LookupUnlocked(handle);
	}

	bool IsValid(RHIShaderHandle handle) const
	{
		std::shared_lock lock(m_Mutex);
		return LookupUnlocked(handle) != nullptr;
	}

private:
	DX12ShaderEntry* LookupUnlocked(RHIShaderHandle handle) const
	{
		if (handle.uHandle >= m_vEntries.size())
			return nullptr;
		auto& e = m_vEntries[handle.uHandle];
		if (!e.bAlive || e.uGeneration != handle.uGeneration)
			return nullptr;
		return const_cast<DX12ShaderEntry*>(&e);
	}

	mutable std::shared_mutex              m_Mutex;
	mutable std::vector<DX12ShaderEntry>   m_vEntries;
	std::vector<uint64>                    m_vFreeList;
};

} // namespace Evo

