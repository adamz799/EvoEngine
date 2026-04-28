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
	m_pRHIDevice = CreateRHIDevice(desc.backend);
	if (!m_pRHIDevice) {
		EVO_LOG_CRITICAL("Failed to create RHI device");
		return false;
	}

	RHIDeviceDesc deviceDesc{};
	deviceDesc.backend     = desc.backend;
	deviceDesc.enableDebug = desc.enableDebug;

	if (!m_pRHIDevice->Initialize(deviceDesc)) {
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

	m_pSwapChain = m_pRHIDevice->CreateSwapChain(scDesc);

	// Create frame fence
	m_pFrameFence = m_pRHIDevice->CreateFence(0);

	EVO_LOG_INFO("Renderer initialized (backend: {})",
		desc.backend == RHIBackendType::DX12 ? "DX12" : "Vulkan");
	return true;
}

void Renderer::Shutdown()
{
	if (m_pRHIDevice) 
	{
		m_pRHIDevice->WaitIdle();

		m_pFrameFence.reset();
		m_pSwapChain.reset();
		m_pRHIDevice->Shutdown();
		m_pRHIDevice.reset();

		EVO_LOG_INFO("Renderer shut down");
	}
}

void Renderer::BeginFrame()
{
	if (m_pRHIDevice)
		m_pRHIDevice->BeginFrame();

	m_uFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	m_pFrameFence->CpuWait(m_vFenceValues[m_uFrameIndex]);

	auto pCmdList = m_pRHIDevice->CreateCommandList(RHIQueueType::Graphics);
	pCmdList->Begin();

	RHITextureHandle BackBuffer = m_pSwapChain->GetCurrentBackBuffer();
	RHIRenderTargetView backBufferRTV = m_pSwapChain->GetCurrentBackBufferRTV();

	// Back buffer: Present → RenderTarget
	RHITextureBarrier barrier = {};
	barrier.texture = BackBuffer;
	barrier.accessBefore = RHIBarrierAccess::Common;
	barrier.accessAfter = RHIBarrierAccess::RenderTarget;
	barrier.layoutBefore = RHITextureLayout::Common;
	barrier.layoutAfter = RHITextureLayout::RenderTarget;
	barrier.syncBefore = RHIBarrierSync::All;
	barrier.syncAfter = RHIBarrierSync::RenderTarget;
	pCmdList->TextureBarrier(&barrier, 1);

	RHIColor clearColor = { 0.2f, 0.3f, 0.4f, 1.0f };
	pCmdList->ClearRenderTarget(backBufferRTV, clearColor);
}

void Renderer::EndFrame()
{
	// TODO Phase 1: transition back buffer to present, submit, signal fence

	if (m_pRHIDevice)
		m_pRHIDevice->EndFrame();

	if (m_pSwapChain)
		m_pSwapChain->Present();

	m_uCurrentFrame++;
}

} // namespace Evo
