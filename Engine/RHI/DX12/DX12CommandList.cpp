#include "RHI/DX12/DX12CommandList.h"
#include "RHI/DX12/DX12Device.h"
#include "Core/Log.h"

namespace Evo {

bool DX12CommandList::Initialize(DX12Device* device, RHIQueueType type)
{
    m_Device    = device;
    m_QueueType = type;

    // TODO Phase 1:
    // 1. Determine D3D12_COMMAND_LIST_TYPE from RHIQueueType
    // 2. device->GetD3D12Device()->CreateCommandAllocator(type, ...) → m_Allocator
    // 3. device->GetD3D12Device()->CreateCommandList(0, type, m_Allocator.Get(), nullptr, ...) → m_CmdList
    // 4. m_CmdList->Close()  (starts in closed state)

    EVO_LOG_INFO("DX12CommandList::Initialize (stub)");
    return true;
}

void DX12CommandList::ShutdownCommandList()
{
    m_CmdList.Reset();
}

void DX12CommandList::Begin()
{
    // TODO: m_Allocator->Reset(); m_CmdList->Reset(m_Allocator.Get(), nullptr);
}

void DX12CommandList::End()
{
    // TODO: m_CmdList->Close();
}

// ---- Barriers ----

void DX12CommandList::TextureBarrier(const RHITextureBarrier* /*barriers*/, u32 /*count*/)
{
    // TODO: translate to D3D12_RESOURCE_BARRIER and call ResourceBarrier()
}

void DX12CommandList::BufferBarrier(const RHIBufferBarrier* /*barriers*/, u32 /*count*/)
{
    // TODO: same as texture barrier but for buffer resources
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

void DX12CommandList::SetPushConstants(const void* /*data*/, u32 /*size*/)
{
    // TODO Phase 3: SetGraphicsRoot32BitConstants
}

void DX12CommandList::SetDescriptorSet(u32 /*index*/, RHIDescriptorSetHandle /*set*/)
{
    // TODO Phase 4: SetGraphicsRootDescriptorTable
}

// ---- Vertex / Index ----

void DX12CommandList::SetVertexBuffer(u32 /*slot*/, RHIBufferHandle /*buffer*/, u64 /*offset*/)
{
    // TODO Phase 3: IASetVertexBuffers
}

void DX12CommandList::SetIndexBuffer(RHIBufferHandle /*buffer*/, u64 /*offset*/, RHIIndexFormat /*format*/)
{
    // TODO Phase 3: IASetIndexBuffer
}

// ---- Draw ----

void DX12CommandList::Draw(u32 /*vertexCount*/, u32 /*instanceCount*/,
    u32 /*firstVertex*/, u32 /*firstInstance*/)
{
    // TODO Phase 3: m_CmdList->DrawInstanced(...)
}

void DX12CommandList::DrawIndexed(u32 /*indexCount*/, u32 /*instanceCount*/,
    u32 /*firstIndex*/, i32 /*vertexOffset*/, u32 /*firstInstance*/)
{
    // TODO Phase 3: m_CmdList->DrawIndexedInstanced(...)
}

// ---- Compute ----

void DX12CommandList::Dispatch(u32 /*groupCountX*/, u32 /*groupCountY*/, u32 /*groupCountZ*/)
{
    // TODO: m_CmdList->Dispatch(...)
}

// ---- Copy ----

void DX12CommandList::CopyBuffer(RHIBufferHandle /*src*/, u64 /*srcOffset*/,
    RHIBufferHandle /*dst*/, u64 /*dstOffset*/, u64 /*size*/)
{
    // TODO: m_CmdList->CopyBufferRegion(...)
}

void DX12CommandList::CopyBufferToTexture(RHIBufferHandle /*src*/, RHITextureHandle /*dst*/)
{
    // TODO Phase 5: CopyTextureRegion
}

} // namespace Evo
