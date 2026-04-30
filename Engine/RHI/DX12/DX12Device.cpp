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
    if (desc.bEnableDebugLayer)
    {
        ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
        {
            debug->EnableDebugLayer();

            if (desc.bEnableGPUBasedValidation)
            {
                ComPtr<ID3D12Debug1> debug1;
                if (SUCCEEDED(debug.As(&debug1)))
                {
                    debug1->SetEnableGPUBasedValidation(TRUE);
                    EVO_LOG_INFO("GPU-based validation enabled");
                }
            }
        }
    }

    // 2. Create DXGI factory
    UINT factoryFlags = 0;
    if (desc.bEnableDebugLayer)
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

    HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_pDxgiFactory));
    if (FAILED(hr))
    {
        EVO_LOG_ERROR("Failed to create DXGI factory: {}", GetHResultString(hr));
        return false;
    }

    // 3. Enumerate adapter (prefer high-performance GPU)
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIFactory6> dxgiFactory6;
    if (SUCCEEDED(m_pDxgiFactory.As(&dxgiFactory6)))
    {
        dxgiFactory6->EnumAdapterByGpuPreference(
            0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
    }
    else
    {
        m_pDxgiFactory->EnumAdapters1(0, &adapter);
    }

    DXGI_ADAPTER_DESC1 adapterDesc;
    adapter->GetDesc1(&adapterDesc);

    // Convert wide-char adapter description to UTF-8
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, adapterDesc.Description, -1,
                                      nullptr, 0, nullptr, nullptr);
        if (len > 0) {
            m_sAdapterName.resize(static_cast<size_t>(len - 1));
            WideCharToMultiByte(CP_UTF8, 0, adapterDesc.Description, -1,
                                m_sAdapterName.data(), len, nullptr, nullptr);
        }
    }
    EVO_LOG_INFO("GPU: {}", m_sAdapterName);

    // 4. Create D3D12 device
    hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_pDevice));
    if (FAILED(hr))
    {
        EVO_LOG_ERROR("Failed to create D3D12 device: {}", GetHResultString(hr));
        return false;
    }

    // 5. Create D3D12MA allocator
    {
        D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
        allocatorDesc.pDevice  = m_pDevice.Get();
        allocatorDesc.pAdapter = adapter.Get();
        hr = D3D12MA::CreateAllocator(&allocatorDesc, &m_pAllocator);
        if (FAILED(hr))
        {
            EVO_LOG_ERROR("Failed to create D3D12MA allocator: {}", GetHResultString(hr));
            return false;
        }
    }

    // 6. Create graphics queue
    m_pGraphicsQueue = std::make_unique<DX12Queue>();
    if (!m_pGraphicsQueue->Initialize(m_pDevice.Get(), RHIQueueType::Graphics))
    {
        EVO_LOG_ERROR("Failed to create graphics queue");
        return false;
    }

    // 7. Create CPU-visible descriptor allocators
    if (!m_RTVAllocator.Initialize(m_pDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256))
    {
        EVO_LOG_ERROR("Failed to create RTV descriptor allocator");
        return false;
    }

    if (!m_DSVAllocator.Initialize(m_pDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 64))
    {
        EVO_LOG_ERROR("Failed to create DSV descriptor allocator");
        return false;
    }

    // 8. Create GPU-visible SRV descriptor heap (for ImGui, texture bindings)
    if (!m_SRVAllocator.Initialize(m_pDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 256))
    {
        EVO_LOG_ERROR("Failed to create SRV descriptor allocator");
        return false;
    }

    // 9. Create command list pool for graphics queue
    m_GraphicsCmdListPool.Initialize(this, D3D12_COMMAND_LIST_TYPE_DIRECT);

    return true;
}

