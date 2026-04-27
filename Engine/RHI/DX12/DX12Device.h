#pragma once

#include "RHI/RHIDevice.h"
#include "RHI/DX12/DX12Common.h"
#include "RHI/DX12/DX12Queue.h"

namespace Evo {

class DX12Device : public RHIDevice {
public:
    DX12Device() = default;
    ~DX12Device() override;

    bool Initialize(const RHIDeviceDesc& desc) override;
    void Shutdown() override;

    RHIBackendType     GetBackendType() const override { return RHIBackendType::DX12; }
    const std::string& GetAdapterName() const override { return m_AdapterName; }

    // ---- Queue access ----
    RHIQueue* GetGraphicsQueue() override;
    RHIQueue* GetComputeQueue()  override;
    RHIQueue* GetCopyQueue()     override;

    // ---- Direct object creation ----
    std::unique_ptr<RHISwapChain>   CreateSwapChain(const RHISwapChainDesc& desc) override;
    std::unique_ptr<RHICommandList> CreateCommandList(RHIQueueType type) override;
    std::unique_ptr<RHIFence>       CreateFence(u64 initialValue = 0) override;

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
        const RHIDescriptorWrite* writes, u32 writeCount) override;

    // ---- Frame management ----
    void BeginFrame() override;
    void EndFrame() override;
    void WaitIdle() override;

    // ---- Native handle accessors (for ImGui and other integrations) ----
    ID3D12Device*        GetD3D12Device()   const { return m_Device.Get(); }
    IDXGIFactory4*       GetDxgiFactory()   const { return m_DxgiFactory.Get(); }

private:
    std::string m_AdapterName;

    // TODO: Fill in during Phase 1 implementation
    ComPtr<IDXGIFactory4>  m_DxgiFactory;
    ComPtr<ID3D12Device>   m_Device;

    std::unique_ptr<DX12Queue> m_GraphicsQueue;
    std::unique_ptr<DX12Queue> m_ComputeQueue;
    std::unique_ptr<DX12Queue> m_CopyQueue;
};

} // namespace Evo
