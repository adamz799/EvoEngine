#include "Renderer/Renderer.h"
#include "Core/Log.h"

#if EVO_RHI_DX12
#include "RHI/DX12/DX12ShaderCompiler.h"
#endif

namespace Evo {

struct TriangleVertex {
	float pos[3];
	float color[4];
};

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

	// ---- Hello triangle resources ----
	if (!CreateTriangleResources())
	{
		EVO_LOG_ERROR("Failed to create triangle resources");
		return false;
	}

	return true;
}

void Renderer::Shutdown()
{
	if (m_pRHIDevice) 
	{
		m_pRHIDevice->WaitIdle();

		// Release triangle resources
		if (m_TrianglePipeline.IsValid()) m_pRHIDevice->DestroyPipeline(m_TrianglePipeline);
		if (m_TriangleVS.IsValid())       m_pRHIDevice->DestroyShader(m_TriangleVS);
		if (m_TrianglePS.IsValid())       m_pRHIDevice->DestroyShader(m_TrianglePS);
		if (m_TriangleVB.IsValid())       m_pRHIDevice->DestroyBuffer(m_TriangleVB);

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

	auto* pCmdList = m_vCmdLists[m_uFrameIndex].get();
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

	// Set render targets + viewport + scissor for draw calls
	pCmdList->SetRenderTargets(&backBufferRTV, 1);

	float w = static_cast<float>(m_pSwapChain->GetWidth());
	float h = static_cast<float>(m_pSwapChain->GetHeight());
	RHIViewport vp = { 0, 0, w, h, 0, 1 };
	pCmdList->SetViewport(vp);
	RHIScissorRect sr = { 0, 0, static_cast<int32>(m_pSwapChain->GetWidth()),
	                            static_cast<int32>(m_pSwapChain->GetHeight()) };
	pCmdList->SetScissorRect(sr);

	// Draw hello triangle
	if (m_TrianglePipeline.IsValid())
	{
		pCmdList->SetPipeline(m_TrianglePipeline);

		RHIVertexBufferView vbView = {};
		vbView.buffer = m_TriangleVB;
		vbView.stride = sizeof(TriangleVertex);
		pCmdList->SetVertexBuffer(0, vbView);

		pCmdList->Draw(3, 1, 0, 0);
	}
}

void Renderer::EndFrame()
{
	auto* pCmdList = m_vCmdLists[m_uFrameIndex].get();

	// Back buffer: RenderTarget → Present
	RHITextureHandle BackBuffer = m_pSwapChain->GetCurrentBackBuffer();
	RHITextureBarrier barrier = {};
	barrier.texture      = BackBuffer;
	barrier.syncBefore   = RHIBarrierSync::RenderTarget;
	barrier.syncAfter    = RHIBarrierSync::All;
	barrier.accessBefore = RHIBarrierAccess::RenderTarget;
	barrier.accessAfter  = RHIBarrierAccess::Common;
	barrier.layoutBefore = RHITextureLayout::RenderTarget;
	barrier.layoutAfter  = RHITextureLayout::Common;
	pCmdList->TextureBarrier(&barrier, 1);

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

bool Renderer::CreateTriangleResources()
{
#if EVO_RHI_DX12
	// Compile shaders
	auto vsBlob = CompileShaderFromFile("Assets/Shaders/Triangle.hlsl", "VSMain", "vs_5_1");
	auto psBlob = CompileShaderFromFile("Assets/Shaders/Triangle.hlsl", "PSMain", "ps_5_1");
	if (!vsBlob || !psBlob)
		return false;

	RHIShaderDesc vsDesc = {};
	vsDesc.bytecode     = vsBlob->GetBufferPointer();
	vsDesc.bytecodeSize = vsBlob->GetBufferSize();
	vsDesc.stage        = RHIShaderStage::Vertex;
	vsDesc.debugName    = "TriangleVS";
	m_TriangleVS = m_pRHIDevice->CreateShader(vsDesc);

	RHIShaderDesc psDesc = {};
	psDesc.bytecode     = psBlob->GetBufferPointer();
	psDesc.bytecodeSize = psBlob->GetBufferSize();
	psDesc.stage        = RHIShaderStage::Pixel;
	psDesc.debugName    = "TrianglePS";
	m_TrianglePS = m_pRHIDevice->CreateShader(psDesc);

	if (!m_TriangleVS.IsValid() || !m_TrianglePS.IsValid())
		return false;

	// Input layout
	RHIInputElement inputElements[] = {
		{ "POSITION", 0, RHIFormat::R32G32B32_FLOAT,    0,  0 },
		{ "COLOR",    0, RHIFormat::R32G32B32A32_FLOAT, 12, 0 },
	};

	// Pipeline
	RHIGraphicsPipelineDesc pipelineDesc = {};
	pipelineDesc.vertexShader      = m_TriangleVS;
	pipelineDesc.pixelShader       = m_TrianglePS;
	pipelineDesc.inputElements     = inputElements;
	pipelineDesc.inputElementCount = 2;
	pipelineDesc.rasterizer.cullMode = RHICullMode::None;
	pipelineDesc.depthStencil.depthTestEnable = false;
	pipelineDesc.renderTargetCount = 1;
	pipelineDesc.renderTargetFormats[0] = m_pSwapChain->GetFormat();
	pipelineDesc.topology          = RHIPrimitiveTopology::TriangleList;
	pipelineDesc.debugName         = "TrianglePSO";
	m_TrianglePipeline = m_pRHIDevice->CreateGraphicsPipeline(pipelineDesc);

	if (!m_TrianglePipeline.IsValid())
		return false;

	// Vertex buffer (3 vertices, upload heap)
	TriangleVertex vertices[] = {
		{ {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },  // top, red
		{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },  // right, green
		{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },  // left, blue
	};

	RHIBufferDesc vbDesc = {};
	vbDesc.size      = sizeof(vertices);
	vbDesc.usage     = RHIBufferUsage::Vertex;
	vbDesc.memory    = RHIMemoryUsage::CpuToGpu;
	vbDesc.debugName = "TriangleVB";
	m_TriangleVB = m_pRHIDevice->CreateBuffer(vbDesc);

	if (!m_TriangleVB.IsValid())
		return false;

	void* mapped = m_pRHIDevice->MapBuffer(m_TriangleVB);
	memcpy(mapped, vertices, sizeof(vertices));

	EVO_LOG_INFO("Triangle resources created");
	return true;
#else
	return false;
#endif
}

} // namespace Evo
