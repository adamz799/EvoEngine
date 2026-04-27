#pragma once

#include "RHI/RHI.h"
#include "Platform/Window.h"
#include <memory>

namespace Evo {

struct RendererDesc {
    RHIBackendType backend = RHIBackendType::DX12;
    bool enableDebug       = true;
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool Initialize(const RendererDesc& desc, Window& window);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    RHIDevice* GetDevice() const { return m_Device.get(); }

private:
    std::unique_ptr<RHIDevice>    m_Device;
    std::unique_ptr<RHISwapChain> m_SwapChain;
    std::unique_ptr<RHIFence>     m_FrameFence;
    u64                           m_FrameCount = 0;
};

} // namespace Evo
