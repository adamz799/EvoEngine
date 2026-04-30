#pragma once

#include "Asset/AssetTypes.h"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <unordered_set>
#include <string>

namespace Evo {

class Asset;
class RHIDevice;

/// Async asset loader — worker thread pool with priority queue.
///
/// Workers execute ReadBinary + OnLoad + OnFinalize entirely on their thread.
/// DX12 resource creation is thread-safe, so no main-thread finalization needed.
/// Completed tasks are drained by the main thread via DrainCompleted().
class AssetLoader {
public:
	/// Start worker threads.
	/// uNumThreads = 0 → auto: clamp(hardware_concurrency/4, 1, 4).
	void Initialize(RHIDevice* pDevice, uint32 uNumThreads = 0);

	/// Signal shutdown and join all workers.
	void Shutdown();

	/// Enqueue an async load task.
	void Submit(AssetHandle handle, Asset* pAsset,
				const std::string& sPath, AssetPriority priority);

	/// Cancel a queued (not yet started) task.
	void Cancel(AssetHandle handle);

	/// Result of a completed load task.
	struct CompletedTask {
		AssetHandle handle;
		bool        bSuccess;
		bool        bCancelled = false;
	};

	/// Drain all completed tasks (swap, minimal lock). Called on main thread.
	std::vector<CompletedTask> DrainCompleted();

private:
	struct LoadTask {
		AssetHandle   handle;
		Asset*        pAsset;
		std::string   sPath;
		AssetPriority priority;

		bool operator>(const LoadTask& other) const
		{
			return static_cast<uint8>(priority) > static_cast<uint8>(other.priority);
		}
	};

	std::vector<std::thread> m_vWorkers;
	std::priority_queue<LoadTask, std::vector<LoadTask>, std::greater<LoadTask>> m_Queue;
	std::mutex              m_QueueMutex;
	std::condition_variable m_QueueCV;
	std::atomic<bool>       m_bShutdown = false;

	RHIDevice*              m_pDevice = nullptr;

	std::mutex              m_CompletionMutex;
	std::vector<CompletedTask> m_vCompleted;

	std::mutex              m_CancelMutex;
	std::unordered_set<uint64> m_CancelledHandles;  // packed index+gen

	void WorkerLoop();

	static uint64 PackHandle(AssetHandle h)
	{
		return (static_cast<uint64>(h.uGeneration) << 32) | h.uIndex;
	}
};

} // namespace Evo

