#include "RHI/DX12/DX12Device.h"
#include "RHI/DX12/DX12Queue.h"
#include "RHI/DX12/DX12Fence.h"
#include "RHI/DX12/DX12SwapChain.h"
#include "RHI/DX12/DX12CommandList.h"
#include "Core/Log.h"

namespace Evo {

DX12Device::~DX12Device()
{
    Shutdown();
}

bool DX12Device::Initialize(const RHIDeviceDesc& desc)
{
    EVO_LOG_INFO("DX12Device::Initialize (stub — implement Phase 1 here)");

    // TODO Phase 1:
    // 1. Enable debug layer if desc.enableDebug
    //    ID3D12Debug* debug; D3D12GetDebugInterface(...); debug->EnableDebugLayer();
    // 2. CreateDXGIFactory2 → m_DxgiFactory
    // 3. EnumAdapterByGpuPreference → choose GPU → fill m_AdapterName
    // 4. D3D12CreateDevice → m_Device
    // 5. Create graphics queue: m_GraphicsQueue->Initialize(m_Device.Get(), RHIQueueType::Graphics)
    // 6. Optionally create compute and copy queues

    m_AdapterName = "DX12 Device (stub)";
    return true;
}

void DX12Device::Shutdown()
{
    if (!m_Device)
        return;

    EVO_LOG_INFO("DX12Device::Shutdown");

    // TODO: WaitIdle, release queues, device, factory
    m_CopyQueue.reset();
    m_ComputeQueue.reset();
    m_GraphicsQueue.reset();
    m_Device.Reset();
    m_DxgiFactory.Reset();
}

// ---- Queue access ----

RHIQueue* DX12Device::GetGraphicsQueue() { return m_GraphicsQueue.get(); }
RHIQueue* DX12Device::GetComputeQueue()  { return m_ComputeQueue.get(); }
RHIQueue* DX12Device::GetCopyQueue()     { return m_CopyQueue.get(); }

// ---- Direct object creation ----

std::unique_ptr<RHISwapChain> DX12Device::CreateSwapChain(const RHISwapChainDesc& desc)
{
    auto sc = std::make_unique<DX12SwapChain>();
    if (!sc->Initialize(this, desc))
        return nullptr;
    return sc;
}

std::unique_ptr<RHICommandList> DX12Device::CreateCommandList(RHIQueueType type)
{
    auto cl = std::make_unique<DX12CommandList>();
    if (!cl->Initialize(this, type))
        return nullptr;
    return cl;
}

std::unique_ptr<RHIFence> DX12Device::CreateFence(u64 initialValue)
{
    auto fence = std::make_unique<DX12Fence>();
    if (!fence->Initialize(m_Device.Get(), initialValue))
        return nullptr;
    return fence;
}

// ---- Handle resources (all stubs — implement in Phase 3+) ----

RHIBufferHandle DX12Device::CreateBuffer(const RHIBufferDesc& /*desc*/)
{
    EVO_LOG_WARN("DX12Device::CreateBuffer — not yet implemented");
    return {};
}

RHITextureHandle DX12Device::CreateTexture(const RHITextureDesc& /*desc*/)
{
    EVO_LOG_WARN("DX12Device::CreateTexture — not yet implemented");
    return {};
}

RHIShaderHandle DX12Device::CreateShader(const RHIShaderDesc& /*desc*/)
{
    EVO_LOG_WARN("DX12Device::CreateShader — not yet implemented");
    return {};
}

RHIPipelineHandle DX12Device::CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& /*desc*/)
{
    EVO_LOG_WARN("DX12Device::CreateGraphicsPipeline — not yet implemented");
    return {};
}

RHIPipelineHandle DX12Device::CreateComputePipeline(const RHIComputePipelineDesc& /*desc*/)
{
    EVO_LOG_WARN("DX12Device::CreateComputePipeline — not yet implemented");
    return {};
}

void DX12Device::DestroyBuffer(RHIBufferHandle /*handle*/) {}
void DX12Device::DestroyTexture(RHITextureHandle /*handle*/) {}
void DX12Device::DestroyShader(RHIShaderHandle /*handle*/) {}
void DX12Device::DestroyPipeline(RHIPipelineHandle /*handle*/) {}

void* DX12Device::MapBuffer(RHIBufferHandle /*handle*/) { return nullptr; }
void  DX12Device::UnmapBuffer(RHIBufferHandle /*handle*/) {}

// ---- Descriptors (implement in Phase 4) ----

RHIDescriptorSetLayoutHandle DX12Device::CreateDescriptorSetLayout(const RHIDescriptorSetLayoutDesc& /*desc*/)
{
    return {};
}
void DX12Device::DestroyDescriptorSetLayout(RHIDescriptorSetLayoutHandle /*handle*/) {}

RHIDescriptorSetHandle DX12Device::AllocateDescriptorSet(RHIDescriptorSetLayoutHandle /*layout*/)
{
    return {};
}
void DX12Device::FreeDescriptorSet(RHIDescriptorSetHandle /*handle*/) {}
void DX12Device::WriteDescriptorSet(RHIDescriptorSetHandle /*set*/,
    const RHIDescriptorWrite* /*writes*/, u32 /*writeCount*/) {}

// ---- Frame management ----

void DX12Device::BeginFrame()
{
    // TODO Phase 1: process deferred deletions, wait for oldest frame fence
}

void DX12Device::EndFrame()
{
    // TODO Phase 1: advance frame counter
}

void DX12Device::WaitIdle()
{
    // TODO Phase 1: signal fence on graphics queue, wait for completion
}

} // namespace Evo
