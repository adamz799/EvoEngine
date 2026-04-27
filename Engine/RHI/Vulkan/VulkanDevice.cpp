#include "RHI/Vulkan/VulkanDevice.h"
#include "Core/Log.h"

namespace Evo {

VulkanDevice::~VulkanDevice() { Shutdown(); }

bool VulkanDevice::Initialize(const RHIDeviceDesc& /*desc*/)
{
    EVO_LOG_INFO("VulkanDevice::Initialize (stub)");
    m_AdapterName = "Vulkan Device (stub — not yet implemented)";
    return true;
}

void VulkanDevice::Shutdown()
{
    EVO_LOG_INFO("VulkanDevice::Shutdown (stub)");
}

std::unique_ptr<RHISwapChain>   VulkanDevice::CreateSwapChain(const RHISwapChainDesc&) { return nullptr; }
std::unique_ptr<RHICommandList> VulkanDevice::CreateCommandList(RHIQueueType) { return nullptr; }
std::unique_ptr<RHIFence>       VulkanDevice::CreateFence(u64) { return nullptr; }

RHIBufferHandle   VulkanDevice::CreateBuffer(const RHIBufferDesc&) { return {}; }
RHITextureHandle  VulkanDevice::CreateTexture(const RHITextureDesc&) { return {}; }
RHIShaderHandle   VulkanDevice::CreateShader(const RHIShaderDesc&) { return {}; }
RHIPipelineHandle VulkanDevice::CreateGraphicsPipeline(const RHIGraphicsPipelineDesc&) { return {}; }
RHIPipelineHandle VulkanDevice::CreateComputePipeline(const RHIComputePipelineDesc&) { return {}; }

void VulkanDevice::DestroyBuffer(RHIBufferHandle) {}
void VulkanDevice::DestroyTexture(RHITextureHandle) {}
void VulkanDevice::DestroyShader(RHIShaderHandle) {}
void VulkanDevice::DestroyPipeline(RHIPipelineHandle) {}

void* VulkanDevice::MapBuffer(RHIBufferHandle) { return nullptr; }
void  VulkanDevice::UnmapBuffer(RHIBufferHandle) {}

RHIDescriptorSetLayoutHandle VulkanDevice::CreateDescriptorSetLayout(const RHIDescriptorSetLayoutDesc&) { return {}; }
void VulkanDevice::DestroyDescriptorSetLayout(RHIDescriptorSetLayoutHandle) {}
RHIDescriptorSetHandle VulkanDevice::AllocateDescriptorSet(RHIDescriptorSetLayoutHandle) { return {}; }
void VulkanDevice::FreeDescriptorSet(RHIDescriptorSetHandle) {}
void VulkanDevice::WriteDescriptorSet(RHIDescriptorSetHandle, const RHIDescriptorWrite*, u32) {}

void VulkanDevice::BeginFrame() {}
void VulkanDevice::EndFrame() {}
void VulkanDevice::WaitIdle() {}

} // namespace Evo
