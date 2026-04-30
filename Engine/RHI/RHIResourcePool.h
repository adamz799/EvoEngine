#pragma once

#include "Core/Types.h"
#include "RHI/RHITypes.h"

namespace Evo {

// ============================================================================
// CPU-side barrier state tracking
// ============================================================================

/// Tracks the most recent barrier state of a resource on the CPU side.
/// Used for validation (detecting layoutBefore mismatches) and potential
/// auto-fill of barrier "before" fields in future phases.
struct RHIBarrierState {
	RHIBarrierSync   currentSync   = RHIBarrierSync::None;
	RHIBarrierAccess currentAccess = RHIBarrierAccess::Common;
	RHITextureLayout currentLayout = RHITextureLayout::Undefined;
};

} // namespace Evo

