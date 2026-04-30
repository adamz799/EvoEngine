#include "TriangleDemo.h"
#include "Renderer/Renderer.h"
#include "Core/Log.h"

#if EVO_RHI_DX12
#include "RHI/DX12/DX12ShaderCompiler.h"
#endif

#include <cstring>

namespace Evo {

struct TriangleVertex {
	float pos[3];
	float color[4];
};

bool TriangleDemo::Initialize(RHIDevice* pDevice, RHIFormat rtFormat)
{
#if EVO_RHI_DX12
	// Compile shaders
	auto vsBlob = CompileShaderFromFile("Assets/Shaders/Triangle.hlsl", "VSMain", "vs_5_1");
	auto psBlob = CompileShaderFromFile("Assets/Shaders/Triangle.hlsl", "PSMain", "ps_5_1");
	if (!vsBlob || !psBlob)
		return false;

	RHIShaderDesc vsDesc = {};
	vsDesc.pBytecode     = vsBlob->GetBufferPointer();
	vsDesc.uBytecodeSize = vsBlob->GetBufferSize();
	vsDesc.stage         = RHIShaderStage::Vertex;
	vsDesc.sDebugName    = "TriangleVS";
	m_TriangleVS = pDevice->CreateShader(vsDesc);

	RHIShaderDesc psDesc = {};
	psDesc.pBytecode     = psBlob->GetBufferPointer();
	psDesc.uBytecodeSize = psBlob->GetBufferSize();
	psDesc.stage         = RHIShaderStage::Pixel;
	psDesc.sDebugName    = "TrianglePS";
	m_TrianglePS = pDevice->CreateShader(psDesc);

	if (!m_TriangleVS.IsValid() || !m_TrianglePS.IsValid())
		return false;

	// Input layout
	RHIInputElement inputElements[] = {
		{ "POSITION", 0, RHIFormat::R32G32B32_FLOAT,    0,  0 },
		{ "COLOR",    0, RHIFormat::R32G32B32A32_FLOAT, 12, 0 },
	};

	// Pipeline
	RHIGraphicsPipelineDesc pipelineDesc = {};
	pipelineDesc.vertexShader         = m_TriangleVS;
	pipelineDesc.pixelShader          = m_TrianglePS;
	pipelineDesc.pInputElements       = inputElements;
	pipelineDesc.uInputElementCount   = 2;
	pipelineDesc.rasterizer.cullMode  = RHICullMode::None;
	pipelineDesc.depthStencil.bDepthTestEnable = false;
	pipelineDesc.uRenderTargetCount   = 1;
	pipelineDesc.renderTargetFormats[0] = rtFormat;
	pipelineDesc.topology             = RHIPrimitiveTopology::TriangleList;
	pipelineDesc.sDebugName           = "TrianglePSO";
	m_TrianglePipeline = pDevice->CreateGraphicsPipeline(pipelineDesc);

	if (!m_TrianglePipeline.IsValid())
		return false;

	// Vertex buffer (3 vertices, upload heap)
	TriangleVertex vertices[] = {
		{ {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },  // top, red
		{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },  // right, green
		{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },  // left, blue
	};

	RHIBufferDesc vbDesc = {};
	vbDesc.uSize      = sizeof(vertices);
	vbDesc.usage      = RHIBufferUsage::Vertex;
	vbDesc.memory     = RHIMemoryUsage::CpuToGpu;
	vbDesc.sDebugName = "TriangleVB";
	m_TriangleVB = pDevice->CreateBuffer(vbDesc);

	if (!m_TriangleVB.IsValid())
		return false;

	void* mapped = pDevice->MapBuffer(m_TriangleVB);
	std::memcpy(mapped, vertices, sizeof(vertices));

	EVO_LOG_INFO("Triangle resources created");
	return true;
#else
	return false;
#endif
}

void TriangleDemo::Shutdown(RHIDevice* pDevice)
{
	if (m_TrianglePipeline.IsValid()) pDevice->DestroyPipeline(m_TrianglePipeline);
	if (m_TriangleVS.IsValid())       pDevice->DestroyShader(m_TriangleVS);
	if (m_TrianglePS.IsValid())       pDevice->DestroyShader(m_TrianglePS);
	if (m_TriangleVB.IsValid())       pDevice->DestroyBuffer(m_TriangleVB);
}

void TriangleDemo::AddPasses(Renderer& renderer)
{
	auto& graph = renderer.GetRenderGraph();
	RGHandle backBuffer = renderer.GetBackBufferRG();
	RHIRenderTargetView backBufferRTV = renderer.GetSwapChain()->GetCurrentBackBufferRTV();

	float w = static_cast<float>(renderer.GetSwapChain()->GetWidth());
	float h = static_cast<float>(renderer.GetSwapChain()->GetHeight());
	RHIColor clearColor = { 0.2f, 0.3f, 0.4f, 1.0f };

	graph.AddPass("TrianglePass",
		[backBuffer, backBufferRTV, clearColor](RGPassBuilder& builder) {
			builder.WriteRenderTarget(backBuffer, backBufferRTV, &clearColor);
		},
		[this, w, h](RHICommandList* pCmdList) {
			RHIViewport vp = { 0, 0, w, h, 0, 1 };
			pCmdList->SetViewport(vp);
			RHIScissorRect sr = { 0, 0, static_cast<int32>(w), static_cast<int32>(h) };
			pCmdList->SetScissorRect(sr);

			pCmdList->SetPipeline(m_TrianglePipeline);

			RHIVertexBufferView vbView = {};
			vbView.buffer = m_TriangleVB;
			vbView.uStride = sizeof(TriangleVertex);
			pCmdList->SetVertexBuffer(0, vbView);

			pCmdList->Draw(3, 1, 0, 0);
		});
}

} // namespace Evo

