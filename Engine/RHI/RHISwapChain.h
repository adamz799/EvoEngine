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
    virtual void Resize(u32 width, u32 height) = 0;

    /// Index of the current back buffer (0..bufferCount-1).
    virtual u32 GetCurrentBackBufferIndex() const = 0;

    /// Handle to the current back buffer texture.
    virtual RHITextureHandle GetCurrentBackBuffer() = 0;

    virtual u32       GetWidth()  const = 0;
    virtual u32       GetHeight() const = 0;
    virtual RHIFormat GetFormat() const = 0;
};

} // namespace Evo
