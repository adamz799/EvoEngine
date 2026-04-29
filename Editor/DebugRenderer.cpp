#include "DebugRenderer.h"
#include "Asset/ShaderAsset.h"
#include "Renderer/Renderer.h"
#include "Core/Log.h"

#include <cstring>

namespace Evo {

bool DebugRenderer::Initialize(RHIDevice* pDevice, AssetManager& assetManager, RHIFormat rtFormat)
{
#if EVO_RHI_DX12
	// Load debug line shader
	m_ShaderHandle = assetManager.LoadSync("Assets/Shaders/DebugLine.hlsl");
	auto* pShader = assetManager.Get<ShaderAsset>(m_ShaderHandle);
	if (!pShader)
	{
		EVO_LOG_ERROR("DebugRenderer: failed to load DebugLine shader");
		return false;
	}

	// Input layout: position(3) + color(4)
	RHIInputElement inputElements[] = {
		{ "POSITION", 0, RHIFormat::R32G32B32_FLOAT,    0, 0 },
		{ "COLOR",    0, RHIFormat::R32G32B32A32_FLOAT, 12, 0 },
	};

	RHIGraphicsPipelineDesc pipelineDesc = {};
	pipelineDesc.vertexShader         = pShader->GetVertexShader();
	pipelineDesc.pixelShader          = pShader->GetPixelShader();
	pipelineDesc.pInputElements       = inputElements;
	pipelineDesc.uInputElementCount   = 2;
	pipelineDesc.rasterizer.cullMode  = RHICullMode::None;
	pipelineDesc.depthStencil.bDepthTestEnable = false;
	pipelineDesc.uRenderTargetCount   = 1;
	pipelineDesc.renderTargetFormats[0] = rtFormat;
	pipelineDesc.topology             = RHIPrimitiveTopology::LineList;
	pipelineDesc.uPushConstantSize    = sizeof(float) * 16;  // 4x4 ViewProjection
	pipelineDesc.sDebugName           = "DebugLinePSO";
	m_LinePipeline = pDevice->CreateGraphicsPipeline(pipelineDesc);

	if (!m_LinePipeline.IsValid())
		return false;

	// Create upload buffer for line vertices
	RHIBufferDesc bufDesc = {};
	bufDesc.uSize     = kMaxVertices * sizeof(DebugLineVertex);
	bufDesc.usage     = RHIBufferUsage::Vertex;
	bufDesc.memory    = RHIMemoryUsage::CpuToGpu;
	bufDesc.sDebugName = "DebugLineVB";
	m_LineBuffer = pDevice->CreateBuffer(bufDesc);

	if (!m_LineBuffer.IsValid())
		return false;

	m_pMappedData = pDevice->MapBuffer(m_LineBuffer);

	EVO_LOG_INFO("DebugRenderer initialized");
	return true;
#else
	return false;
#endif
}

void DebugRenderer::Shutdown(RHIDevice* pDevice)
{
	if (m_LineBuffer.IsValid())
	{
		pDevice->UnmapBuffer(m_LineBuffer);
		m_pMappedData = nullptr;
		pDevice->DestroyBuffer(m_LineBuffer);
	}
	if (m_LinePipeline.IsValid())
		pDevice->DestroyPipeline(m_LinePipeline);
}

void DebugRenderer::DrawLine(const Vec3& from, const Vec3& to, const Vec4& color)
{
	DebugLineVertex v0 = { from.x, from.y, from.z, color.x, color.y, color.z, color.w };
	DebugLineVertex v1 = { to.x,   to.y,   to.z,   color.x, color.y, color.z, color.w };
	m_vVertices.push_back(v0);
	m_vVertices.push_back(v1);
}

void DebugRenderer::DrawFrustum(const Camera& camera, const Vec4& color)
{
	// Unproject 8 NDC corners through inverse VP to get world-space frustum corners
	Mat4 invVP = camera.GetViewProjectionMatrix().Inverse();

	// NDC corners: (x, y, z) where x,y in {-1,+1}, z in {0,1} (DX convention)
	Vec3 corners[8];
	int idx = 0;
	for (float z : { 0.0f, 1.0f })
	{
		for (float y : { -1.0f, 1.0f })
		{
			for (float x : { -1.0f, 1.0f })
			{
				corners[idx++] = invVP.TransformPoint(Vec3(x, y, z));
			}
		}
	}

	// Near plane edges (z=0): corners 0-3
	DrawLine(corners[0], corners[1], color);
	DrawLine(corners[1], corners[3], color);
	DrawLine(corners[3], corners[2], color);
	DrawLine(corners[2], corners[0], color);

	// Far plane edges (z=1): corners 4-7
	DrawLine(corners[4], corners[5], color);
	DrawLine(corners[5], corners[7], color);
	DrawLine(corners[7], corners[6], color);
	DrawLine(corners[6], corners[4], color);

	// Connecting edges (near to far)
	DrawLine(corners[0], corners[4], color);
	DrawLine(corners[1], corners[5], color);
	DrawLine(corners[2], corners[6], color);
	DrawLine(corners[3], corners[7], color);
}

void DebugRenderer::DrawCameraIcon(const Camera& camera, const Vec4& color, float fSize)
{
	Vec3 pos     = camera.GetPosition();
	Vec3 forward = camera.GetForward();
	Vec3 right   = camera.GetRight();
	Vec3 up      = camera.GetUp();

	// Small pyramid: apex at camera position, base offset along forward
	float fBaseOffset = fSize;
	float fBaseHalf   = fSize * 0.5f;

	Vec3 baseCenter = pos + forward * fBaseOffset;
	Vec3 c0 = baseCenter + right * fBaseHalf + up * fBaseHalf;
	Vec3 c1 = baseCenter - right * fBaseHalf + up * fBaseHalf;
	Vec3 c2 = baseCenter - right * fBaseHalf - up * fBaseHalf;
	Vec3 c3 = baseCenter + right * fBaseHalf - up * fBaseHalf;

	// Apex to base corners
	DrawLine(pos, c0, color);
	DrawLine(pos, c1, color);
	DrawLine(pos, c2, color);
	DrawLine(pos, c3, color);

	// Base rectangle
	DrawLine(c0, c1, color);
	DrawLine(c1, c2, color);
	DrawLine(c2, c3, color);
	DrawLine(c3, c0, color);
}

void DebugRenderer::Render(Renderer& renderer, RGHandle target, RHIRenderTargetView rtv,
                           const Mat4& viewProj, float fViewportWidth, float fViewportHeight)
{
	if (m_vVertices.empty())
		return;

	// Clamp to buffer capacity
	uint32 uVertexCount = static_cast<uint32>(m_vVertices.size());
	if (uVertexCount > kMaxVertices)
		uVertexCount = kMaxVertices;

	// Copy vertices to mapped upload buffer
	std::memcpy(m_pMappedData, m_vVertices.data(), uVertexCount * sizeof(DebugLineVertex));

	// Capture values for lambda
	RHIPipelineHandle pipeline = m_LinePipeline;
	RHIBufferHandle   buffer   = m_LineBuffer;
	Mat4 vp = viewProj;
	float w = fViewportWidth;
	float h = fViewportHeight;
	uint32 count = uVertexCount;

	auto& rg = renderer.GetRenderGraph();
	rg.AddPass("DebugLines", [&](RGPassBuilder& builder) {
		builder.WriteRenderTarget(target, rtv);
	}, [pipeline, buffer, vp, w, h, count](RHICommandList* pCmdList) {
		pCmdList->SetPipeline(pipeline);

		RHIViewport viewport = {};
		viewport.fWidth  = w;
		viewport.fHeight = h;
		viewport.fMaxDepth = 1.0f;
		pCmdList->SetViewport(viewport);

		RHIScissorRect scissor = {};
		scissor.right  = static_cast<int32>(w);
		scissor.bottom = static_cast<int32>(h);
		pCmdList->SetScissorRect(scissor);

		pCmdList->SetPushConstants(&vp, sizeof(Mat4));

		RHIVertexBufferView vbv = {};
		vbv.buffer  = buffer;
		vbv.uStride = sizeof(DebugLineVertex);
		vbv.uSize   = count * sizeof(DebugLineVertex);
		pCmdList->SetVertexBuffer(0, vbv);

		pCmdList->Draw(count);
	});

	m_vVertices.clear();
}

} // namespace Evo
