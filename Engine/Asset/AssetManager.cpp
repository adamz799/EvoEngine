#include "Asset/AssetManager.h"
#include "Asset/Asset.h"
#include "Platform/FileSystem.h"
#include "Core/Log.h"

#include <algorithm>

namespace Evo {

void AssetManager::Initialize(RHIDevice* pDevice)
{
	m_pDevice = pDevice;
	m_Loader.Initialize(pDevice);
	EVO_LOG_INFO("AssetManager initialized");
}

void AssetManager::Shutdown()
{
	m_Loader.Shutdown();

	// Unload all alive assets
	for (auto& entry : m_vEntries)
	{
		if (entry.bAlive && entry.pAsset && entry.state == AssetState::Loaded)
			entry.pAsset->OnUnload(m_pDevice);
	}

	m_vEntries.clear();
	m_vFreeList.clear();
	m_PathMap.clear();
	m_Factories.clear();
	m_pDevice = nullptr;

	EVO_LOG_INFO("AssetManager shut down");
}

void AssetManager::RegisterFactory(const std::string& sExtension, AssetFactory factory)
{
	m_Factories[sExtension] = std::move(factory);
}

AssetHandle AssetManager::Load(const std::string& sPath, AssetPriority priority)
{
	std::string normalized = NormalizePath(sPath);

	// Check deduplication
	{
		std::shared_lock lock(m_RegistryMutex);
		auto it = m_PathMap.find(normalized);
		if (it != m_PathMap.end())
			return it->second;
	}

	// Find factory
	std::string ext = GetExtension(normalized);
	auto factoryIt = m_Factories.find(ext);
	if (factoryIt == m_Factories.end())
	{
		EVO_LOG_ERROR("AssetManager::Load — no factory for extension '{}'", ext);
		return {};
	}

	auto pAsset = factoryIt->second();
	Asset* pRaw = pAsset.get();
	AssetHandle handle = AllocateEntry(normalized, std::move(pAsset));

	{
		std::unique_lock lock(m_RegistryMutex);
		m_vEntries[handle.uIndex].state = AssetState::Queued;
		m_vEntries[handle.uIndex].pAsset->m_State = AssetState::Queued;
	}

	m_Loader.Submit(handle, pRaw, normalized, priority);
	return handle;
}

AssetHandle AssetManager::LoadSync(const std::string& sPath)
{
	std::string normalized = NormalizePath(sPath);

	// Check deduplication
	{
		std::shared_lock lock(m_RegistryMutex);
		auto it = m_PathMap.find(normalized);
		if (it != m_PathMap.end())
			return it->second;
	}

	// Find factory
	std::string ext = GetExtension(normalized);
	auto factoryIt = m_Factories.find(ext);
	if (factoryIt == m_Factories.end())
	{
		EVO_LOG_ERROR("AssetManager::LoadSync — no factory for extension '{}'", ext);
		return {};
	}

	auto pAsset = factoryIt->second();
	AssetHandle handle = AllocateEntry(normalized, std::move(pAsset));

	auto& entry = m_vEntries[handle.uIndex];
	entry.state = AssetState::Loading;
	entry.pAsset->m_State = AssetState::Loading;

	// Read + parse + finalize on calling thread
	auto rawData = FileSystem::ReadBinary(normalized);
	bool bSuccess = !rawData.empty() && entry.pAsset->OnLoad(rawData);
	if (bSuccess)
		bSuccess = entry.pAsset->OnFinalize(m_pDevice);

	if (bSuccess)
	{
		entry.state = AssetState::Loaded;
		entry.pAsset->m_State = AssetState::Loaded;
		entry.lastWriteTime = std::filesystem::last_write_time(normalized);
		EVO_LOG_INFO("AssetManager: loaded '{}' (sync)", normalized);
	}
	else
	{
		entry.state = AssetState::Failed;
		entry.pAsset->m_State = AssetState::Failed;
		EVO_LOG_ERROR("AssetManager: failed to load '{}' (sync)", normalized);
	}

	return handle;
}

Asset* AssetManager::GetBase(AssetHandle handle)
{
	std::shared_lock lock(m_RegistryMutex);
	if (handle.uIndex >= m_vEntries.size())
		return nullptr;
	auto& entry = m_vEntries[handle.uIndex];
	if (!entry.bAlive || entry.uGeneration != handle.uGeneration)
		return nullptr;
	return entry.pAsset.get();
}

AssetState AssetManager::GetState(AssetHandle handle) const
{
	std::shared_lock lock(m_RegistryMutex);
	if (handle.uIndex >= m_vEntries.size())
		return AssetState::Unloaded;
	auto& entry = m_vEntries[handle.uIndex];
	if (!entry.bAlive || entry.uGeneration != handle.uGeneration)
		return AssetState::Unloaded;
	return entry.state;
}

AssetHandle AssetManager::FindByPath(const std::string& sPath) const
{
	std::string normalized = NormalizePath(sPath);
	std::shared_lock lock(m_RegistryMutex);
	auto it = m_PathMap.find(normalized);
	if (it != m_PathMap.end())
		return it->second;
	return {};
}

void AssetManager::Unload(AssetHandle handle)
{
	std::unique_lock lock(m_RegistryMutex);
	if (handle.uIndex >= m_vEntries.size())
		return;
	auto& entry = m_vEntries[handle.uIndex];
	if (!entry.bAlive || entry.uGeneration != handle.uGeneration)
		return;

	if (entry.state == AssetState::Loaded)
		entry.pAsset->OnUnload(m_pDevice);

	entry.pAsset->m_State = AssetState::Unloaded;
	entry.state = AssetState::Unloaded;

	// Remove from path map and free entry
	m_PathMap.erase(entry.sPath);
	entry.bAlive = false;
	entry.pAsset.reset();
	entry.uGeneration++;
	m_vFreeList.push_back(handle.uIndex);
}

void AssetManager::Reload(AssetHandle handle)
{
	std::unique_lock lock(m_RegistryMutex);
	if (handle.uIndex >= m_vEntries.size())
		return;
	auto& entry = m_vEntries[handle.uIndex];
	if (!entry.bAlive || entry.uGeneration != handle.uGeneration)
		return;
	if (entry.state != AssetState::Loaded)
		return;

	auto rawData = FileSystem::ReadBinary(entry.sPath);
	if (rawData.empty())
	{
		EVO_LOG_WARN("AssetManager::Reload — failed to read '{}'", entry.sPath);
		return;
	}

	bool bSuccess = entry.pAsset->OnReload(rawData, m_pDevice);
	if (bSuccess)
	{
		entry.lastWriteTime = std::filesystem::last_write_time(entry.sPath);
		EVO_LOG_INFO("AssetManager: reloaded '{}'", entry.sPath);
	}
	else
	{
		EVO_LOG_WARN("AssetManager: reload failed for '{}', keeping old state", entry.sPath);
	}
}

void AssetManager::Cancel(AssetHandle handle)
{
	m_Loader.Cancel(handle);
}

void AssetManager::CheckHotReload()
{
	std::shared_lock lock(m_RegistryMutex);
	for (uint32 i = 0; i < static_cast<uint32>(m_vEntries.size()); ++i)
	{
		auto& entry = m_vEntries[i];
		if (!entry.bAlive || entry.state != AssetState::Loaded)
			continue;

		std::error_code ec;
		auto currentTime = std::filesystem::last_write_time(entry.sPath, ec);
		if (ec)
			continue;

		if (currentTime > entry.lastWriteTime)
		{
			AssetHandle handle;
			handle.uIndex = i;
			handle.uGeneration = entry.uGeneration;

			lock.unlock();
			Reload(handle);
			lock.lock();
		}
	}
}

void AssetManager::Update()
{
	auto completed = m_Loader.DrainCompleted();
	if (completed.empty())
		return;

	std::unique_lock lock(m_RegistryMutex);
	for (auto& task : completed)
	{
		if (task.handle.uIndex >= m_vEntries.size())
			continue;
		auto& entry = m_vEntries[task.handle.uIndex];
		if (!entry.bAlive || entry.uGeneration != task.handle.uGeneration)
			continue;

		if (task.bCancelled)
		{
			entry.state = AssetState::Unloaded;
			entry.pAsset->m_State = AssetState::Unloaded;
		}
		else if (task.bSuccess)
		{
			entry.state = AssetState::Loaded;
			entry.pAsset->m_State = AssetState::Loaded;

			std::error_code ec;
			entry.lastWriteTime = std::filesystem::last_write_time(entry.sPath, ec);

			EVO_LOG_INFO("AssetManager: loaded '{}' (async)", entry.sPath);
		}
		else
		{
			entry.state = AssetState::Failed;
			entry.pAsset->m_State = AssetState::Failed;
			EVO_LOG_ERROR("AssetManager: failed to load '{}' (async)", entry.sPath);
		}
	}
}

AssetHandle AssetManager::AllocateEntry(const std::string& sPath, std::unique_ptr<Asset> pAsset)
{
	std::unique_lock lock(m_RegistryMutex);

	AssetHandle handle;

	if (!m_vFreeList.empty())
	{
		handle.uIndex = m_vFreeList.back();
		m_vFreeList.pop_back();
		handle.uGeneration = m_vEntries[handle.uIndex].uGeneration;
	}
	else
	{
		handle.uIndex = static_cast<uint32>(m_vEntries.size());
		handle.uGeneration = 0;
		m_vEntries.emplace_back();
	}

	auto& entry     = m_vEntries[handle.uIndex];
	entry.bAlive      = true;
	entry.uGeneration = handle.uGeneration;
	entry.sPath       = sPath;
	entry.state       = AssetState::Unloaded;

	pAsset->m_Handle = handle;
	pAsset->m_sPath  = sPath;
	pAsset->m_State  = AssetState::Unloaded;
	entry.pAsset = std::move(pAsset);

	m_PathMap[sPath] = handle;

	return handle;
}

void AssetManager::FreeEntry(AssetHandle handle)
{
	// Caller must hold m_RegistryMutex (unique)
	auto& entry = m_vEntries[handle.uIndex];
	m_PathMap.erase(entry.sPath);
	entry.bAlive = false;
	entry.pAsset.reset();
	entry.uGeneration++;
	m_vFreeList.push_back(handle.uIndex);
}

std::string AssetManager::NormalizePath(const std::string& sPath) const
{
	std::filesystem::path p(sPath);
	std::string result = p.lexically_normal().generic_string();
	return result;
}

std::string AssetManager::GetExtension(const std::string& sPath) const
{
	std::filesystem::path p(sPath);
	std::string ext = p.extension().string();
	// Lowercase
	std::transform(ext.begin(), ext.end(), ext.begin(),
	               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return ext;
}

} // namespace Evo