void DX12Device::Shutdown()
{
    if (!m_pDevice)
        return;

    EVO_LOG_INFO("DX12Device::Shutdown");

    WaitIdle();

    m_GraphicsCmdListPool.Shutdown();
    m_SRVAllocator.Shutdown();
    m_DSVAllocator.Shutdown();
    m_RTVAllocator.Shutdown();
    m_pCopyQueue.reset();
    m_pComputeQueue.reset();
    m_pGraphicsQueue.reset();
    if (m_pAllocator) { m_pAllocator->Release(); m_pAllocator = nullptr; }
    m_pDevice.Reset();
    m_pDxgiFactory.Reset();
}

// ---- Queue access ----

RHIQueue* DX12Device::GetGraphicsQueue() { return m_pGraphicsQueue.get(); }
RHIQueue* DX12Device::GetComputeQueue()  { return m_pComputeQueue.get(); }
RHIQueue* DX12Device::GetCopyQueue()     { return m_pCopyQueue.get(); }

ID3D12CommandQueue* DX12Device::GetGraphicsCommandQueue() const
{
    return m_pGraphicsQueue ? m_pGraphicsQueue->GetD3D12Queue() : nullptr;
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
    //Todo 这里还是应该使用一个支持多线程的Manager单例来管�?
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

// ---- Handle resources (all stubs �?implement in Phase 3+) ----

RHIBufferHandle DX12Device::CreateBuffer(const RHIBufferDesc& desc)
{
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width              = desc.uSize;
    resourceDesc.Height             = 1;
    resourceDesc.DepthOrArraySize   = 1;
    resourceDesc.MipLevels          = 1;
    resourceDesc.SampleDesc.Count   = 1;
    resourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = MapMemoryUsage(desc.memory);

    D3D12_RESOURCE_STATES initialState = (desc.memory == RHIMemoryUsage::CpuToGpu)
        ? D3D12_RESOURCE_STATE_GENERIC_READ
        : D3D12_RESOURCE_STATE_COMMON;

    D3D12MA::Allocation* allocation = nullptr;
    ComPtr<ID3D12Resource> resource;
    HRESULT hr = m_pAllocator->CreateResource(
        &allocDesc, &resourceDesc, initialState,
        nullptr, &allocation,
        IID_PPV_ARGS(&resource));
    if (FAILED(hr))
    {
        EVO_LOG_ERROR("Failed to create buffer '{}': {}", desc.sDebugName, GetHResultString(hr));
        return {};
    }

    if (!desc.sDebugName.empty())
    {
        wchar_t wname[256];
        MultiByteToWideChar(CP_UTF8, 0, desc.sDebugName.c_str(), -1, wname, 256);
        resource->SetName(wname);
    }

    void* mappedPtr = nullptr;
    if (desc.memory == RHIMemoryUsage::CpuToGpu)
    {
        resource->Map(0, nullptr, &mappedPtr);
    }

    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = resource->GetGPUVirtualAddress();
    return m_BufferPool.Allocate(std::move(resource), allocation, gpuAddress, desc.uSize, mappedPtr, desc.sDebugName);
}

RHITextureHandle DX12Device::CreateTexture(const RHITextureDesc& desc)
{
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Width            = desc.uWidth;
    resourceDesc.Height           = desc.uHeight;
    resourceDesc.DepthOrArraySize = static_cast<UINT16>(desc.uDepthOrArraySize);
    resourceDesc.MipLevels        = static_cast<UINT16>(desc.uMipLevels);
    // Use typeless format for depth textures that also need SRV access
    bool bDepthWithSRV = IsDepthFormat(desc.format)
        && (desc.usage & RHITextureUsage::ShaderResource) != RHITextureUsage(0);
    resourceDesc.Format = bDepthWithSRV ? MapDepthToTypeless(desc.format) : MapFormat(desc.format);
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    if ((desc.usage & RHITextureUsage::RenderTarget) != RHITextureUsage(0))
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if ((desc.usage & RHITextureUsage::DepthStencil) != RHITextureUsage(0))
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    if ((desc.usage & RHITextureUsage::UnorderedAccess) != RHITextureUsage(0))
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    resourceDesc.Flags = flags;

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE* pClearValue = nullptr;
    D3D12_CLEAR_VALUE clearValue = {};
    if (flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
        clearValue.Format   = resourceDesc.Format;
        clearValue.Color[0] = desc.clearColor.fR;
        clearValue.Color[1] = desc.clearColor.fG;
        clearValue.Color[2] = desc.clearColor.fB;
        clearValue.Color[3] = desc.clearColor.fA;
        pClearValue = &clearValue;
    }
    else if (flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
    {
        clearValue.Format               = MapFormat(desc.format);  // actual depth format, not typeless
        clearValue.DepthStencil.Depth   = 1.0f;
        clearValue.DepthStencil.Stencil = 0;
        pClearValue = &clearValue;
    }

    D3D12MA::Allocation* allocation = nullptr;
    ComPtr<ID3D12Resource> resource;
    HRESULT hr = m_pAllocator->CreateResource(
        &allocDesc, &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON, pClearValue,
        &allocation, IID_PPV_ARGS(&resource));
    if (FAILED(hr))
    {
        EVO_LOG_ERROR("Failed to create texture '{}': {}", desc.sDebugName, GetHResultString(hr));
        return {};
    }

    if (!desc.sDebugName.empty())
    {
        wchar_t wname[256];
        MultiByteToWideChar(CP_UTF8, 0, desc.sDebugName.c_str(), -1, wname, 256);
        resource->SetName(wname);
    }

    return m_TexturePool.Allocate(std::move(resource), desc.sDebugName, desc.format);
}

RHIShaderHandle DX12Device::CreateShader(const RHIShaderDesc& desc)
{
    if (!desc.pBytecode || desc.uBytecodeSize == 0)
    {
        EVO_LOG_ERROR("CreateShader: null or empty bytecode");
        return {};
    }
    return m_ShaderPool.Allocate(desc.pBytecode, desc.uBytecodeSize, desc.stage, desc.sDebugName);
}

RHIPipelineHandle DX12Device::CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc)
{
    // Resolve shader bytecode
    auto* vsEntry = m_ShaderPool.GetEntry(desc.vertexShader);
    auto* psEntry = m_ShaderPool.GetEntry(desc.pixelShader);  // may be null for depth-only
    if (!vsEntry)
    {
        EVO_LOG_ERROR("CreateGraphicsPipeline: invalid vertex shader handle");
        return {};
    }

    // Build root signature
    // Root params: [push constants (optional)] [descriptor table per set]
    std::vector<D3D12_ROOT_PARAMETER> rootParams;
    // Storage for descriptor ranges �?must outlive D3D12SerializeRootSignature
    std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> allRanges;

    uint32 descTableRootOffset = 0;

    if (desc.uPushConstantSize > 0)
    {
        D3D12_ROOT_PARAMETER param = {};
        param.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param.Constants.ShaderRegister  = 0;
        param.Constants.RegisterSpace   = 0;
        param.Constants.Num32BitValues  = (desc.uPushConstantSize + 3) / 4;
        param.ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
        rootParams.push_back(param);
        descTableRootOffset = 1;
    }

    // Add descriptor tables for each set layout
    for (uint32 setIdx = 0; setIdx < desc.uDescriptorSetLayoutCount; ++setIdx)
    {
        auto* layoutEntry = m_DescSetLayoutPool.GetEntry(desc.descriptorSetLayouts[setIdx]);
        if (!layoutEntry) continue;

        // Register space: push constants use space 0, sets start at space 1 (if push constants present)
        uint32 regSpace = desc.uPushConstantSize > 0 ? setIdx + 1 : setIdx;

        std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
        for (const auto& binding : layoutEntry->vBindings)
        {
            D3D12_DESCRIPTOR_RANGE range = {};
            switch (binding.type)
            {
                case RHIDescriptorType::ConstantBuffer:  range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; break;
                case RHIDescriptorType::ShaderResource:  range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; break;
                case RHIDescriptorType::UnorderedAccess: range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV; break;
                case RHIDescriptorType::Sampler:         range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER; break;
            }
            range.NumDescriptors                    = binding.uCount;
            range.BaseShaderRegister                = binding.uBinding;
            range.RegisterSpace                     = regSpace;
            range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            ranges.push_back(range);
        }

        allRanges.push_back(std::move(ranges));

        D3D12_ROOT_PARAMETER param = {};
        param.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(allRanges.back().size());
        param.DescriptorTable.pDescriptorRanges   = allRanges.back().data();
        param.ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;
        rootParams.push_back(param);
    }

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = static_cast<UINT>(rootParams.size());
    rootSigDesc.pParameters   = rootParams.empty() ? nullptr : rootParams.data();
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> sigBlob, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                             &sigBlob, &errorBlob);
    if (FAILED(hr))
    {
        EVO_LOG_ERROR("Failed to serialize root signature: {}",
                      errorBlob ? static_cast<const char*>(errorBlob->GetBufferPointer()) : "unknown");
        return {};
    }

    ComPtr<ID3D12RootSignature> rootSig;
    hr = m_pDevice->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
                                        IID_PPV_ARGS(&rootSig));
    if (FAILED(hr))
    {
        EVO_LOG_ERROR("Failed to create root signature: {}", GetHResultString(hr));
        return {};
    }

    // Input layout
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements(desc.uInputElementCount);
    for (uint32 i = 0; i < desc.uInputElementCount; ++i)
    {
        auto& src = desc.pInputElements[i];
        auto& dst = inputElements[i];
        dst.SemanticName         = src.pSemanticName;
        dst.SemanticIndex        = src.uSemanticIndex;
        dst.Format               = MapFormat(src.format);
        dst.InputSlot            = src.uBufferSlot;
        dst.AlignedByteOffset    = src.uByteOffset;
        dst.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        dst.InstanceDataStepRate = 0;
    }

    // PSO description
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSig.Get();

    psoDesc.VS = { vsEntry->vBytecode.data(), vsEntry->vBytecode.size() };
    if (psEntry)
        psoDesc.PS = { psEntry->vBytecode.data(), psEntry->vBytecode.size() };

    psoDesc.InputLayout = { inputElements.data(), static_cast<UINT>(inputElements.size()) };

    // Rasterizer
    psoDesc.RasterizerState.FillMode              = MapFillMode(desc.rasterizer.fillMode);
    psoDesc.RasterizerState.CullMode              = MapCullMode(desc.rasterizer.cullMode);
    psoDesc.RasterizerState.FrontCounterClockwise = desc.rasterizer.bFrontCounterClockwise;
    psoDesc.RasterizerState.DepthBias             = desc.rasterizer.depthBias;
    psoDesc.RasterizerState.SlopeScaledDepthBias  = desc.rasterizer.fSlopeScaledDepthBias;
    psoDesc.RasterizerState.DepthClipEnable       = TRUE;

    // Blend
    for (uint32 i = 0; i < desc.uRenderTargetCount; ++i)
    {
        auto& src = desc.blendTargets[i];
        auto& dst = psoDesc.BlendState.RenderTarget[i];
        dst.BlendEnable    = src.bBlendEnable;
        dst.SrcBlend       = MapBlendFactor(src.srcColor);
        dst.DestBlend      = MapBlendFactor(src.dstColor);
        dst.BlendOp        = MapBlendOp(src.colorOp);
        dst.SrcBlendAlpha  = MapBlendFactor(src.srcAlpha);
        dst.DestBlendAlpha = MapBlendFactor(src.dstAlpha);
        dst.BlendOpAlpha   = MapBlendOp(src.alphaOp);
        dst.RenderTargetWriteMask = src.uWriteMask;
    }

    // Depth stencil
    psoDesc.DepthStencilState.DepthEnable    = desc.depthStencil.bDepthTestEnable;
    psoDesc.DepthStencilState.DepthWriteMask = desc.depthStencil.bDepthWriteEnable
                                               ? D3D12_DEPTH_WRITE_MASK_ALL
                                               : D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.DepthFunc      = MapCompareOp(desc.depthStencil.depthCompareOp);

    // Render targets
    psoDesc.NumRenderTargets = desc.uRenderTargetCount;
    for (uint32 i = 0; i < desc.uRenderTargetCount; ++i)
        psoDesc.RTVFormats[i] = MapFormat(desc.renderTargetFormats[i]);

    if (desc.depthStencilFormat != RHIFormat::Unknown)
        psoDesc.DSVFormat = MapFormat(desc.depthStencilFormat);

    psoDesc.PrimitiveTopologyType = MapPrimitiveTopologyType(desc.topology);
    psoDesc.SampleDesc.Count      = 1;
    psoDesc.SampleMask             = UINT_MAX;

    ComPtr<ID3D12PipelineState> pso;
    hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    if (FAILED(hr))
    {
        EVO_LOG_ERROR("Failed to create graphics pipeline '{}': {}", desc.sDebugName, GetHResultString(hr));
        return {};
    }

    return m_PipelinePool.Allocate(std::move(pso), std::move(rootSig), desc.topology, desc.uPushConstantSize, descTableRootOffset, desc.sDebugName);
}

