#pragma once

#include "RHI/RHITypes.h"

namespace Evo {

/// Swap chain — manages presentation of rendered frames to a window.
///
/// DX12:   IDXGISwapChain3
/// Vulkan: VkSwapchainKHR
class RHISwapChain {
public:
    virtual ~RHISwapChain() = default;

    /// Present the current back buffer to the screen.
    virtual void Present() = 0;

    /// Resize back buffers (call after window resize, GPU must be idle).
    virtual void Resize(uint32 width, uint32 height) = 0;

    /// Index of the current back buffer (0..bufferCount-1).
    virtual uint32 GetCurrentBackBufferIndex() const = 0;

    /// Handle to the current back buffer texture (resource).
    virtual RHITextureHandle GetCurrentBackBuffer() = 0;

    /// Render target view for the current back buffer.
    virtual RHIRenderTargetView GetCurrentBackBufferRTV() = 0;

    virtual uint32       GetWidth()  const = 0;
    virtual uint32       GetHeight() const = 0;
    virtual RHIFormat GetFormat() const = 0;
};

} // namespace Evo
