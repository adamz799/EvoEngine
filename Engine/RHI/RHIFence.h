#pragma once

#include "RHI/RHITypes.h"

namespace Evo {

/// Timeline fence — GPU/CPU synchronization primitive.
///
/// DX12:   ID3D12Fence (natively a timeline fence)
/// Vulkan: VkSemaphore (VK_SEMAPHORE_TYPE_TIMELINE)
class RHIFence {
public:
    virtual ~RHIFence() = default;

    /// Query the latest value the GPU has completed.
    virtual u64 GetCompletedValue() = 0;

    /// Block the CPU until the GPU reaches the specified value.
    virtual void CpuWait(u64 value) = 0;

    /// Signal from the CPU side (rarely needed; GPU signals via Queue::Submit).
    virtual void CpuSignal(u64 value) = 0;
};

} // namespace Evo
