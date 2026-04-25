#pragma once

#include "RHI/RHITypes.h"
#include "RHI/RHISwapChain.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIBuffer.h"
#include "RHI/RHITexture.h"
#include "RHI/RHIPipeline.h"
#include <memory>

namespace Evo {

class RHIDevice {
public:
    virtual ~RHIDevice() = default;

    virtual bool Initialize(const RHIDeviceDesc& desc) = 0;
    virtual void Shutdown() = 0;

    /// Get the backend type this device is using.
    virtual RHIBackendType GetBackendType() const = 0;

    /// Get the adapter/GPU name.
    virtual const std::string& GetAdapterName() const = 0;

    // ---- Factory methods ----
    virtual std::unique_ptr<RHISwapChain>   CreateSwapChain(const RHISwapChainDesc& desc) = 0;
    virtual std::unique_ptr<RHICommandList> CreateCommandList(RHICommandListType type) = 0;
    virtual std::unique_ptr<RHIBuffer>      CreateBuffer(const RHIBufferDesc& desc) = 0;
    virtual std::unique_ptr<RHITexture>     CreateTexture(const RHITextureDesc& desc) = 0;

    // ---- Per-frame operations ----
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

    /// Submit a command list for execution.
    virtual void SubmitCommandList(RHICommandList* cmdList) = 0;

    /// Wait for GPU idle (flush all queued work).
    virtual void WaitIdle() = 0;
};

/// Factory: create an RHI device for the specified backend.
std::unique_ptr<RHIDevice> CreateRHIDevice(RHIBackendType backend);

} // namespace Evo
