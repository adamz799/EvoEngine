#pragma once

#include "RHI/RHITypes.h"
#include "RHI/RHITexture.h"
#include <memory>

namespace Evo {

class RHISwapChain {
public:
    virtual ~RHISwapChain() = default;

    virtual bool Initialize(const RHISwapChainDesc& desc) = 0;
    virtual void Shutdown() = 0;

    virtual void Present() = 0;
    virtual void Resize(u32 width, u32 height) = 0;

    virtual u32  GetCurrentBackBufferIndex() const = 0;
    virtual RHITexture* GetCurrentBackBuffer() = 0;

    virtual u32  GetWidth()  const = 0;
    virtual u32  GetHeight() const = 0;
};

} // namespace Evo
