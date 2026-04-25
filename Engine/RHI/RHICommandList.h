#pragma once

#include "RHI/RHITypes.h"
#include "RHI/RHIBuffer.h"
#include "RHI/RHITexture.h"
#include <memory>

namespace Evo {

class RHICommandList {
public:
    virtual ~RHICommandList() = default;

    virtual void Begin() = 0;
    virtual void End() = 0;

    virtual void SetViewport(const RHIViewport& viewport) = 0;
    virtual void SetScissorRect(const RHIScissorRect& rect) = 0;

    virtual void ClearRenderTarget(RHITexture* target, const RHIColor& color) = 0;
    virtual void ClearDepthStencil(RHITexture* target, f32 depth, u8 stencil) = 0;

    // TODO: Draw, Dispatch, Barrier, CopyBuffer, etc.
};

} // namespace Evo