RHIPipelineHandle DX12Device::CreateComputePipeline(const RHIComputePipelineDesc& /*desc*/)
{
    EVO_LOG_WARN("DX12Device::CreateComputePipeline �?not yet implemented");
    return {};
}

void DX12Device::DestroyBuffer(RHIBufferHandle handle) { m_BufferPool.Free(handle); }
void DX12Device::DestroyTexture(RHITextureHandle handle) { m_TexturePool.Free(handle); }
void DX12Device::DestroyShader(RHIShaderHandle handle) { m_ShaderPool.Free(handle); }
void DX12Device::DestroyPipeline(RHIPipelineHandle handle) { m_PipelinePool.Free(handle); }

// ---- Views ----

RHIRenderTargetView DX12Device::CreateRenderTargetView(RHITextureHandle texture)
{
    auto* entry = m_TexturePool.GetEntry(texture);
    if (!entry || !entry->pResource)
    {
        EVO_LOG_WARN("CreateRenderTargetView: invalid texture handle");
        return {};
    }

    D3D12_CPU_DESCRIPTOR_HANDLE slot = m_RTVAllocator.Allocate();
    if (slot.ptr == 0)
        return {};

    m_pDevice->CreateRenderTargetView(entry->pResource.Get(), nullptr, slot);
    return WrapRTV(slot);
}

