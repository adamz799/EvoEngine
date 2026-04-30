#pragma once

#include "Asset/AssetTypes.h"
#include "Asset/Asset.h"
#include "Asset/AssetLoader.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <functional>
#include <filesystem>

namespace Evo {

class RHIDevice;

/// Central asset registry — manages loading, caching, hot-reload, and lifetime.
///
/// Usage:
///   manager.RegisterFactory(".obj", []{ return std::make_unique<MeshAsset>(); });
///   AssetHandle h = manager.Load("Assets/Models/cube.obj");
///   // ... each frame:
///   manager.Update();  // drains async completion queue
///   if (auto* mesh = manager.Get<MeshAsset>(h)) { /* use it */ }
class AssetManager {
public:
	void Initialize(RHIDevice* pDevice);
	void Shutdown();

	// ---- Factory registration ----
	using AssetFactory = std::function<std::unique_ptr<Asset>()>;
	void RegisterFactory(const std::string& sExtension, AssetFactory factory);

	// ---- Loading ----

	/// Async load. Returns handle immediately; asset loads in background.
	AssetHandle Load(const std::string& sPath,
					 AssetPriority priority = AssetPriority::Normal);

	/// Sync load. Blocks until Loaded or Failed.
	AssetHandle LoadSync(const std::string& sPath);

	// ---- Query ----

	/// Get typed asset pointer. Returns nullptr if not loaded or type mismatch.
	template<typename T>
	T* Get(AssetHandle handle);

	/// Get base Asset pointer.
	Asset* GetBase(AssetHandle handle);

	/// Query load state.
	AssetState GetState(AssetHandle handle) const;

	/// Find existing handle by path. Returns invalid handle if not found.
	AssetHandle FindByPath(const std::string& sPath) const;

	// ---- Lifetime ----

	/// Unload asset and release GPU resources.
	void Unload(AssetHandle handle);

	/// Synchronous reload from disk. Keeps old state on failure.
	void Reload(AssetHandle handle);

	/// Cancel a queued (not yet started) async load.
	void Cancel(AssetHandle handle);

	// ---- Hot-reload ----

	/// Scan all loaded assets for file timestamp changes. Call periodically.
	void CheckHotReload();

	// ---- Per-frame update ----

	/// Drain async completion queue. Call once per frame.
	void Update();

private:
	struct AssetEntry {
		std::unique_ptr<Asset>          pAsset;
		std::string                     sPath;
		std::filesystem::file_time_type lastWriteTime = {};
		AssetState                      state       = AssetState::Unloaded;
		uint16                          uGeneration = 0;
		bool                            bAlive      = false;
	};

	RHIDevice* m_pDevice = nullptr;

	mutable std::shared_mutex                       m_RegistryMutex;
	std::vector<AssetEntry>                         m_vEntries;
	std::vector<uint32>                             m_vFreeList;
	std::unordered_map<std::string, AssetHandle>    m_PathMap;
	std::unordered_map<std::string, AssetFactory>   m_Factories;
	AssetLoader                                     m_Loader;

	AssetHandle AllocateEntry(const std::string& sPath, std::unique_ptr<Asset> pAsset);
	void        FreeEntry(AssetHandle handle);
	std::string NormalizePath(const std::string& sPath) const;
	std::string GetExtension(const std::string& sPath) const;
};

// ---- Template implementation ----

template<typename T>
T* AssetManager::Get(AssetHandle handle)
{
	Asset* pBase = GetBase(handle);
	if (!pBase || pBase->GetState() != AssetState::Loaded)
		return nullptr;
	return dynamic_cast<T*>(pBase);
}

} // namespace Evo

