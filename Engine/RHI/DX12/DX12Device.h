#pragma once

#include "RHI/RHIDevice.h"
#include "RHI/DX12/DX12Common.h"
#include "RHI/DX12/DX12Queue.h"
#include "RHI/DX12/DX12TexturePool.h"
#include "RHI/DX12/DX12BufferPool.h"
#include "RHI/DX12/DX12ShaderPool.h"
#include "RHI/DX12/DX12PipelinePool.h"
#include "RHI/DX12/DX12DescriptorAllocator.h"
#include "RHI/DX12/DX12GpuDescriptorAllocator.h"
#include "RHI/DX12/DX12CommandListPool.h"
#include <D3D12MemAlloc.h>

namespace Evo {

class DX12Device : public RHIDevice {
public:
    DX12Device() = default;
    ~DX12Device() override;

    bool Initialize(const RHIDeviceDesc& desc) override;
    void Shutdown() override;

    RHIBackendType     GetBackendType() const override { return RHIBackendType::DX12; }
    const std::string& GetAdapterName() const override { return m_sAdapterName; }

    // ---- Queue access ----
    RHIQueue* GetGraphicsQueue() override;
    RHIQueue* GetComputeQueue()  override;
    RHIQueue* GetCopyQueue()     override;

    // ---- Direct object creation ----
    std::unique_ptr<RHISwapChain>   CreateSwapChain(const RHISwapChainDesc& desc) override;
    std::unique_ptr<RHICommandList> CreateCommandList(RHIQueueType type) override;
    std::unique_ptr<RHIFence>       CreateFence(uint64 initialValue = 0) override;

    // ---- Handle resources ----
    RHIBufferHandle   CreateBuffer(const RHIBufferDesc& desc) override;
    RHITextureHandle  CreateTexture(const RHITextureDesc& desc) override;
    RHIShaderHandle   CreateShader(const RHIShaderDesc& desc) override;
    RHIPipelineHandle CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc) override;
    RHIPipelineHandle CreateComputePipeline(const RHIComputePipelineDesc& desc) override;

    void DestroyBuffer(RHIBufferHandle handle) override;
    void DestroyTexture(RHITextureHandle handle) override;
    void DestroyShader(RHIShaderHandle handle) override;
    void DestroyPipeline(RHIPipelineHandle handle) override;

    // ---- Views ----
    RHIRenderTargetView CreateRenderTargetView(RHITextureHandle texture) override;
    void                DestroyRenderTargetView(RHIRenderTargetView rtv) override;

    // ---- Buffer ops ----
    void* MapBuffer(RHIBufferHandle handle) override;
    void  UnmapBuffer(RHIBufferHandle handle) override;

    // ---- Descriptors ----
    RHIDescriptorSetLayoutHandle CreateDescriptorSetLayout(
        const RHIDescriptorSetLayoutDesc& desc) override;
    void DestroyDescriptorSetLayout(RHIDescriptorSetLayoutHandle handle) override;

    RHIDescriptorSetHandle AllocateDescriptorSet(
        RHIDescriptorSetLayoutHandle layout) override;
    void FreeDescriptorSet(RHIDescriptorSetHandle handle) override;
    void WriteDescriptorSet(RHIDescriptorSetHandle set,
        const RHIDescriptorWrite* writes, uint32 writeCount) override;

    // ---- CommandList pool ----
    RHICommandList* AcquireCommandList(RHIQueueType type) override;

    // ---- Frame management ----
    void BeginFrame(uint64 uCompletedFenceValue) override;
    void EndFrame(uint64 uFrameFenceValue) override;
    void WaitIdle() override;

    // ---- Native handle accessors (for ImGui and other integrations) ----
    ID3D12Device*        GetD3D12Device()   const { return m_pDevice.Get(); }
    IDXGIFactory4*       GetDXGIFactory()   const { return m_pDxgiFactory.Get(); }

    // Return the native DX12 graphics command queue (needed by SwapChain creation, ImGui, etc.)
    ID3D12CommandQueue*  GetGraphicsCommandQueue() const;

    // Return the HWND stored during SwapChain creation
    HWND                 GetHWND() const { return m_HWND; }
    void                 SetHWND(HWND hwnd) { m_HWND = hwnd; }

    // ---- GPU-visible SRV descriptor heap (for ImGui, texture bindings) ----
    DX12GpuDescriptorAllocator& GetSRVAllocator() { return m_SRVAllocator; }
    ID3D12DescriptorHeap*       GetSRVHeap() const { return m_SRVAllocator.GetHeap(); }

    /// Create a shader resource view and return its GPU descriptor allocation.
    DX12GpuDescriptorAllocator::Allocation CreateShaderResourceView(RHITextureHandle texture);
    void DestroyShaderResourceView(const DX12GpuDescriptorAllocator::Allocation& alloc);

    // ---- Texture pool access ----
    const DX12TextureEntry* ResolveTexture(RHITextureHandle handle) const;
    DX12TextureEntry*       ResolveTextureMutable(RHITextureHandle handle);

    RHITextureHandle RegisterTextureExternal(ComPtr<ID3D12Resource> resource,
                                              const std::string& debugName,
                                              const RHIBarrierState& initialBarrier = {});
    void UnregisterTextureExternal(RHITextureHandle handle);

    // ---- Buffer / Shader / Pipeline pool access ----
    const DX12BufferEntry*   ResolveBuffer(RHIBufferHandle handle) const;
    const DX12ShaderEntry*   ResolveShader(RHIShaderHandle handle) const;
    const DX12PipelineEntry* ResolvePipeline(RHIPipelineHandle handle) const;

private:
    std::string m_sAdapterName;

    ComPtr<IDXGIFactory4>   m_pDxgiFactory;
    ComPtr<ID3D12Device>    m_pDevice;
    D3D12MA::Allocator*     m_pAllocator = nullptr;

    std::unique_ptr<DX12Queue> m_pGraphicsQueue;
    std::unique_ptr<DX12Queue> m_pComputeQueue;
    std::unique_ptr<DX12Queue> m_pCopyQueue;

    HWND m_HWND = nullptr;

    DX12TexturePool m_TexturePool;
    DX12BufferPool  m_BufferPool;
    DX12ShaderPool  m_ShaderPool;
    DX12PipelinePool m_PipelinePool;
    DX12CpuDescriptorAllocator m_RTVAllocator;
    DX12GpuDescriptorAllocator m_SRVAllocator;
    DX12CommandListPool        m_GraphicsCmdListPool;
};

} // namespace Evo