void DX12Device::DestroyRenderTargetView(RHIRenderTargetView rtv)
{
    if (!rtv.IsValid())
        return;
    m_RTVAllocator.Free(UnwrapRTV(rtv));
}

RHIDepthStencilView DX12Device::CreateDepthStencilView(RHITextureHandle texture)
{
    auto* entry = m_TexturePool.GetEntry(texture);
    if (!entry || !entry->pResource)
    {
        EVO_LOG_WARN("CreateDepthStencilView: invalid texture handle");
        return {};
    }

    D3D12_CPU_DESCRIPTOR_HANDLE slot = m_DSVAllocator.Allocate();
    if (slot.ptr == 0)
        return {};

    // When the resource uses a typeless format (depth+SRV), provide explicit DSV desc
    D3D12_DEPTH_STENCIL_VIEW_DESC* pDsvDesc = nullptr;
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    if (IsDepthFormat(entry->rhiFormat)
        && entry->pResource->GetDesc().Format != MapFormat(entry->rhiFormat))
    {
        dsvDesc.Format        = MapFormat(entry->rhiFormat);
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        pDsvDesc = &dsvDesc;
    }

    m_pDevice->CreateDepthStencilView(entry->pResource.Get(), pDsvDesc, slot);
    return WrapDSV(slot);
}

