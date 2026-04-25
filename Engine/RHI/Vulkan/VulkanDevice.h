#pragma once

#include "RHI/RHIDevice.h"

namespace Evo {

class VulkanDevice : public RHIDevice {
public:
    VulkanDevice() = default;
    ~VulkanDevice() override;

    bool Initialize(const RHIDeviceDesc& desc) override;
    void Shutdown() override;

    RHIBackendType    GetBackendType() const override { return RHIBackendType::Vulkan; }
    const std::string& GetAdapterName() const override { return m_AdapterName; }

    std::unique_ptr<RHISwapChain>   CreateSwapChain(const RHISwapChainDesc& desc) override;
    std::unique_ptr<RHICommandList> CreateCommandList(RHICommandListType type) override;
    std::unique_ptr<RHIBuffer>      CreateBuffer(const RHIBufferDesc& desc) override;
    std::unique_ptr<RHITexture>     CreateTexture(const RHITextureDesc& desc) override;

    void BeginFrame() override;
    void EndFrame() override;
    void SubmitCommandList(RHICommandList* cmdList) override;
    void WaitIdle() override;

private:
    std::string m_AdapterName = "Vulkan Device (stub)";

    // TODO: VkInstance, VkPhysicalDevice, VkDevice, VkQueue, etc.
};

} // namespace Evo
