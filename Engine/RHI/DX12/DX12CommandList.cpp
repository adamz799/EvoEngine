#include "RHI/DX12/DX12CommandList.h"
#include "RHI/DX12/DX12Device.h"
#include "Core/Log.h"
#include "DX12TypeMap.h"

namespace Evo {

bool DX12CommandList::Initialize(DX12Device* device, RHIQueueType type)
{
	m_Device    = device;
	m_QueueType = type;

	if (!device || !device->GetD3D12Device())
		return false;

	HRESULT hr = device->GetD3D12Device()->CreateCommandAllocator(MapQueueType(type), IID_PPV_ARGS(&m_Allocator));
	if (FAILED(hr))
	{
		EVO_LOG_ERROR("Failed to create command allocator: {}", GetHResultString(hr));
		return false;
	}

	// Create as base CommandList, then QI for CommandList7 (Enhanced Barriers)
	ComPtr<ID3D12GraphicsCommandList> baseCmdList;
	hr = device->GetD3D12Device()->CreateCommandList(0, MapQueueType(type), m_Allocator.Get(), nullptr, IID_PPV_ARGS(&baseCmdList));
	if (FAILED(hr))
	{
		EVO_LOG_ERROR("Failed to create command list: {}", GetHResultString(hr));
		return false;
	}

	hr = baseCmdList.As(&m_CmdList);
	if (FAILED(hr))
	{
		EVO_LOG_ERROR("Failed to query ID3D12GraphicsCommandList7 (Enhanced Barriers require Agility SDK): {}",
		              GetHResultString(hr));
		return false;
	}

	m_CmdList->Close();
	return true;
}

void DX12CommandList::ShutdownCommandList()
{
	m_CmdList.Reset();
	m_Allocator.Reset();
}

void DX12CommandList::Begin()
{
	m_Allocator->Reset(); 
	m_CmdList->Reset(m_Allocator.Get(), nullptr);
}

void DX12CommandList::End()
{
	m_CmdList->Close();
}

// ---- Barriers (Enhanced Barrier API via ID3D12GraphicsCommandList7) ----

void DX12CommandList::TextureBarrier(const RHITextureBarrier* barriers, uint32 count)
{
	// TODO: resolve RHITextureHandle → ID3D12Resource* via Device handle pool (Phase 3)
	// For Phase 1, use NativeTextureBarrier() directly with SwapChain resources.
	(void)barriers;
	(void)count;
}

void DX12CommandList::BufferBarrier(const RHIBufferBarrier* barriers, uint32 count)
{
	// TODO: resolve RHIBufferHandle → ID3D12Resource* via Device handle pool (Phase 3)
	(void)barriers;
	(void)count;
}

void DX12CommandList::NativeTextureBarrier(
	ID3D12Resource* resource,
	D3D12_BARRIER_SYNC syncBefore, D3D12_BARRIER_SYNC syncAfter,
	D3D12_BARRIER_ACCESS accessBefore, D3D12_BARRIER_ACCESS accessAfter,
	D3D12_BARRIER_LAYOUT layoutBefore, D3D12_BARRIER_LAYOUT layoutAfter)
{
	D3D12_TEXTURE_BARRIER texBarrier = {};
	texBarrier.SyncBefore   = syncBefore;
	texBarrier.SyncAfter    = syncAfter;
	texBarrier.AccessBefore = accessBefore;
	texBarrier.AccessAfter  = accessAfter;
	texBarrier.LayoutBefore = layoutBefore;
	texBarrier.LayoutAfter  = layoutAfter;
	texBarrier.pResource    = resource;
	texBarrier.Subresources.IndexOrFirstMipLevel = 0xFFFFFFFF; // all subresources
	texBarrier.Flags        = D3D12_TEXTURE_BARRIER_FLAG_NONE;

	D3D12_BARRIER_GROUP group = {};
	group.Type                = D3D12_BARRIER_TYPE_TEXTURE;
	group.NumBarriers         = 1;
	group.pTextureBarriers    = &texBarrier;

	m_CmdList->Barrier(1, &group);
}

// ---- Render pass ----

void DX12CommandList::BeginRenderPass(const RHIRenderPassDesc& /*desc*/)
{
	// TODO Phase 1:
	// 1. Resolve RHITextureHandles to D3D12_CPU_DESCRIPTOR_HANDLE RTVs
	// 2. OMSetRenderTargets
	// 3. If loadAction == Clear: ClearRenderTargetView / ClearDepthStencilView
}

void DX12CommandList::EndRenderPass()
{
	// No-op for DX12 (unless using ID3D12GraphicsCommandList4::BeginRenderPass)
}

// ---- Pipeline ----

void DX12CommandList::SetPipeline(RHIPipelineHandle /*pipeline*/)
{
	// TODO Phase 3: resolve handle, SetPipelineState + SetGraphicsRootSignature
}

void DX12CommandList::SetViewport(const RHIViewport& viewport)
{
	// TODO:
	// D3D12_VIEWPORT vp = { viewport.x, viewport.y, viewport.width, viewport.height,
	//                       viewport.minDepth, viewport.maxDepth };
	// m_CmdList->RSSetViewports(1, &vp);
	(void)viewport;
}

void DX12CommandList::SetScissorRect(const RHIScissorRect& rect)
{
	// TODO:
	// D3D12_RECT r = { rect.left, rect.top, rect.right, rect.bottom };
	// m_CmdList->RSSetScissorRects(1, &r);
	(void)rect;
}

// ---- Binding ----

void DX12CommandList::SetPushConstants(const void* /*data*/, uint32 /*size*/)
{
	// TODO Phase 3: SetGraphicsRoot32BitConstants
}

void DX12CommandList::SetDescriptorSet(uint32 /*index*/, RHIDescriptorSetHandle /*set*/)
{
	// TODO Phase 4: SetGraphicsRootDescriptorTable
}

// ---- Vertex / Index ----

void DX12CommandList::SetVertexBuffer(uint32 /*slot*/, RHIBufferHandle /*buffer*/, uint64 /*offset*/)
{
	// TODO Phase 3: IASetVertexBuffers
}

void DX12CommandList::SetIndexBuffer(RHIBufferHandle /*buffer*/, uint64 /*offset*/, RHIIndexFormat /*format*/)
{
	// TODO Phase 3: IASetIndexBuffer
}

// ---- Draw ----

void DX12CommandList::Draw(uint32 /*vertexCount*/, uint32 /*instanceCount*/,
	uint32 /*firstVertex*/, uint32 /*firstInstance*/)
{
	// TODO Phase 3: m_CmdList->DrawInstanced(...)
}

void DX12CommandList::DrawIndexed(uint32 /*indexCount*/, uint32 /*instanceCount*/,
	uint32 /*firstIndex*/, int32 /*vertexOffset*/, uint32 /*firstInstance*/)
{
	// TODO Phase 3: m_CmdList->DrawIndexedInstanced(...)
}

// ---- Compute ----

void DX12CommandList::Dispatch(uint32 /*groupCountX*/, uint32 /*groupCountY*/, uint32 /*groupCountZ*/)
{
	// TODO: m_CmdList->Dispatch(...)
}

// ---- Copy ----

void DX12CommandList::CopyBuffer(RHIBufferHandle /*src*/, uint64 /*srcOffset*/,
	RHIBufferHandle /*dst*/, uint64 /*dstOffset*/, uint64 /*size*/)
{
	// TODO: m_CmdList->CopyBufferRegion(...)
}

void DX12CommandList::CopyBufferToTexture(RHIBufferHandle /*src*/, RHITextureHandle /*dst*/)
{
	// TODO Phase 5: CopyTextureRegion
}

} // namespace Evo