void DX12Device::DestroyDepthStencilView(RHIDepthStencilView dsv)
{
    if (!dsv.IsValid())
        return;
    m_DSVAllocator.Free(UnwrapDSV(dsv));
}

DX12GpuDescriptorAllocator::Allocation DX12Device::CreateShaderResourceView(RHITextureHandle texture)
{
    auto* pEntry = m_TexturePool.GetEntry(texture);
    if (!pEntry || !pEntry->pResource)
    {
        EVO_LOG_WARN("CreateShaderResourceView: invalid texture handle");
        return {};
    }

    auto alloc = m_SRVAllocator.Allocate();
    if (!alloc.IsValid())
        return {};

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    // Use SRV-compatible format for depth textures (typeless resource)
    srvDesc.Format = IsDepthFormat(pEntry->rhiFormat)
        ? MapDepthToSRVFormat(pEntry->rhiFormat)
        : pEntry->pResource->GetDesc().Format;
    srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels    = 1;
    m_pDevice->CreateShaderResourceView(pEntry->pResource.Get(), &srvDesc, alloc.cpuHandle);
    return alloc;
}

void DX12Device::DestroyShaderResourceView(const DX12GpuDescriptorAllocator::Allocation& alloc)
{
    m_SRVAllocator.Free(alloc);
}

