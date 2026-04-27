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

	m_Device      = device;
	m_Width       = desc.width;
	m_Height      = desc.height;
	m_BufferCount = desc.bufferCount;
	m_Format      = desc.format;
	m_Vsync       = desc.vsync;

	// TODO Phase 1:
	// 1. DXGI_SWAP_CHAIN_DESC1 scDesc = { width, height, DXGI_FORMAT_R8G8B8A8_UNORM, ... };
	//    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//    scDesc.BufferCount = bufferCount;
	// 2. factory->CreateSwapChainForHwnd(graphicsQueue, hwnd, &scDesc, ...) → swapChain1
	// 3. swapChain1.As(&m_SwapChain) to get IDXGISwapChain3
	// 4. CreateRenderTargetViews()

	EVO_LOG_INFO("DX12SwapChain::Initialize (stub) {}x{}", m_Width, m_Height);

	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width                 = m_Width;
	scDesc.Height                = m_Height;
	scDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.SampleDesc.Count      = 1;
	scDesc.SampleDesc.Quality    = 0;
	scDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount           = m_BufferCount;
	scDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	ComPtr<IDXGISwapChain1> swapChain1;
	if (FAILED(m_Device->GetDXGIFactory()->CreateSwapChainForHwnd(
		m_Device->GetGraphicsCommandQueue(),
		static_cast<HWND>(desc.windowHandle),
		&scDesc,
		nullptr,
		nullptr,
		&swapChain1)))
	{
		EVO_LOG_ERROR("Failed to create swap chain");
		return false;
	}

	if (FAILED(swapChain1.As(&m_SwapChain)))
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
	m_SwapChain.Reset();
	m_Device = nullptr;
}

void DX12SwapChain::Present()
{
	// TODO: m_SwapChain->Present(m_Vsync ? 1 : 0, 0);
}

void DX12SwapChain::Resize(uint32 width, uint32 height)
{
	// TODO:
	// ReleaseRenderTargetViews();
	// m_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
	// CreateRenderTargetViews();
	m_Width  = width;
	m_Height = height;
}

uint32 DX12SwapChain::GetCurrentBackBufferIndex() const
{
	// TODO: return m_SwapChain->GetCurrentBackBufferIndex();
	return 0;
}

RHITextureHandle DX12SwapChain::GetCurrentBackBuffer()
{
	// TODO: return m_BackBufferHandles[GetCurrentBackBufferIndex()];
	return {};
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12SwapChain::GetBackBufferRTV(uint32 /*index*/) const
{
	// TODO: compute from RTV heap start + index * descriptorSize
	return {};
}

ID3D12Resource* DX12SwapChain::GetBackBufferResource(uint32 /*index*/) const
{
	// TODO: return m_BackBuffers[index].Get();
	return nullptr;
}

void DX12SwapChain::CreateRenderTargetViews()
{
	// TODO Phase 1:
	// Create RTV descriptor heap, get back buffer resources, create RTVs

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = m_BufferCount;
	rtvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask       = 0;
	if (FAILED(m_Device->GetD3D12Device()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap))))
	{
		EVO_LOG_ERROR("Failed to create RTV descriptor heap");
	}
}

void DX12SwapChain::ReleaseRenderTargetViews()
{
	// TODO: release back buffer ComPtrs
}

} // namespace Evo
