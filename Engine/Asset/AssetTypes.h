#pragma once

#include "Core/Types.h"
#include <cstdint>

namespace Evo {

/// Lightweight asset handle — index + generation for use-after-free detection.
struct AssetHandle {
	uint32 uIndex      = UINT32_MAX;
	uint16 uGeneration = 0;

	bool IsValid() const { return uIndex != UINT32_MAX; }

	bool operator==(const AssetHandle&) const = default;
	bool operator!=(const AssetHandle&) const = default;
};

/// Current loading state of an asset.
enum class AssetState : uint8 {
	Unloaded,
	Queued,
	Loading,
	Loaded,
	Failed
};

/// Loading priority — lower numeric value = higher priority.
enum class AssetPriority : uint8 {
	Critical   = 0,
	High       = 1,
	Normal     = 2,
	Low        = 3,
	Background = 4
};

} // namespace Evo