void* DX12Device::MapBuffer(RHIBufferHandle handle)
{
    auto* entry = m_BufferPool.GetEntry(handle);
    return entry ? entry->pMappedPtr : nullptr;
}

void DX12Device::UnmapBuffer(RHIBufferHandle /*handle*/)
{
    // Upload heap uses persistent mapping �?no explicit unmap needed
}

// ---- Descriptors ----

RHIDescriptorSetLayoutHandle DX12Device::CreateDescriptorSetLayout(const RHIDescriptorSetLayoutDesc& desc)
{
    return m_DescSetLayoutPool.Allocate(desc);
}

void DX12Device::DestroyDescriptorSetLayout(RHIDescriptorSetLayoutHandle handle)
{
    m_DescSetLayoutPool.Free(handle);
}

RHIDescriptorSetHandle DX12Device::AllocateDescriptorSet(RHIDescriptorSetLayoutHandle layout)
{
    auto* layoutEntry = m_DescSetLayoutPool.GetEntry(layout);
    if (!layoutEntry)
    {
        EVO_LOG_ERROR("AllocateDescriptorSet: invalid layout handle");
        return {};
    }

    auto range = m_SRVAllocator.AllocateRange(layoutEntry->uTotalDescriptorCount);
    if (!range.IsValid())
    {
        EVO_LOG_ERROR("AllocateDescriptorSet: failed to allocate {} descriptors",
                      layoutEntry->uTotalDescriptorCount);
        return {};
    }

    return m_DescSetPool.Allocate(layout, range);
}

void DX12Device::FreeDescriptorSet(RHIDescriptorSetHandle handle)
{
    // Note: range descriptors are not individually freed back to the allocator
    // since we use bump allocation for ranges. They'll be reclaimed on reset.
    m_DescSetPool.Free(handle);
}

