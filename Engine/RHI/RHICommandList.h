#pragma once

#include "RHI/RHITypes.h"

namespace Evo {

/// Command list — records GPU commands for later submission to a queue.
///
/// DX12:   ID3D12GraphicsCommandList + ID3D12CommandAllocator
/// Vulkan: VkCommandBuffer + VkCommandPool
class RHICommandList {
public:
    virtual ~RHICommandList() = default;

    // ---- Lifecycle ----

    /// Reset allocator and open for recording.
    virtual void Begin() = 0;

    /// Close the command list; ready for Queue::Submit.
    virtual void End() = 0;

    // ---- Resource barriers ----

    virtual void TextureBarrier(const RHITextureBarrier* barriers, uint32 count) = 0;
    virtual void BufferBarrier(const RHIBufferBarrier* barriers, uint32 count) = 0;

    // Convenience single-resource overloads (inline, non-virtual):
    void TextureBarrier(const RHITextureBarrier& barrier) {
        TextureBarrier(&barrier, 1);
    }
    void BufferBarrier(const RHIBufferBarrier& barrier) {
        BufferBarrier(&barrier, 1);
    }

    // ---- Render pass ----

    virtual void BeginRenderPass(const RHIRenderPassDesc& desc) = 0;
    virtual void EndRenderPass() = 0;

    // ---- Pipeline state ----

    virtual void SetPipeline(RHIPipelineHandle pipeline) = 0;
    virtual void SetViewport(const RHIViewport& viewport) = 0;
    virtual void SetScissorRect(const RHIScissorRect& rect) = 0;

    // ---- Resource binding ----

    virtual void SetPushConstants(const void* data, uint32 size) = 0;
    virtual void SetDescriptorSet(uint32 index, RHIDescriptorSetHandle set) = 0;

    // ---- Vertex / Index buffers ----

    virtual void SetVertexBuffer(uint32 slot, RHIBufferHandle buffer, uint64 offset = 0) = 0;
    virtual void SetIndexBuffer(RHIBufferHandle buffer, uint64 offset, RHIIndexFormat format) = 0;

    // ---- Draw ----

    virtual void Draw(uint32 vertexCount, uint32 instanceCount = 1,
                      uint32 firstVertex = 0, uint32 firstInstance = 0) = 0;
    virtual void DrawIndexed(uint32 indexCount, uint32 instanceCount = 1,
                             uint32 firstIndex = 0, int32 vertexOffset = 0,
                             uint32 firstInstance = 0) = 0;

    // ---- Compute ----

    virtual void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) = 0;

    // ---- Copy ----

    virtual void CopyBuffer(RHIBufferHandle src, uint64 srcOffset,
                            RHIBufferHandle dst, uint64 dstOffset, uint64 size) = 0;
    virtual void CopyBufferToTexture(RHIBufferHandle src, RHITextureHandle dst) = 0;

    // ---- Query ----

    virtual RHIQueueType GetQueueType() const = 0;
};

} // namespace Evo
