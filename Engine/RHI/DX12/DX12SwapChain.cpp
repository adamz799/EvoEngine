#include "RHI/DX12/DX12SwapChain.h"
#include "RHI/DX12/DX12Device.h"
#include "Core/Log.h"

namespace Evo {

DX12SwapChain::~DX12SwapChain()
{
	ShutdownSwapChain();
}

bool DX12SwapChain::Initialize(DX12Device* device, const RHISwapChainDesc& desc)
{
	if (!device)
		return false;

	m_pDevice      = device;
	m_uWidth       = desc.uWidth;
	m_uHeight      = desc.uHeight;
	m_uBufferCount = desc.uBufferCount;
	m_Format      = desc.format;
	m_bVsync       = desc.bVsync;

	// TODO Phase 1:
	// 1. DXGI_SWAP_CHAIN_DESC1 scDesc = { width, height, DXGI_FORMAT_R8G8B8A8_UNORM, ... };
	//    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//    scDesc.BufferCount = bufferCount;
	// 2. factory->CreateSwapChainForHwnd(graphicsQueue, hwnd, &scDesc, ...) → swapChain1
	// 3. swapChain1.As(&m_pSwapChain) to get IDXGISwapChain3
	// 4. CreateRenderTargetViews()

	EVO_LOG_INFO("DX12SwapChain::Initialize (stub) {}x{}", m_uWidth, m_uHeight);

	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width                 = m_uWidth;
	scDesc.Height                = m_uHeight;
	scDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.SampleDesc.Count      = 1;
	scDesc.SampleDesc.Quality    = 0;
	scDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount           = m_uBufferCount;
	scDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	ComPtr<IDXGISwapChain1> swapChain1;
	if (FAILED(m_pDevice->GetDXGIFactory()->CreateSwapChainForHwnd(
		m_pDevice->GetGraphicsCommandQueue(),
		static_cast<HWND>(desc.pWindowHandle),
		&scDesc,
		nullptr,
		nullptr,
		&swapChain1)))
	{
		EVO_LOG_ERROR("Failed to create swap chain");
		return false;
	}

	if (FAILED(swapChain1.As(&m_pSwapChain)))
	{
		EVO_LOG_ERROR("Failed to get IDXGISwapChain3");
		return false;
	}

	CreateRenderTargetViews();

	return true;
}

void DX12SwapChain::ShutdownSwapChain()
{
	ReleaseRenderTargetViews();
	m_pSwapChain.Reset();
	m_pDevice = nullptr;
}

void DX12SwapChain::Present()
{
	m_pSwapChain->Present(m_bVsync ? 1 : 0, 0);
}

void DX12SwapChain::Resize(uint32 width, uint32 height)
{
	// TODO:
	ReleaseRenderTargetViews();
	m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
	CreateRenderTargetViews();
	m_uWidth = width;
	m_uHeight = height;
}

uint32 DX12SwapChain::GetCurrentBackBufferIndex() const
{
	return m_pSwapChain->GetCurrentBackBufferIndex();
}

RHITextureHandle DX12SwapChain::GetCurrentBackBuffer()
{
	return m_vBackBufferHandles[m_pSwapChain->GetCurrentBackBufferIndex()];
}

RHIRenderTargetView DX12SwapChain::GetCurrentBackBufferRTV()
{
	return m_vBackBufferRTVs[m_pSwapChain->GetCurrentBackBufferIndex()];
}

ID3D12Resource* DX12SwapChain::GetBackBufferResource(uint32 index) const
{
	return m_vBackBuffers[index].Get();
}

void DX12SwapChain::CreateRenderTargetViews()
{
	for (UINT i = 0; i < m_uBufferCount; ++i)
	{
		HRESULT hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_vBackBuffers[i]));
		if (FAILED(hr))
		{
			EVO_LOG_ERROR("Failed to get swap chain buffer {}: {}", i, GetHResultString(hr));
			return;
		}

		wchar_t bufferName[64];
		swprintf_s(bufferName, L"SwapChainBackBuffer%u", i);
		m_vBackBuffers[i]->SetName(bufferName);

		// Register resource in texture pool
		RHIBarrierState initialState;
		initialState.currentSync   = RHIBarrierSync::None;
		initialState.currentAccess = RHIBarrierAccess::Common;
		initialState.currentLayout = RHITextureLayout::Common;

		char name[64];
		snprintf(name, sizeof(name), "SwapChainBackBuffer%u", i);

		m_vBackBufferHandles[i] = m_pDevice->RegisterTextureExternal(
			m_vBackBuffers[i], name, initialState);

		// Create RTV via Device
		m_vBackBufferRTVs[i] = m_pDevice->CreateRenderTargetView(m_vBackBufferHandles[i]);
	}
}

void DX12SwapChain::ReleaseRenderTargetViews()
{
	for (UINT i = 0; i < m_uBufferCount; ++i)
	{
		if (m_vBackBufferRTVs[i].IsValid())
		{
			m_pDevice->DestroyRenderTargetView(m_vBackBufferRTVs[i]);
			m_vBackBufferRTVs[i] = {};
		}
		if (m_vBackBufferHandles[i].IsValid())
		{
			m_pDevice->UnregisterTextureExternal(m_vBackBufferHandles[i]);
			m_vBackBufferHandles[i] = {};
		}
		m_vBackBuffers[i].Reset();
	}
}

} // namespace Evo

