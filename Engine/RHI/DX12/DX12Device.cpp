#include "RHI/DX12/DX12Device.h"
#include "Core/Log.h"

namespace Evo {

DX12Device::~DX12Device()
{
    Shutdown();
}

bool DX12Device::Initialize(const RHIDeviceDesc& desc)
{
    EVO_LOG_INFO("DX12Device::Initialize (stub)");

    // TODO: Real DX12 initialization:
    //   1. CreateDXGIFactory2 (with debug flag)
    //   2. EnumAdapters → choose best GPU
    //   3. D3D12CreateDevice
    //   4. Create command queue, allocator, fence
    //   5. Enable debug layer if desc.enableDebug

    m_AdapterName = "DX12 Device (stub — not yet implemented)";
    EVO_LOG_INFO("DX12 adapter: {}", m_AdapterName);
    return true;
}

void DX12Device::Shutdown()
{
    EVO_LOG_INFO("DX12Device::Shutdown (stub)");
    // TODO: Release all DX12 objects
}

std::unique_ptr<RHISwapChain> DX12Device::CreateSwapChain(const RHISwapChainDesc& /*desc*/)
{
    EVO_LOG_WARN("DX12Device::CreateSwapChain — not yet implemented");
    return nullptr;
}

std::unique_ptr<RHICommandList> DX12Device::CreateCommandList(RHICommandListType /*type*/)
{
    EVO_LOG_WARN("DX12Device::CreateCommandList — not yet implemented");
    return nullptr;
}

std::unique_ptr<RHIBuffer> DX12Device::CreateBuffer(const RHIBufferDesc& /*desc*/)
{
    EVO_LOG_WARN("DX12Device::CreateBuffer — not yet implemented");
    return nullptr;
}

std::unique_ptr<RHITexture> DX12Device::CreateTexture(const RHITextureDesc& /*desc*/)
{
    EVO_LOG_WARN("DX12Device::CreateTexture — not yet implemented");
    return nullptr;
}

void DX12Device::BeginFrame()
{
    // TODO: Wait for previous frame's fence, reset command allocator
}

void DX12Device::EndFrame()
{
    // TODO: Signal fence
}

void DX12Device::SubmitCommandList(RHICommandList* /*cmdList*/)
{
    // TODO: Execute command list on the queue
}

void DX12Device::WaitIdle()
{
    // TODO: Flush queue and wait for fence
}

} // namespace Evo
