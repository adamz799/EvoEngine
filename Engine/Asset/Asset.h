#pragma once

#include "Asset/AssetTypes.h"
#include <vector>
#include <string>

namespace Evo {

class RHIDevice;

/// Base class for all loadable assets.
///
/// Lifecycle hooks (all called on worker threads unless noted):
///   OnLoad      — parse raw file data (CPU work)
///   OnFinalize  — create GPU resources (DX12 resource creation is thread-safe)
///   OnUnload    — release all resources (called on main thread)
///   OnReload    — hot-reload from new data; keeps old state on failure
class Asset {
public:
	virtual ~Asset() = default;

	/// Runtime type name for debugging and type checks.
	virtual const char* GetTypeName() const = 0;

	// ====== Lifecycle Hooks ======

	/// Parse raw file bytes. Called on a worker thread.
	/// Return false on parse failure.
	virtual bool OnLoad(const std::vector<uint8>& rawData) = 0;

	/// Create GPU resources after successful OnLoad.
	/// Called on the same worker thread immediately after OnLoad.
	/// DX12 resource creation APIs and engine resource pools are thread-safe.
	/// Assets that don't need GPU resources can leave the default (return true).
	virtual bool OnFinalize(RHIDevice* pDevice) { (void)pDevice; return true; }

	/// Release all resources. Called on the main thread.
	virtual void OnUnload(RHIDevice* pDevice) = 0;

	/// Hot-reload with new file data. Return false to keep old state.
	/// Default: unload + load + finalize. Override for incremental updates.
	virtual bool OnReload(const std::vector<uint8>& rawData, RHIDevice* pDevice)
	{
		OnUnload(pDevice);
		if (!OnLoad(rawData)) return false;
		return OnFinalize(pDevice);
	}

	// ====== Queries ======
	AssetHandle        GetHandle() const { return m_Handle; }
	AssetState         GetState()  const { return m_State; }
	const std::string& GetPath()   const { return m_sPath; }

protected:
	friend class AssetManager;
	AssetHandle m_Handle;
	AssetState  m_State = AssetState::Unloaded;
	std::string m_sPath;
};

} // namespace Evo

