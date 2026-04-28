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

	// Pre-create command lists (one per back buffer)
	for (uint32 i = 0; i < NUM_BACK_FRAMES; ++i)
	{
		m_vCmdLists[i] = m_pRHIDevice->CreateCommandList(RHIQueueType::Graphics);
		if (!m_vCmdLists[i])
		{
			EVO_LOG_CRITICAL("Failed to create command list {}", i);
			return false;
		}
	}

	EVO_LOG_INFO("Renderer initialized (backend: {})",
		desc.backend == RHIBackendType::DX12 ? "DX12" : "Vulkan");

	return true;
}

void Renderer::Shutdown()
{
	if (m_pRHIDevice)
	{
		m_pRHIDevice->WaitIdle();

		for (auto& cmd : m_vCmdLists)
			cmd.reset();
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

	m_vCmdLists[m_uFrameIndex]->Begin();

	// Prepare render graph for this frame
	m_RenderGraph.Reset();
	m_BackBufferRG = m_RenderGraph.ImportTexture(
		"BackBuffer",
		m_pSwapChain->GetCurrentBackBuffer(),
		RHITextureLayout::Common,
		RHITextureLayout::Common);
}

void Renderer::EndFrame()
{
	auto* pCmdList = m_vCmdLists[m_uFrameIndex].get();

	// Compile and execute the render graph (barriers + passes + final barriers)
	m_RenderGraph.Compile();
	m_RenderGraph.Execute(pCmdList);

	pCmdList->End();

	// Submit + signal fence
	RHICommandList* cmdListPtr = pCmdList;
	m_uCurrentFrame++;
	m_pRHIDevice->GetGraphicsQueue()->Submit(
		&cmdListPtr, 1,
		nullptr, 0,
		m_pFrameFence.get(), m_uCurrentFrame);
	m_vFenceValues[m_uFrameIndex] = m_uCurrentFrame;

	m_pSwapChain->Present();

	if (m_pRHIDevice)
		m_pRHIDevice->EndFrame();
}

} // namespace Evo