void DX12Device::WriteDescriptorSet(RHIDescriptorSetHandle set,
    const RHIDescriptorWrite* writes, uint32 writeCount)
{
    auto* setEntry = m_DescSetPool.GetEntry(set);
    if (!setEntry)
    {
        EVO_LOG_ERROR("WriteDescriptorSet: invalid set handle");
        return;
    }

    auto* layoutEntry = m_DescSetLayoutPool.GetEntry(setEntry->layout);
    if (!layoutEntry) return;

    for (uint32 w = 0; w < writeCount; ++w)
    {
        const auto& write = writes[w];

        // Find the descriptor offset within the set for this binding
        uint32 offset = 0;
        bool found = false;
        for (const auto& binding : layoutEntry->vBindings)
        {
            if (binding.uBinding == write.uBinding)
            {
                offset += write.uArrayIndex;
                found = true;
                break;
            }
            offset += binding.uCount;
        }
        if (!found) continue;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_SRVAllocator.GetCpuHandle(
            setEntry->allocation, offset);

        switch (write.type)
        {
            case RHIDescriptorType::ShaderResource:
            {
                auto* texEntry = m_TexturePool.GetEntry(write.texture);
                if (!texEntry || !texEntry->pResource) break;

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = IsDepthFormat(texEntry->rhiFormat)
                    ? MapDepthToSRVFormat(texEntry->rhiFormat)
                    : texEntry->pResource->GetDesc().Format;
                srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Texture2D.MipLevels    = 1;
                m_pDevice->CreateShaderResourceView(texEntry->pResource.Get(), &srvDesc, cpuHandle);
                break;
            }
            case RHIDescriptorType::ConstantBuffer:
            {
                auto* bufEntry = m_BufferPool.GetEntry(write.buffer);
                if (!bufEntry) break;

                D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
                cbvDesc.BufferLocation = bufEntry->uGpuAddress + write.uBufferOffset;
                cbvDesc.SizeInBytes    = static_cast<UINT>(
                    write.uBufferRange > 0 ? write.uBufferRange
                                           : (bufEntry->uSize - write.uBufferOffset));
                cbvDesc.SizeInBytes = (cbvDesc.SizeInBytes + 255) & ~255u; // align to 256
                m_pDevice->CreateConstantBufferView(&cbvDesc, cpuHandle);
                break;
            }
            case RHIDescriptorType::UnorderedAccess:
            {
                // TODO: implement UAV creation
                break;
            }
            case RHIDescriptorType::Sampler:
            {
                // Samplers require a separate heap �?use static samplers for now
                break;
            }
        }
    }
}

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
    return m_TexturePool.Allocate(std::move(resource), debugName, RHIFormat::Unknown, initialBarrier);
}

void DX12Device::UnregisterTextureExternal(RHITextureHandle handle)
{
    m_TexturePool.Free(handle);
}

// ---- Buffer / Shader / Pipeline pool access ----

const DX12BufferEntry* DX12Device::ResolveBuffer(RHIBufferHandle handle) const
{
    return m_BufferPool.GetEntry(handle);
}

const DX12ShaderEntry* DX12Device::ResolveShader(RHIShaderHandle handle) const
{
    return m_ShaderPool.GetEntry(handle);
}

const DX12PipelineEntry* DX12Device::ResolvePipeline(RHIPipelineHandle handle) const
{
    return m_PipelinePool.GetEntry(handle);
}

const DX12DescriptorSetEntry* DX12Device::ResolveDescriptorSet(RHIDescriptorSetHandle handle) const
{
    return m_DescSetPool.GetEntry(handle);
}

// ---- Frame management ----

RHICommandList* DX12Device::AcquireCommandList(RHIQueueType /*type*/)
{
    // Currently only Graphics pool is supported
    return m_GraphicsCmdListPool.Acquire();
}

void DX12Device::BeginFrame(uint64 uCompletedFenceValue)
{
    m_GraphicsCmdListPool.BeginFrame(uCompletedFenceValue);
}

void DX12Device::EndFrame(uint64 uFrameFenceValue)
{
    m_GraphicsCmdListPool.EndFrame(uFrameFenceValue);
}

void DX12Device::WaitIdle()
{
    if (m_pGraphicsQueue) m_pGraphicsQueue->WaitIdle();
    if (m_pComputeQueue)  m_pComputeQueue->WaitIdle();
    if (m_pCopyQueue)     m_pCopyQueue->WaitIdle();
}

void DX12Device::BindGpuDescriptorHeap(RHICommandList* pCmdList)
{
    ID3D12DescriptorHeap* pHeap = m_SRVAllocator.GetHeap();
    if (pHeap)
    {
        void* heapPtr = static_cast<void*>(pHeap);
        pCmdList->SetDescriptorHeaps(1, &heapPtr);
    }
}

} // namespace Evo
