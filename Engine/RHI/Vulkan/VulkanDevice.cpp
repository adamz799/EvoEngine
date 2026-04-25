#include "RHI/Vulkan/VulkanDevice.h"
#include "Core/Log.h"

namespace Evo {

VulkanDevice::~VulkanDevice()
{
    Shutdown();
}

bool VulkanDevice::Initialize(const RHIDeviceDesc& desc)
{
    EVO_LOG_INFO("VulkanDevice::Initialize (stub)");

    // TODO: Real Vulkan initialization:
    //   1. vkCreateInstance (with validation layers if desc.enableDebug)
    //   2. Pick physical device
    //   3. vkCreateDevice with queue families
    //   4. Create command pool

    m_AdapterName = "Vulkan Device (stub — not yet implemented)";
    EVO_LOG_INFO("Vulkan adapter: {}", m_AdapterName);
    return true;
}

void VulkanDevice::Shutdown()
{
    EVO_LOG_INFO("VulkanDevice::Shutdown (stub)");
    // TODO: vkDestroyDevice, vkDestroyInstance, etc.
}

std::unique_ptr<RHISwapChain> VulkanDevice::CreateSwapChain(const RHISwapChainDesc& /*desc*/)
{
    EVO_LOG_WARN("VulkanDevice::CreateSwapChain — not yet implemented");
    return nullptr;
}

std::unique_ptr<RHICommandList> VulkanDevice::CreateCommandList(RHICommandListType /*type*/)
{
    EVO_LOG_WARN("VulkanDevice::CreateCommandList — not yet implemented");
    return nullptr;
}

std::unique_ptr<RHIBuffer> VulkanDevice::CreateBuffer(const RHIBufferDesc& /*desc*/)
{
    EVO_LOG_WARN("VulkanDevice::CreateBuffer — not yet implemented");
    return nullptr;
}

std::unique_ptr<RHITexture> VulkanDevice::CreateTexture(const RHITextureDesc& /*desc*/)
{
    EVO_LOG_WARN("VulkanDevice::CreateTexture — not yet implemented");
    return nullptr;
}

void VulkanDevice::BeginFrame()
{
    // TODO: vkAcquireNextImageKHR, reset command buffer
}

void VulkanDevice::EndFrame()
{
    // TODO: vkQueueSubmit, vkQueuePresentKHR
}

void VulkanDevice::SubmitCommandList(RHICommandList* /*cmdList*/)
{
    // TODO: vkQueueSubmit
}

void VulkanDevice::WaitIdle()
{
    // TODO: vkDeviceWaitIdle
}

} // namespace Evo
