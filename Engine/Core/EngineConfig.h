#pragma once

#include "Core/Types.h"
#include "RHI/RHITypes.h"
#include <string>
#include <optional>
#include <filesystem>

namespace Evo {

// ============================================================================
// EngineConfig — global singleton, loaded before Engine::Launch().
// Access from anywhere via EngineConfig::GetInstance().
//
// Usage:
//   EngineConfig::LoadFromFile("Assets/engine_config.json");
//   auto& cfg = EngineConfig::GetInstance();
//   bool debug = cfg.rhi.bEnableDebugLayer;
// ============================================================================

struct RHIEngineConfig {
	std::string backend               = "dx12";   // "dx12" or "vulkan"
	bool        bEnableDebugLayer     = true;
	bool        bEnableGPUBasedValidation = false;
};

struct WindowEngineConfig {
	std::string sTitle  = "EvoEngine";
	uint32      uWidth  = 1280;
	uint32      uHeight = 720;
	bool        bVsync  = true;
};

class EngineConfig {
public:
	// ---- Singleton ----
	static EngineConfig& GetInstance();
	static bool          IsLoaded();

	// ---- Loading ----
	/// Load from JSON file. Returns false on parse error, true on success.
	static bool LoadFromFile(const std::filesystem::path& path);

	/// Use hardcoded defaults (no file needed).
	static void SetDefaults();

	// ---- Config data ----
	RHIEngineConfig    rhi;
	WindowEngineConfig window;

	/// Resolve rhi.backend string to enum, logging availability warnings.
	RHIBackendType ResolveBackend() const;

private:
	EngineConfig() = default;

	static EngineConfig s_Instance;
	static bool         s_bLoaded;

	static std::optional<EngineConfig> Load(const std::filesystem::path& path);
	static std::optional<EngineConfig> Parse(const std::string& json);
	static EngineConfig Default();
};

} // namespace Evo
