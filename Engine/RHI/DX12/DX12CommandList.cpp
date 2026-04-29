#include "RHI/DX12/DX12CommandList.h"
#include "RHI/DX12/DX12Device.h"
#include "Core/Log.h"
#include "DX12TypeMap.h"

namespace Evo {

bool DX12CommandList::Initialize(DX12Device* device, RHIQueueType type)
{
	m_pDevice    = device;
	m_QueueType = type;

	if (!device || !device->GetD3D12Device())
		return false;

	HRESULT hr = device->GetD3D12Device()->CreateCommandAllocator(MapQueueType(type), IID_PPV_ARGS(&m_pAllocator));
	if (FAILED(hr))
	{
		EVO_LOG_ERROR("Failed to create command allocator: {}", GetHResultString(hr));
		return false;
	}

	// Create as base CommandList, then QI for CommandList7 (Enhanced Barriers)
	ComPtr<ID3D12GraphicsCommandList> baseCmdList;
	hr = device->GetD3D12Device()->CreateCommandList(0, MapQueueType(type), m_pAllocator.Get(), nullptr, IID_PPV_ARGS(&baseCmdList));
	if (FAILED(hr))
	{
		EVO_LOG_ERROR("Failed to create command list: {}", GetHResultString(hr));
		return false;
	}

	hr = baseCmdList.As(&m_pCmdList);
	if (FAILED(hr))
	{
		EVO_LOG_ERROR("Failed to query ID3D12GraphicsCommandList7 (Enhanced Barriers require Agility SDK): {}",
		              GetHResultString(hr));
		return false;
	}

	m_pCmdList->Close();
	return true;
}

void DX12CommandList::ShutdownCommandList()
{
	m_pCmdList.Reset();
	m_pAllocator.Reset();
}

void DX12CommandList::Begin()
{
	m_pAllocator->Reset(); 
	m_pCmdList->Reset(m_pAllocator.Get(), nullptr);
}

void DX12CommandList::End()
{
	m_pCmdList->Close();
}

// ---- Barriers (Enhanced Barrier API via ID3D12GraphicsCommandList7) ----

void DX12CommandList::TextureBarrier(const RHITextureBarrier* barriers, uint32 count)
{
	for (uint32 i = 0; i < count; ++i)
	{
		const auto& b = barriers[i];

		auto* entry = m_pDevice->ResolveTexture(b.texture);
		if (!entry)
		{
			EVO_LOG_WARN("TextureBarrier: invalid texture handle (idx={}, gen={})",
			             b.texture.uHandle, b.texture.uGeneration);
			continue;
		}

		ID3D12Resource* resource = entry->pResource.Get();
		if (!resource)
		{
			EVO_LOG_WARN("TextureBarrier: texture '{}' has null ID3D12Resource",
			             entry->sDebugName);
			continue;
		}

		NativeTextureBarrier(
			resource,
			MapBarrierSync(b.syncBefore),    MapBarrierSync(b.syncAfter),
			MapBarrierAccess(b.accessBefore), MapBarrierAccess(b.accessAfter),
			MapTextureLayout(b.layoutBefore), MapTextureLayout(b.layoutAfter));

		// Update CPU-side barrier state tracking
		if (auto* mut = m_pDevice->ResolveTextureMutable(b.texture))
		{
			mut->barrierState.currentSync   = b.syncAfter;
			mut->barrierState.currentAccess  = b.accessAfter;
			mut->barrierState.currentLayout  = b.layoutAfter;
		}
	}
}

void DX12CommandList::BufferBarrier(const RHIBufferBarrier* barriers, uint32 count)
{
	// TODO: resolve RHIBufferHandle → ID3D12Resource* via Device handle pool (Phase 3)
	(void)barriers;
	(void)count;
}

// ---- Clear ----

void DX12CommandList::ClearRenderTarget(RHIRenderTargetView rtv, const RHIColor& color)
{
	if (!rtv.IsValid())
	{
		EVO_LOG_WARN("ClearRenderTarget: invalid RTV");
		return;
	}

	const float clearColor[4] = { color.fR, color.fG, color.fB, color.fA };
	m_pCmdList->ClearRenderTargetView(UnwrapRTV(rtv), clearColor, 0, nullptr);
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

	m_pCmdList->Barrier(1, &group);
}

// ---- Render pass ----

void DX12CommandList::SetRenderTargets(const RHIRenderTargetView* rtvs, uint32 count,
                                       const RHIDepthStencilView* dsv)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[8] = {};
	for (uint32 i = 0; i < count && i < 8; ++i)
		rtvHandles[i] = UnwrapRTV(rtvs[i]);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
	bool hasDsv = dsv && dsv->IsValid();
	if (hasDsv)
		dsvHandle = UnwrapDSV(*dsv);

