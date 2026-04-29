#pragma once

#include "Core/Types.h"
#include <vector>
#include <cassert>

namespace Evo {

/// Lightweight entity handle — index + generation for use-after-free detection.
struct EntityHandle {
	uint32 uIndex      = UINT32_MAX;
	uint16 uGeneration = 0;

	bool IsValid() const { return uIndex != UINT32_MAX; }

	bool operator==(const EntityHandle&) const = default;
	bool operator!=(const EntityHandle&) const = default;
};

/// Sparse-set based component array. Dense storage for cache-friendly iteration,
/// sparse map for O(1) lookup by entity index.
template<typename T>
class ComponentArray {
public:
	/// Get component for entity, or nullptr if not present.
	T* Get(EntityHandle entity)
	{
		if (entity.uIndex >= m_vSparse.size())
			return nullptr;
		uint32 dense = m_vSparse[entity.uIndex];
		if (dense >= m_uCount || m_vDense[dense] != entity.uIndex)
			return nullptr;
		return &m_vData[dense];
	}

	const T* Get(EntityHandle entity) const
	{
		if (entity.uIndex >= m_vSparse.size())
			return nullptr;
		uint32 dense = m_vSparse[entity.uIndex];
		if (dense >= m_uCount || m_vDense[dense] != entity.uIndex)
			return nullptr;
		return &m_vData[dense];
	}

	/// Add component for entity. Entity must not already have this component.
	T& Add(EntityHandle entity, const T& value = T{})
	{
		assert(!Has(entity) && "Entity already has this component");

		if (entity.uIndex >= m_vSparse.size())
			m_vSparse.resize(entity.uIndex + 1, UINT32_MAX);

		uint32 denseIndex = m_uCount;
		if (denseIndex >= m_vData.size())
		{
			m_vData.push_back(value);
			m_vDense.push_back(entity.uIndex);
		}
		else
		{
			m_vData[denseIndex] = value;
			m_vDense[denseIndex] = entity.uIndex;
		}

		m_vSparse[entity.uIndex] = denseIndex;
		m_uCount++;
		return m_vData[denseIndex];
	}

	/// Remove component for entity.
	void Remove(EntityHandle entity)
	{
		if (!Has(entity))
			return;

		uint32 denseIndex = m_vSparse[entity.uIndex];
		uint32 lastIndex  = m_uCount - 1;

		if (denseIndex != lastIndex)
		{
			// Swap with last element
			m_vData[denseIndex]  = std::move(m_vData[lastIndex]);
			m_vDense[denseIndex] = m_vDense[lastIndex];
			m_vSparse[m_vDense[denseIndex]] = denseIndex;
		}

		m_vSparse[entity.uIndex] = UINT32_MAX;
		m_uCount--;
	}

	/// Check if entity has this component.
	bool Has(EntityHandle entity) const
	{
		if (entity.uIndex >= m_vSparse.size())
			return false;
		uint32 dense = m_vSparse[entity.uIndex];
		return dense < m_uCount && m_vDense[dense] == entity.uIndex;
	}

	/// Iterate all components. Callback signature: void(EntityHandle, T&)
	template<typename Fn>
	void ForEach(Fn&& fn)
	{
		for (uint32 i = 0; i < m_uCount; ++i)
		{
			EntityHandle handle;
			handle.uIndex = m_vDense[i];
			fn(handle, m_vData[i]);
		}
	}

	uint32 Count() const { return m_uCount; }

private:
	std::vector<T>      m_vData;      // Dense storage
	std::vector<uint32> m_vSparse;    // Entity index → dense index
	std::vector<uint32> m_vDense;     // Dense index → entity index
	uint32              m_uCount = 0;
};

} // namespace Evo
