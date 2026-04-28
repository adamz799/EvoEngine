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
	void Resize(uint32 width, uint32 height) override;

	uint32              GetCurrentBackBufferIndex() const override;
	RHITextureHandle GetCurrentBackBuffer() override;
	RHIRenderTargetView GetCurrentBackBufferRTV() override;
	uint32              GetWidth()  const override { return m_Width; }
	uint32              GetHeight() const override { return m_Height; }
	RHIFormat        GetFormat() const override { return m_Format; }

	// Native accessors
	IDXGISwapChain3*            GetDxgiSwapChain() const { return m_pSwapChain.Get(); }
	ID3D12Resource*             GetBackBufferResource(uint32 index) const;

private:
	void CreateRenderTargetViews();
	void ReleaseRenderTargetViews();

	DX12Device* m_pDevice = nullptr;   // non-owning

	ComPtr<IDXGISwapChain3> m_pSwapChain;
	uint32       m_Width       = 0;
	uint32       m_Height      = 0;
	uint32       m_BufferCount = NUM_BACK_FRAMES;
	RHIFormat m_Format      = RHIFormat::R8G8B8A8_UNORM;
	bool      m_Vsync       = true;

	 ComPtr<ID3D12Resource>       m_BackBuffers[NUM_BACK_FRAMES];
	 RHITextureHandle            m_BackBufferHandles[NUM_BACK_FRAMES] = {};
	 RHIRenderTargetView         m_BackBufferRTVs[NUM_BACK_FRAMES] = {};
};

} // namespace Evo
