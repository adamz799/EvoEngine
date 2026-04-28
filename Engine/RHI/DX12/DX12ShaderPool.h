#pragma once

#include "RHI/RHITypes.h"
#include <vector>
#include <string>
#include <shared_mutex>

namespace Evo {

struct DX12ShaderEntry {
	std::vector<uint8> bytecode;
	RHIShaderStage     stage = RHIShaderStage::Vertex;
	std::string        debugName;
	uint16             generation = 0;
	bool               alive      = false;
};

class DX12ShaderPool {
public:
	RHIShaderHandle Allocate(const void* bytecodeData, uint64 bytecodeSize,
	                         RHIShaderStage stage, const std::string& name)
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

		auto& e     = m_Entries[index];
		auto* src   = static_cast<const uint8*>(bytecodeData);
		e.bytecode.assign(src, src + bytecodeSize);
		e.stage     = stage;
		e.debugName = name;
		e.alive     = true;

		RHIShaderHandle h;
		h.Handle     = index;
		h.generation = e.generation;
		return h;
	}

	void Free(RHIShaderHandle handle)
	{
		std::unique_lock lock(m_Mutex);
		auto* e = LookupUnlocked(handle);
		if (!e) return;

		e->alive = false;
		e->generation++;
		e->bytecode.clear();
		e->debugName.clear();
		m_FreeList.push_back(handle.Handle);
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
		if (handle.Handle >= m_Entries.size())
			return nullptr;
		auto& e = m_Entries[handle.Handle];
		if (!e.alive || e.generation != handle.generation)
			return nullptr;
		return const_cast<DX12ShaderEntry*>(&e);
	}

	mutable std::shared_mutex              m_Mutex;
	mutable std::vector<DX12ShaderEntry>   m_Entries;
	std::vector<uint64>                    m_FreeList;
};

} // namespace Evo
