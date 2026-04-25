#pragma once

#include "RHI/RHIDevice.h"

namespace Evo {

class DX12Device : public RHIDevice {
public:
    DX12Device() = default;
    ~DX12Device() override;

    bool Initialize(const RHIDeviceDesc& desc) override;
    void Shutdown() override;

    RHIBackendType    GetBackendType() const override { return RHIBackendType::DX12; }
    const std::string& GetAdapterName() const override { return m_AdapterName; }

    std::unique_ptr<RHISwapChain>   CreateSwapChain(const RHISwapChainDesc& desc) override;
    std::unique_ptr<RHICommandList> CreateCommandList(RHICommandListType type) override;
    std::unique_ptr<RHIBuffer>      CreateBuffer(const RHIBufferDesc& desc) override;
    std::unique_ptr<RHITexture>     CreateTexture(const RHITextureDesc& desc) override;

    void BeginFrame() override;
    void EndFrame() override;
    void SubmitCommandList(RHICommandList* cmdList) override;
    void WaitIdle() override;

private:
    std::string m_AdapterName = "DX12 Device (stub)";

    // TODO: ID3D12Device*, IDXGIFactory*, IDXGIAdapter*, command queues, etc.
};

} // namespace Evo
