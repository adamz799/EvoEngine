#pragma once

#include "RHI/RHIDevice.h"

namespace Evo {

class VulkanDevice : public RHIDevice {
public:
    VulkanDevice() = default;
    ~VulkanDevice() override;

    bool Initialize(const RHIDeviceDesc& desc) override;
    void Shutdown() override;

    RHIBackendType     GetBackendType() const override { return RHIBackendType::Vulkan; }
    const std::string& GetAdapterName() const override { return m_AdapterName; }

    // ---- Queue access ----
    RHIQueue* GetGraphicsQueue() override { return nullptr; }
    RHIQueue* GetComputeQueue()  override { return nullptr; }
    RHIQueue* GetCopyQueue()     override { return nullptr; }

    // ---- Direct object creation ----
    std::unique_ptr<RHISwapChain>   CreateSwapChain(const RHISwapChainDesc& desc) override;
    std::unique_ptr<RHICommandList> CreateCommandList(RHIQueueType type) override;
    std::unique_ptr<RHIFence>       CreateFence(uint64 initialValue) override;

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

    // ---- Frame management ----
    void BeginFrame() override;
    void EndFrame() override;
    void WaitIdle() override;

private:
    std::string m_AdapterName = "Vulkan Device (stub)";
};

} // namespace Evo
