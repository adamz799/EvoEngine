#pragma once

#include "RHI/RHITypes.h"

namespace Evo {

// Forward declarations
class RHICommandList;
class RHIFence;

/// Command queue — submits recorded command lists to the GPU.
///
/// DX12:   ID3D12CommandQueue
/// Vulkan: VkQueue
class RHIQueue {
public:
    virtual ~RHIQueue() = default;

    /// Submit command lists for GPU execution.
    ///
    /// @param cmdLists      Array of command lists (must be in End() state).
    /// @param cmdListCount  Number of command lists.
    /// @param waitFence     Optional fence to wait on before execution.
    /// @param waitValue     Value the waitFence must reach before execution starts.
    /// @param signalFence   Optional fence to signal after execution completes.
    /// @param signalValue   Value to signal on signalFence.
    virtual void Submit(
        RHICommandList* const* cmdLists, u32 cmdListCount,
        RHIFence* waitFence   = nullptr, u64 waitValue   = 0,
        RHIFence* signalFence = nullptr, u64 signalValue = 0
    ) = 0;

    /// Block until all previously submitted work on this queue completes.
    virtual void WaitIdle() = 0;

    virtual RHIQueueType GetType() const = 0;
};

} // namespace Evo
