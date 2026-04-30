#include "Asset/AssetLoader.h"
#include "Asset/Asset.h"
#include "Platform/FileSystem.h"
#include "Core/Log.h"

#include <algorithm>

namespace Evo {

void AssetLoader::Initialize(RHIDevice* pDevice, uint32 uNumThreads)
{
	m_pDevice = pDevice;
	m_bShutdown = false;

	if (uNumThreads == 0)
	{
		uint32 hw = std::thread::hardware_concurrency();
		uNumThreads = std::clamp(hw / 4, 1u, 4u);
	}

	EVO_LOG_INFO("AssetLoader: starting {} worker thread(s)", uNumThreads);

	m_vWorkers.reserve(uNumThreads);
	for (uint32 i = 0; i < uNumThreads; ++i)
		m_vWorkers.emplace_back(&AssetLoader::WorkerLoop, this);
}

void AssetLoader::Shutdown()
{
	{
		std::lock_guard lock(m_QueueMutex);
		m_bShutdown = true;
	}
	m_QueueCV.notify_all();

	for (auto& t : m_vWorkers)
	{
		if (t.joinable())
			t.join();
	}
	m_vWorkers.clear();

	// Clear remaining state
	while (!m_Queue.empty())
		m_Queue.pop();
	m_vCompleted.clear();
	m_CancelledHandles.clear();
}

void AssetLoader::Submit(AssetHandle handle, Asset* pAsset,
						 const std::string& sPath, AssetPriority priority)
{
	{
		std::lock_guard lock(m_QueueMutex);
		m_Queue.push(LoadTask{ handle, pAsset, sPath, priority });
	}
	m_QueueCV.notify_one();
}

void AssetLoader::Cancel(AssetHandle handle)
{
	std::lock_guard lock(m_CancelMutex);
	m_CancelledHandles.insert(PackHandle(handle));
}

std::vector<AssetLoader::CompletedTask> AssetLoader::DrainCompleted()
{
	std::vector<CompletedTask> result;
	{
		std::lock_guard lock(m_CompletionMutex);
		result.swap(m_vCompleted);
	}
	return result;
}

void AssetLoader::WorkerLoop()
{
	while (true)
	{
		LoadTask task;

		// Wait for work
		{
			std::unique_lock lock(m_QueueMutex);
			m_QueueCV.wait(lock, [this] { return m_bShutdown || !m_Queue.empty(); });

			if (m_bShutdown && m_Queue.empty())
				return;

			task = std::move(const_cast<LoadTask&>(m_Queue.top()));
			m_Queue.pop();
		}

		// Check cancellation
		{
			std::lock_guard lock(m_CancelMutex);
			uint64 packed = PackHandle(task.handle);
			auto it = m_CancelledHandles.find(packed);
			if (it != m_CancelledHandles.end())
			{
				m_CancelledHandles.erase(it);
				std::lock_guard compLock(m_CompletionMutex);
				m_vCompleted.push_back({ task.handle, false, true });
				continue;
			}
		}

		// Read file + parse + create GPU resources
		auto rawData = FileSystem::ReadBinary(task.sPath);
		bool bSuccess = !rawData.empty() && task.pAsset->OnLoad(rawData);
		if (bSuccess)
			bSuccess = task.pAsset->OnFinalize(m_pDevice);

		// Push result
		{
			std::lock_guard lock(m_CompletionMutex);
			m_vCompleted.push_back({ task.handle, bSuccess, false });
		}
	}
}

} // namespace Evo

