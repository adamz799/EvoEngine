#pragma once

#include "RHI/RHICommandList.h"
#include "RHI/DX12/DX12Common.h"

namespace Evo {

class DX12Device;

class DX12CommandList : public RHICommandList {
public:
	DX12CommandList() = default;
	~DX12CommandList() override = default;

	/// Initialize with self-owned allocator (standalone mode).
	bool Initialize(DX12Device* device, RHIQueueType type);

	/// Initialize with external allocator (pool mode — pool owns the allocator).
	bool InitializePooled(DX12Device* device, RHIQueueType type,
	                       ID3D12CommandAllocator* pExternalAllocator);

	void ShutdownCommandList();

	// ---- Lifecycle ----
	void Begin() override;
	void End() override;

	// ---- Barriers ----
	void TextureBarrier(const RHITextureBarrier* barriers, uint32 count) override;
	void BufferBarrier(const RHIBufferBarrier* barriers, uint32 count) override;

	// ---- Clear ----
	void ClearRenderTarget(RHIRenderTargetView rtv, const RHIColor& color) override;

	// ---- Render pass ----
	void SetRenderTargets(const RHIRenderTargetView* rtvs, uint32 count,
	                      const RHIDepthStencilView* dsv = nullptr) override;
	void BeginRenderPass(const RHIRenderPassDesc& desc) override;
	void EndRenderPass() override;

	// ---- Pipeline ----
	void SetPipeline(RHIPipelineHandle pipeline) override;
	void SetViewport(const RHIViewport& viewport) override;
	void SetScissorRect(const RHIScissorRect& rect) override;

	// ---- Binding ----
	void SetPushConstants(const void* data, uint32 size) override;
	void SetDescriptorSet(uint32 index, RHIDescriptorSetHandle set) override;
	void SetDescriptorHeaps(uint32 uCount, void* const* ppHeaps) override;

	// ---- Vertex / Index ----
	void SetVertexBuffer(uint32 slot, const RHIVertexBufferView& view) override;
	void SetIndexBuffer(const RHIIndexBufferView& view) override;

	// ---- Draw ----
	void Draw(uint32 vertexCount, uint32 instanceCount,
			  uint32 firstVertex, uint32 firstInstance) override;
	void DrawIndexed(uint32 indexCount, uint32 instanceCount,
					 uint32 firstIndex, int32 vertexOffset, uint32 firstInstance) override;

	// ---- Compute ----
	void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override;

	// ---- Copy ----
	void CopyBuffer(RHIBufferHandle src, uint64 srcOffset,
					RHIBufferHandle dst, uint64 dstOffset, uint64 size) override;
	void CopyBufferToTexture(RHIBufferHandle src, RHITextureHandle dst) override;

	// ---- Query ----
	RHIQueueType GetQueueType() const override { return m_QueueType; }

	// ---- Native accessor ----
	ID3D12GraphicsCommandList7* GetD3D12CommandList() const { return m_pCmdList.Get(); }

	// ---- Native barrier (Phase 1: for SwapChain back buffers without Handle) ----
	void NativeTextureBarrier(ID3D12Resource* resource,
	                          D3D12_BARRIER_SYNC syncBefore, D3D12_BARRIER_SYNC syncAfter,
	                          D3D12_BARRIER_ACCESS accessBefore, D3D12_BARRIER_ACCESS accessAfter,
	                          D3D12_BARRIER_LAYOUT layoutBefore, D3D12_BARRIER_LAYOUT layoutAfter);

private:
	DX12Device*                         m_pDevice = nullptr;   // non-owning
	RHIQueueType                        m_QueueType = RHIQueueType::Graphics;

	ID3D12CommandAllocator*             m_pAllocator = nullptr;       // non-owning, used by Begin()
	ComPtr<ID3D12CommandAllocator>      m_pOwnedAllocator;           // only in standalone mode
	ComPtr<ID3D12GraphicsCommandList7>  m_pCmdList;
};

} // namespace Evo
