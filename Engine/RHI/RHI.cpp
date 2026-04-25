#include "RHI/RHIDevice.h"
#include "Core/Log.h"
#include "RHIConfig.h"

#if EVO_RHI_DX12
#include "RHI/DX12/DX12Device.h"
#endif

#if EVO_RHI_VULKAN
#include "RHI/Vulkan/VulkanDevice.h"
#endif

namespace Evo {

std::unique_ptr<RHIDevice> CreateRHIDevice(RHIBackendType backend)
{
    switch (backend) {
#if EVO_RHI_DX12
        case RHIBackendType::DX12: {
            EVO_LOG_INFO("Creating DX12 RHI device...");
            return std::make_unique<DX12Device>();
        }
#endif

#if EVO_RHI_VULKAN
        case RHIBackendType::Vulkan: {
            EVO_LOG_INFO("Creating Vulkan RHI device...");
            return std::make_unique<VulkanDevice>();
        }
#endif

        default:
            EVO_LOG_ERROR("Requested RHI backend is not available in this build");
            return nullptr;
    }
}

} // namespace Evo
