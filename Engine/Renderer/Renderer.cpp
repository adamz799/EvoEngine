#include "Renderer/Renderer.h"
#include "Core/Log.h"

namespace Evo {

Renderer::~Renderer()
{
    Shutdown();
}

bool Renderer::Initialize(const RendererDesc& desc, Window& window)
{
    EVO_LOG_INFO("Renderer initializing...");

    // Create RHI device
    m_Device = CreateRHIDevice(desc.backend);
    if (!m_Device) {
        EVO_LOG_CRITICAL("Failed to create RHI device");
        return false;
    }

    RHIDeviceDesc deviceDesc{};
    deviceDesc.backend      = desc.backend;
    deviceDesc.enableDebug  = desc.enableDebug;
    deviceDesc.windowHandle = window.GetNativeHandle();
    deviceDesc.windowWidth  = window.GetWidth();
    deviceDesc.windowHeight = window.GetHeight();

    if (!m_Device->Initialize(deviceDesc)) {
        EVO_LOG_CRITICAL("Failed to initialize RHI device");
        return false;
    }

    // Create swap chain
    RHISwapChainDesc scDesc{};
    scDesc.windowHandle = window.GetNativeHandle();
    scDesc.width        = window.GetWidth();
    scDesc.height       = window.GetHeight();
    scDesc.bufferCount  = 2;
    scDesc.format       = RHIFormat::R8G8B8A8_UNORM;

    m_SwapChain = m_Device->CreateSwapChain(scDesc);
    // SwapChain is nullptr in stub — that's OK for the skeleton

    EVO_LOG_INFO("Renderer initialized (backend: {})",
        desc.backend == RHIBackendType::DX12 ? "DX12" : "Vulkan");
    return true;
}

void Renderer::Shutdown()
{
    if (m_Device) {
        m_Device->WaitIdle();

        m_SwapChain.reset();
        m_Device->Shutdown();
        m_Device.reset();

        EVO_LOG_INFO("Renderer shut down");
    }
}

void Renderer::BeginFrame()
{
    if (m_Device)
        m_Device->BeginFrame();

    // TODO: acquire back buffer, begin command list, clear render target
}

void Renderer::EndFrame()
{
    // TODO: end command list, submit, present
    if (m_Device)
        m_Device->EndFrame();

    if (m_SwapChain)
        m_SwapChain->Present();
}

} // namespace Evo
