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

    virtual void TextureBarrier(const RHITextureBarrier* barriers, u32 count) = 0;
    virtual void BufferBarrier(const RHIBufferBarrier* barriers, u32 count) = 0;

    // Convenience single-resource overloads (inline, non-virtual):
    void TextureBarrier(RHITextureHandle tex, RHIResourceState before, RHIResourceState after) {
        RHITextureBarrier b{ tex, before, after };
        TextureBarrier(&b, 1);
    }
    void BufferBarrier(RHIBufferHandle buf, RHIResourceState before, RHIResourceState after) {
        RHIBufferBarrier b{ buf, before, after };
        BufferBarrier(&b, 1);
    }

    // ---- Render pass ----

    virtual void BeginRenderPass(const RHIRenderPassDesc& desc) = 0;
    virtual void EndRenderPass() = 0;

    // ---- Pipeline state ----

    virtual void SetPipeline(RHIPipelineHandle pipeline) = 0;
    virtual void SetViewport(const RHIViewport& viewport) = 0;
    virtual void SetScissorRect(const RHIScissorRect& rect) = 0;

    // ---- Resource binding ----

    virtual void SetPushConstants(const void* data, u32 size) = 0;
    virtual void SetDescriptorSet(u32 index, RHIDescriptorSetHandle set) = 0;

    // ---- Vertex / Index buffers ----

    virtual void SetVertexBuffer(u32 slot, RHIBufferHandle buffer, u64 offset = 0) = 0;
    virtual void SetIndexBuffer(RHIBufferHandle buffer, u64 offset, RHIIndexFormat format) = 0;

    // ---- Draw ----

    virtual void Draw(u32 vertexCount, u32 instanceCount = 1,
                      u32 firstVertex = 0, u32 firstInstance = 0) = 0;
    virtual void DrawIndexed(u32 indexCount, u32 instanceCount = 1,
                             u32 firstIndex = 0, i32 vertexOffset = 0,
                             u32 firstInstance = 0) = 0;

    // ---- Compute ----

    virtual void Dispatch(u32 groupCountX, u32 groupCountY, u32 groupCountZ) = 0;

    // ---- Copy ----

    virtual void CopyBuffer(RHIBufferHandle src, u64 srcOffset,
                            RHIBufferHandle dst, u64 dstOffset, u64 size) = 0;
    virtual void CopyBufferToTexture(RHIBufferHandle src, RHITextureHandle dst) = 0;

    // ---- Query ----

    virtual RHIQueueType GetQueueType() const = 0;
};

} // namespace Evo
