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
	deviceDesc.backend                 = desc.backend;
	deviceDesc.bEnableDebugLayer        = desc.bEnableDebugLayer;
	deviceDesc.bEnableGPUBasedValidation = desc.bEnableGPUBasedValidation;

	if (!m_pRHIDevice->Initialize(deviceDesc)) {
		EVO_LOG_CRITICAL("Failed to initialize RHI device");
		return false;
	}

	// Create swap chain
	RHISwapChainDesc scDesc{};
	scDesc.pWindowHandle = window.GetNativeHandle();
	scDesc.uWidth        = window.GetWidth();
	scDesc.uHeight       = window.GetHeight();
	scDesc.uBufferCount  = 2;
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

void Renderer::HandleResize(uint32 uWidth, uint32 uHeight)
{
	if (!m_pSwapChain || uWidth == 0 || uHeight == 0)
		return;
	if (uWidth == m_pSwapChain->GetWidth() && uHeight == m_pSwapChain->GetHeight())
		return;

	m_pRHIDevice->WaitIdle();
	m_pSwapChain->Resize(uWidth, uHeight);
	EVO_LOG_INFO("Swap chain resized: {}x{}", uWidth, uHeight);
}

void Renderer::BeginFrame()
{
	m_uFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	m_pFrameFence->CpuWait(m_vFenceValues[m_uFrameIndex]);

	// Recycle pool entries whose GPU work has completed
	m_pRHIDevice->BeginFrame(m_pFrameFence->GetCompletedValue());

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
	// Compile and execute the render graph (each pass gets its own CmdList)
	m_RenderGraph.Compile();

	std::vector<RHICommandList*> cmdLists;
	m_RenderGraph.Execute(m_pRHIDevice.get(), cmdLists);

	// Submit all CmdLists in order + signal fence
	m_uCurrentFrame++;
	m_pRHIDevice->GetGraphicsQueue()->Submit(
		cmdLists.data(), static_cast<uint32>(cmdLists.size()),
		nullptr, 0,
		m_pFrameFence.get(), m_uCurrentFrame);
	m_vFenceValues[m_uFrameIndex] = m_uCurrentFrame;

	m_pSwapChain->Present();

	// Mark acquired pool entries as in-flight
	m_pRHIDevice->EndFrame(m_uCurrentFrame);
}

} // namespace Evo
