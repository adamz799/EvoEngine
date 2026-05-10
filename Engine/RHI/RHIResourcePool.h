#pragma once

#include "Core/Types.h"
#include "RHI/RHITypes.h"
#include <shared_mutex>

namespace Evo
{

	// ============================================================================
	// CPU-side barrier state tracking
	// ============================================================================

	/// Tracks the most recent barrier state of a resource on the CPU side.
	/// Used for validation (detecting layoutBefore mismatches) and potential
	/// auto-fill of barrier "before" fields in future phases.
	struct RHIBarrierState {
		RHIBarrierSync   currentSync = RHIBarrierSync::None;
		RHIBarrierAccess currentAccess = RHIBarrierAccess::Common;
		RHITextureLayout currentLayout = RHITextureLayout::Undefined;
	};

	// ============================================================================
	// Generic resource pool — typed slots with free-list reuse and generation-based
	// use-after-free protection.
	// ============================================================================
	//
	// TEntry must provide:
	//   - uint16 uGeneration  (initialized to 0)
	//   - bool   bAlive       (initialized to false)
	//
	// Thread safety:
	//   Reads  (GetEntry, IsValid) — shared_lock
	//   Writes (Allocate, Free)    — unique_lock
	//
	// The pool owns NO backend resources. Allocate() takes a pre-filled entry;
	// Free() returns it — the caller handles backend resource destruction.

	/// O(1) lookup, generation-based use-after-free protection.
	///
	/// Thread safety:
	///   Reads  (GetResource, GetEntry, IsValid) — shared_lock
	///   Writes (Allocate, Free)                 — unique_lock
	template<typename THandleTag, typename TEntry>
	class RHIResourcePool {
	public:
		using Handle = RHIHandle<THandleTag>;

		/// Take ownership of a pre-constructed entry. Returns a typed handle.
		Handle Allocate(TEntry&& entry)
		{
			std::unique_lock lock(m_Mutex);

			uint64 index;
			if (!m_vFreeList.empty())
			{
				index = m_vFreeList.back();
				m_vFreeList.pop_back();
			}
			else
			{
				index = m_vEntries.size();
				m_vEntries.emplace_back();
			}

			m_vEntries[index] = std::move(entry);
			m_vEntries[index].bAlive = true;

			Handle h;
			h.uHandle = index;
			h.uGeneration = m_vEntries[index].uGeneration;
			return h;
		}

		/// Release the slot and return the entry. Caller owns the returned
		/// entry's backend resources — destroy immediately, or hold for
		/// deferred destruction. The slot is recycled immediately so the
		/// handle becomes stale (generation bumped).
		TEntry Free(Handle handle)
		{
			std::unique_lock lock(m_Mutex);
			auto* e = LookupUnlocked(handle);
			if (!e)
				return {};

			e->bAlive = false;
			e->uGeneration++;

			TEntry result = std::move(*e);
			*e = {};                                 // reset slot
			e->uGeneration = result.uGeneration;     // preserve bumped gen

			m_vFreeList.push_back(handle.uHandle);
			return result;
		}

		/// Non-owning mutable access. Returns nullptr if handle is stale.
		TEntry* GetEntry(Handle handle)
		{
			std::shared_lock lock(m_Mutex);
			return LookupUnlocked(handle);
		}

		/// Non-owning const access.
		const TEntry* GetEntry(Handle handle) const
		{
			std::shared_lock lock(m_Mutex);
			return LookupUnlocked(handle);
		}

		/// Check whether a handle refers to a live slot.
		bool IsValid(Handle handle) const
		{
			std::shared_lock lock(m_Mutex);
			return LookupUnlocked(handle) != nullptr;
		}

	private:
		TEntry* LookupUnlocked(Handle handle) const
		{
			if (handle.uHandle >= m_vEntries.size())
				return nullptr;
			auto& entry = m_vEntries[handle.uHandle];
			if (!entry.bAlive || entry.uGeneration != handle.uGeneration)
				return nullptr;
			return const_cast<TEntry*>(&entry);
		}

		mutable std::shared_mutex   m_Mutex;
		mutable std::vector<TEntry> m_vEntries;
		std::vector<uint64>         m_vFreeList;
	};

	// ============================================================================
	// Deferred destruction queue — holds freed pool entries until GPU fence passes.
	// ============================================================================
	//
	// Lifecycle (mirrors CommandListPool):
	//   Enqueue(entry)              — called during frame (DestroyXxx)
	//   StampPending(fenceValue)    — EndFrame: stamp + move to m_Deferred
	//   ReleaseCompleted(fenceVal)  — BeginFrame: destroy entries with fence ≤ value
	//   Drain()                     — WaitIdle/Shutdown: release everything
	//
	// TEntry must be destructible (destructor releases backend resources).
	template<typename TEntry>
	class RHIDeferredDestroyQueue {
	public:
		/// Take ownership of a freed pool entry. Called during the frame.
		void Enqueue(TEntry&& entry)
		{
			m_Pending.push_back(std::move(entry));
		}

		/// Stamp all pending entries with this frame's fence and move to deferred.
		void StampPending(uint64 fenceValue)
		{
			for (auto& entry : m_Pending)
				m_Deferred.push_back({ std::move(entry), fenceValue });
			m_Pending.clear();
		}

		/// Destroy entries whose fence has been reached by the GPU.
		void ReleaseCompleted(uint64 completedFenceValue)
		{
			size_t writeIdx = 0;
			for (size_t i = 0; i < m_Deferred.size(); ++i)
			{
				if (m_Deferred[i].fenceValue <= completedFenceValue)
					continue; // destroyed
				if (i != writeIdx)
					m_Deferred[writeIdx] = std::move(m_Deferred[i]);
				++writeIdx;
			}
			m_Deferred.resize(writeIdx);
		}

		/// Release all entries immediately (no fence check).
		void Drain()
		{
			m_Pending.clear();
			m_Deferred.clear();
		}

	private:
		struct Stamped {
			TEntry entry;
			uint64 fenceValue;
		};
		std::vector<TEntry>  m_Pending;
		std::vector<Stamped> m_Deferred;
	};

} // namespace Evo

