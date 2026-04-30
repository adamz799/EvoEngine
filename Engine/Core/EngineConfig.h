#pragma once

#include "Core/Types.h"
#include <string>
#include <optional>
#include <filesystem>

namespace Evo {

// ============================================================================
// EngineConfig — runtime configuration loaded from JSON at startup.
// Read BEFORE RHI / window initialization so backend and debug options
// are available when creating the device.
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

struct EngineConfig {
	RHIEngineConfig    rhi;
	WindowEngineConfig window;

	/// Load config from a JSON file. Returns nullopt on failure.
	static std::optional<EngineConfig> Load(const std::filesystem::path& path);

	/// Parse config from a JSON string. Returns nullopt on failure.
	static std::optional<EngineConfig> Parse(const std::string& json);

	/// Return a default config (hardcoded fallback).
	static EngineConfig Default();
};

} // namespace Evo