	m_pCmdList->OMSetRenderTargets(count, rtvHandles, FALSE, hasDsv ? &dsvHandle : nullptr);
}

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

void DX12CommandList::SetPipeline(RHIPipelineHandle pipeline)
{
	auto* entry = m_pDevice->ResolvePipeline(pipeline);
	if (!entry)
	{
		EVO_LOG_WARN("SetPipeline: invalid pipeline handle");
		return;
	}
	m_pCmdList->SetPipelineState(entry->pPso.Get());
	m_pCmdList->SetGraphicsRootSignature(entry->pRootSignature.Get());
	m_pCmdList->IASetPrimitiveTopology(MapPrimitiveTopology(entry->topology));
}

void DX12CommandList::SetViewport(const RHIViewport& viewport)
{
	D3D12_VIEWPORT vp = { viewport.fX, viewport.fY, viewport.fWidth, viewport.fHeight,
						  viewport.fMinDepth, viewport.fMaxDepth };
	m_pCmdList->RSSetViewports(1, &vp);
}

void DX12CommandList::SetScissorRect(const RHIScissorRect& rect)
{
	D3D12_RECT r = { rect.left, rect.top, rect.right, rect.bottom };
	m_pCmdList->RSSetScissorRects(1, &r);
}

// ---- Binding ----

void DX12CommandList::SetPushConstants(const void* data, uint32 size)
{
	// Root parameter 0 is always the push constants slot (32-bit constants)
	uint32 num32BitValues = (size + 3) / 4;
	m_pCmdList->SetGraphicsRoot32BitConstants(0, num32BitValues, data, 0);
}

void DX12CommandList::SetDescriptorSet(uint32 /*index*/, RHIDescriptorSetHandle /*set*/)
{
	// TODO Phase 4: SetGraphicsRootDescriptorTable
}

// ---- Vertex / Index ----

void DX12CommandList::SetVertexBuffer(uint32 slot, const RHIVertexBufferView& view)
{
	auto* entry = m_pDevice->ResolveBuffer(view.buffer);
	if (!entry)
	{
		EVO_LOG_WARN("SetVertexBuffer: invalid buffer handle");
		return;
	}
	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	vbv.BufferLocation = entry->uGpuAddress + view.uOffset;
	vbv.SizeInBytes    = view.uSize ? view.uSize : static_cast<UINT>(entry->uSize - view.uOffset);
	vbv.StrideInBytes  = view.uStride;
	m_pCmdList->IASetVertexBuffers(slot, 1, &vbv);
}

void DX12CommandList::SetIndexBuffer(const RHIIndexBufferView& view)
{
	auto* entry = m_pDevice->ResolveBuffer(view.buffer);
	if (!entry)
	{
		EVO_LOG_WARN("SetIndexBuffer: invalid buffer handle");
		return;
	}
	D3D12_INDEX_BUFFER_VIEW ibv = {};
	ibv.BufferLocation = entry->uGpuAddress + view.uOffset;
	ibv.SizeInBytes    = view.uSize ? view.uSize : static_cast<UINT>(entry->uSize - view.uOffset);
	ibv.Format         = MapIndexFormat(view.format);
	m_pCmdList->IASetIndexBuffer(&ibv);
}

// ---- Draw ----

void DX12CommandList::Draw(uint32 vertexCount, uint32 instanceCount,
	uint32 firstVertex, uint32 firstInstance)
{
	m_pCmdList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
}

void DX12CommandList::DrawIndexed(uint32 indexCount, uint32 instanceCount,
	uint32 firstIndex, int32 vertexOffset, uint32 firstInstance)
{
	m_pCmdList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

// ---- Compute ----

void DX12CommandList::Dispatch(uint32 /*groupCountX*/, uint32 /*groupCountY*/, uint32 /*groupCountZ*/)
{
	// TODO: m_pCmdList->Dispatch(...)
}

// ---- Copy ----

void DX12CommandList::CopyBuffer(RHIBufferHandle /*src*/, uint64 /*srcOffset*/,
	RHIBufferHandle /*dst*/, uint64 /*dstOffset*/, uint64 /*size*/)
{
	// TODO: m_pCmdList->CopyBufferRegion(...)
}

void DX12CommandList::CopyBufferToTexture(RHIBufferHandle /*src*/, RHITextureHandle /*dst*/)
{
	// TODO Phase 5: CopyTextureRegion
}

} // namespace Evo
