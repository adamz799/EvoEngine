#include "RHI/DX12/DX12Device.h"
#include "RHI/DX12/DX12Queue.h"
#include "RHI/DX12/DX12Fence.h"
#include "RHI/DX12/DX12SwapChain.h"
#include "RHI/DX12/DX12CommandList.h"
#include "RHI/DX12/DX12TypeMap.h"
#include "Core/Log.h"

#include <locale>
#include <codecvt>

namespace Evo {

DX12Device::~DX12Device()
{
    Shutdown();
}

bool DX12Device::Initialize(const RHIDeviceDesc& desc)
{
    EVO_LOG_INFO("DX12Device::Initialize");

    // 1. Enable debug layer
    if (desc.enableDebug)
    {
        ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
        {
            debug->EnableDebugLayer();
			{
				ComPtr<ID3D12Debug1> debug1;
				if (SUCCEEDED(debug.As(&debug1)))
				{
					debug1->SetEnableGPUBasedValidation(TRUE);
				}
			}
        }
    }

    // 2. Create DXGI factory
    UINT factoryFlags = 0;
    if (desc.enableDebug)
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

    HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_DxgiFactory));
    if (FAILED(hr))
    {
        EVO_LOG_ERROR("Failed to create DXGI factory: {}", GetHResultString(hr));
        return false;
    }

    // 3. Enumerate adapter (prefer high-performance GPU)
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIFactory6> dxgiFactory6;
    if (SUCCEEDED(m_DxgiFactory.As(&dxgiFactory6)))
    {
        dxgiFactory6->EnumAdapterByGpuPreference(
            0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
    }
    else
    {
        m_DxgiFactory->EnumAdapters1(0, &adapter);
    }

    DXGI_ADAPTER_DESC1 adapterDesc;
    adapter->GetDesc1(&adapterDesc);

    // Convert wide-char adapter description to UTF-8
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, adapterDesc.Description, -1,
                                      nullptr, 0, nullptr, nullptr);
        if (len > 0) {
            m_AdapterName.resize(static_cast<size_t>(len - 1));
            WideCharToMultiByte(CP_UTF8, 0, adapterDesc.Description, -1,
                                m_AdapterName.data(), len, nullptr, nullptr);
        }
    }
    EVO_LOG_INFO("GPU: {}", m_AdapterName);

    // 4. Create D3D12 device
    hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_pDevice));
    if (FAILED(hr))
    {
        EVO_LOG_ERROR("Failed to create D3D12 device: {}", GetHResultString(hr));
        return false;
    }

    // 5. Create graphics queue
    m_GraphicsQueue = std::make_unique<DX12Queue>();
    if (!m_GraphicsQueue->Initialize(m_pDevice.Get(), RHIQueueType::Graphics))
    {
        EVO_LOG_ERROR("Failed to create graphics queue");
        return false;
    }

    // 6. Create CPU-visible descriptor allocators
    if (!m_RTVAllocator.Initialize(m_pDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256))
    {
        EVO_LOG_ERROR("Failed to create RTV descriptor allocator");
        return false;
    }

    return true;
}

void DX12Device::Shutdown()
{
    if (!m_pDevice)
        return;

    EVO_LOG_INFO("DX12Device::Shutdown");

    WaitIdle();

    m_RTVAllocator.Shutdown();
    m_CopyQueue.reset();
    m_ComputeQueue.reset();
    m_GraphicsQueue.reset();
    m_pDevice.Reset();
    m_DxgiFactory.Reset();
}

// ---- Queue access ----

RHIQueue* DX12Device::GetGraphicsQueue() { return m_GraphicsQueue.get(); }
RHIQueue* DX12Device::GetComputeQueue()  { return m_ComputeQueue.get(); }
RHIQueue* DX12Device::GetCopyQueue()     { return m_CopyQueue.get(); }

ID3D12CommandQueue* DX12Device::GetGraphicsCommandQueue() const
{
    return m_GraphicsQueue ? m_GraphicsQueue->GetD3D12Queue() : nullptr;
}

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
    //Todo 这里还是应该使用一个支持多线程的Manager单例来管理
    auto cl = std::make_unique<DX12CommandList>();
    if (!cl->Initialize(this, type))
        return nullptr;
    return cl;
}

std::unique_ptr<RHIFence> DX12Device::CreateFence(uint64 initialValue)
{
    auto fence = std::make_unique<DX12Fence>();
    if (!fence->Initialize(m_pDevice.Get(), initialValue))
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

// ---- Views ----

RHIRenderTargetView DX12Device::CreateRenderTargetView(RHITextureHandle texture)
{
    auto* entry = m_TexturePool.GetEntry(texture);
    if (!entry || !entry->resource)
    {
        EVO_LOG_WARN("CreateRenderTargetView: invalid texture handle");
        return {};
    }

    D3D12_CPU_DESCRIPTOR_HANDLE slot = m_RTVAllocator.Allocate();
    if (slot.ptr == 0)
        return {};

    m_pDevice->CreateRenderTargetView(entry->resource.Get(), nullptr, slot);
    return WrapRTV(slot);
}

void DX12Device::DestroyRenderTargetView(RHIRenderTargetView rtv)
{
    if (!rtv.IsValid())
        return;
    m_RTVAllocator.Free(UnwrapRTV(rtv));
}

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
    const RHIDescriptorWrite* /*writes*/, uint32 /*writeCount*/) {}

// ---- Texture pool access ----

const DX12TextureEntry* DX12Device::ResolveTexture(RHITextureHandle handle) const
{
    return m_TexturePool.GetEntry(handle);
}

DX12TextureEntry* DX12Device::ResolveTextureMutable(RHITextureHandle handle)
{
    return m_TexturePool.GetEntry(handle);
}

RHITextureHandle DX12Device::RegisterTextureExternal(
    ComPtr<ID3D12Resource> resource,
    const std::string& debugName,
    const RHIBarrierState& initialBarrier)
{
    return m_TexturePool.Allocate(std::move(resource), debugName, initialBarrier);
}

void DX12Device::UnregisterTextureExternal(RHITextureHandle handle)
{
    m_TexturePool.Free(handle);
}

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
    if (m_GraphicsQueue) m_GraphicsQueue->WaitIdle();
    if (m_ComputeQueue)  m_ComputeQueue->WaitIdle();
    if (m_CopyQueue)     m_CopyQueue->WaitIdle();
}

} // namespace Evo
