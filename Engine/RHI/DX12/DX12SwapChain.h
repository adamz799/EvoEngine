#pragma once

#include "RHI/RHISwapChain.h"
#include "RHI/DX12/DX12Common.h"

namespace Evo {

class DX12Device;

class DX12SwapChain : public RHISwapChain {
public:
    DX12SwapChain() = default;
    ~DX12SwapChain() override;

    bool Initialize(DX12Device* device, const RHISwapChainDesc& desc);
    void ShutdownSwapChain();

    void Present() override;
    void Resize(u32 width, u32 height) override;

    u32              GetCurrentBackBufferIndex() const override;
    RHITextureHandle GetCurrentBackBuffer() override;
    u32              GetWidth()  const override { return m_Width; }
    u32              GetHeight() const override { return m_Height; }
    RHIFormat        GetFormat() const override { return m_Format; }

    // Native accessors
    IDXGISwapChain3*            GetDxgiSwapChain() const { return m_SwapChain.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRTV(u32 index) const;
    ID3D12Resource*             GetBackBufferResource(u32 index) const;

private:
    void CreateRenderTargetViews();
    void ReleaseRenderTargetViews();

    DX12Device* m_Device = nullptr;   // non-owning

    ComPtr<IDXGISwapChain3> m_SwapChain;
    u32       m_Width       = 0;
    u32       m_Height      = 0;
    u32       m_BufferCount = 2;
    RHIFormat m_Format      = RHIFormat::R8G8B8A8_UNORM;
    bool      m_Vsync       = true;

    // TODO: back buffer resources and RTV descriptor heap
    // ComPtr<ID3D12Resource>      m_BackBuffers[3];
    // ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
    // UINT m_RtvDescriptorSize = 0;
    // RHITextureHandle m_BackBufferHandles[3];
};

} // namespace Evo
