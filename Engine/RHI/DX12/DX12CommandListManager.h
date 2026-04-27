#pragma once

#include "RHI/DX12/DX12Common.h"
#include "RHI/RHITypes.h"

#include <vector>
#include <queue>

namespace Evo {

/// Manages DX12 command allocators and command lists.
///
/// Key responsibilities:
/// - Maintains a pool of command allocators per frame-in-flight per queue type
/// - Recycles command lists after GPU execution completes
/// - Handles Begin/End frame lifecycle for allocator rotation
///
/// Usage pattern:
///   BeginFrame(completedFenceValue)   -- reset allocators for this frame
///   auto* cl = Allocate(queueType)    -- get a ready-to-record command list
///   ... record commands via cl ...
///   cl->End()
///   queue->Submit(...)
///   EndFrame(frameFenceValue)         -- tag this frame's allocators with fence value
///
/// The manager tracks which allocators belong to which fence value.
/// On BeginFrame, it resets all allocators whose fence value has been reached.
class DX12CommandListManager {
public:
    static constexpr uint32 MAX_FRAMES_IN_FLIGHT = 3;

    bool Initialize(ID3D12Device* device, uint32 framesInFlight = 2);
    void Shutdown();

    /// Call at the start of each frame.
    /// Resets allocators whose work the GPU has completed (fenceValue <= completedFenceValue).
    void BeginFrame(uint64 completedFenceValue);

    /// Call at the end of each frame.
    /// Tags the current frame's used allocators with the signal fence value.
    void EndFrame(uint64 frameFenceValue);

    /// Allocate a command list ready for recording (already in open state).
    /// Returns nullptr on failure.
    ID3D12GraphicsCommandList* Allocate(D3D12_COMMAND_LIST_TYPE type);

    /// Return a command allocator to a specific type pool after the command list
    /// using it has been submitted. The allocator will be reusable once
    /// completedFenceValue >= the frame's fence value.
    /// (Internally managed — callers typically don't need this.)

    /// Get the allocator that was used for the most recently Allocate()'d command list.
    /// Needed if you want to store the allocator reference externally.
    ID3D12CommandAllocator* GetCurrentAllocator() const { return m_LastAllocator; }

private:
    struct AllocatorEntry {
        ComPtr<ID3D12CommandAllocator> allocator;
        uint64                           fenceValue = 0;  // GPU work tag
    };

    struct PerTypePool {
        // Available (reset-able) allocators — ready to be used this frame
        std::vector<AllocatorEntry>  available;

        // In-flight allocators — waiting for GPU completion
        std::queue<AllocatorEntry>   inFlight;

        // Recycled command lists (closed, can be Reset with a new allocator)
        std::queue<ComPtr<ID3D12GraphicsCommandList>> freeLists;
    };

    ID3D12Device* m_Device = nullptr;

    // One pool per command list type (DIRECT=0, BUNDLE=1, COMPUTE=2, COPY=3)
    PerTypePool m_Pools[4];

    // Track allocators used this frame (to tag with fence value in EndFrame)
    std::vector<std::pair<D3D12_COMMAND_LIST_TYPE, AllocatorEntry*>> m_FrameAllocators;

    ID3D12CommandAllocator* m_LastAllocator = nullptr;

    // Create a new allocator of the given type
    ComPtr<ID3D12CommandAllocator> CreateAllocator(D3D12_COMMAND_LIST_TYPE type);

    // Get or create an available allocator for the given type
    AllocatorEntry* AcquireAllocator(D3D12_COMMAND_LIST_TYPE type);

    static uint32 TypeIndex(D3D12_COMMAND_LIST_TYPE type);
};

} // namespace Evo
