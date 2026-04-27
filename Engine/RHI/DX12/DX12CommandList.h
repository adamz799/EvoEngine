#pragma once

#include "RHI/RHICommandList.h"
#include "RHI/DX12/DX12Common.h"

namespace Evo {

class DX12Device;

class DX12CommandList : public RHICommandList {
public:
    DX12CommandList() = default;
    ~DX12CommandList() override = default;

    bool Initialize(DX12Device* device, RHIQueueType type);
    void ShutdownCommandList();

    // ---- Lifecycle ----
    void Begin() override;
    void End() override;

    // ---- Barriers ----
    void TextureBarrier(const RHITextureBarrier* barriers, u32 count) override;
    void BufferBarrier(const RHIBufferBarrier* barriers, u32 count) override;

    // ---- Render pass ----
    void BeginRenderPass(const RHIRenderPassDesc& desc) override;
    void EndRenderPass() override;

    // ---- Pipeline ----
    void SetPipeline(RHIPipelineHandle pipeline) override;
    void SetViewport(const RHIViewport& viewport) override;
    void SetScissorRect(const RHIScissorRect& rect) override;

    // ---- Binding ----
    void SetPushConstants(const void* data, u32 size) override;
    void SetDescriptorSet(u32 index, RHIDescriptorSetHandle set) override;

    // ---- Vertex / Index ----
    void SetVertexBuffer(u32 slot, RHIBufferHandle buffer, u64 offset) override;
    void SetIndexBuffer(RHIBufferHandle buffer, u64 offset, RHIIndexFormat format) override;

    // ---- Draw ----
    void Draw(u32 vertexCount, u32 instanceCount,
              u32 firstVertex, u32 firstInstance) override;
    void DrawIndexed(u32 indexCount, u32 instanceCount,
                     u32 firstIndex, i32 vertexOffset, u32 firstInstance) override;

    // ---- Compute ----
    void Dispatch(u32 groupCountX, u32 groupCountY, u32 groupCountZ) override;

    // ---- Copy ----
    void CopyBuffer(RHIBufferHandle src, u64 srcOffset,
                    RHIBufferHandle dst, u64 dstOffset, u64 size) override;
    void CopyBufferToTexture(RHIBufferHandle src, RHITextureHandle dst) override;

    // ---- Query ----
    RHIQueueType GetQueueType() const override { return m_QueueType; }

    // ---- Native accessor ----
    ID3D12GraphicsCommandList* GetD3D12CommandList() const { return m_CmdList.Get(); }

private:
    DX12Device* m_Device = nullptr;   // non-owning
    RHIQueueType m_QueueType = RHIQueueType::Graphics;

    // TODO: Fill in during Phase 1 implementation
    // ComPtr<ID3D12CommandAllocator>      m_Allocator;
    // ComPtr<ID3D12GraphicsCommandList>   m_CmdList;
    ComPtr<ID3D12GraphicsCommandList> m_CmdList;
};

} // namespace Evo
