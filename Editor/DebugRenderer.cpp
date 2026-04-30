#include "DebugRenderer.h"
#include "Asset/ShaderAsset.h"
#include "Renderer/Renderer.h"
#include "Core/Log.h"

#include <cstring>

namespace Evo {

struct DebugLinePushConstants {
	Mat4    viewProj;
	float32 fScreenWidth;
	float32 fScreenHeight;
	float32 fLineWidth;
	float32 _pad;
};

bool DebugRenderer::Initialize(Render* pRender, AssetManager& assetManager)
{
#if EVO_RHI_DX12
	auto* pDevice  = pRender->GetDevice();
	auto rtFormat  = pRender->GetSwapChain()->GetFormat();

	// Load debug line shader
	m_ShaderHandle = assetManager.LoadSync("Assets/Shaders/DebugLine.hlsl");
	auto* pShader = assetManager.Get<ShaderAsset>(m_ShaderHandle);
	if (!pShader)
	{
		EVO_LOG_ERROR("DebugRenderer: failed to load DebugLine shader");
		return false;
	}

	// Input layout: position(3) + other(3) + color(4) + edgeDist(1)
	RHIInputElement inputElements[] = {
		{ "POSITION",  0, RHIFormat::R32G32B32_FLOAT,    0,  0 },
		{ "TEXCOORD",  0, RHIFormat::R32G32B32_FLOAT,    12, 0 },
		{ "COLOR",     0, RHIFormat::R32G32B32A32_FLOAT, 24, 0 },
		{ "TEXCOORD",  1, RHIFormat::R32_FLOAT,          40, 0 },
	};

	RHIGraphicsPipelineDesc pipelineDesc = {};
	pipelineDesc.vertexShader         = pShader->GetVertexShader();
	pipelineDesc.pixelShader          = pShader->GetPixelShader();
	pipelineDesc.pInputElements       = inputElements;
	pipelineDesc.uInputElementCount   = 4;
	pipelineDesc.rasterizer.cullMode  = RHICullMode::None;
	pipelineDesc.depthStencil.bDepthTestEnable  = false;
	pipelineDesc.depthStencil.bDepthWriteEnable = false;
	pipelineDesc.uRenderTargetCount   = 1;
	pipelineDesc.renderTargetFormats[0] = rtFormat;
	pipelineDesc.topology             = RHIPrimitiveTopology::TriangleList;
	pipelineDesc.uPushConstantSize    = sizeof(DebugLinePushConstants);
	pipelineDesc.sDebugName           = "DebugLinePSO";

	// Alpha blending for smooth edges
	pipelineDesc.blendTargets[0].bBlendEnable = true;
	pipelineDesc.blendTargets[0].srcColor     = RHIBlendFactor::SrcAlpha;
	pipelineDesc.blendTargets[0].dstColor     = RHIBlendFactor::InvSrcAlpha;
	pipelineDesc.blendTargets[0].colorOp      = RHIBlendOp::Add;
	pipelineDesc.blendTargets[0].srcAlpha     = RHIBlendFactor::One;
	pipelineDesc.blendTargets[0].dstAlpha     = RHIBlendFactor::InvSrcAlpha;
	pipelineDesc.blendTargets[0].alphaOp      = RHIBlendOp::Add;

	m_LinePipeline = pDevice->CreateGraphicsPipeline(pipelineDesc);

	if (!m_LinePipeline.IsValid())
		return false;

	// Create upload buffer for expanded quad vertices
	RHIBufferDesc bufDesc = {};
	bufDesc.uSize      = kMaxVertices * sizeof(DebugLineVertex);
	bufDesc.usage      = RHIBufferUsage::Vertex;
	bufDesc.memory     = RHIMemoryUsage::CpuToGpu;
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

void DebugRenderer::Shutdown(Render* pRender)
{
	auto* pDevice = pRender->GetDevice();
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
	m_vLines.push_back({ from, to, color });
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

void DebugRenderer::DrawTranslationGizmo(const Vec3& position, float fSize, int iHighlightAxis)
{
	const Vec3 axes[3] = { Vec3(1,0,0), Vec3(0,1,0), Vec3(0,0,1) };
	const Vec4 colors[3] = {
		Vec4(1.0f, 0.15f, 0.15f, 1.0f),   // X red
		Vec4(0.15f, 1.0f, 0.15f, 1.0f),   // Y green
		Vec4(0.3f, 0.3f, 1.0f, 1.0f),     // Z blue
	};

	for (int i = 0; i < 3; ++i)
	{
		Vec4 color = colors[i];
		if (iHighlightAxis >= 0 && iHighlightAxis != i)
			color.w = 0.3f;   // dim non-highlighted axes

		Vec3 tip = position + axes[i] * fSize;
		DrawLine(position, tip, color);

		// Arrowhead: small cone at tip
		float fArrowLen  = fSize * 0.15f;
		float fArrowHalf = fSize * 0.05f;

		// Pick two perpendicular directions to the axis
		Vec3 perp1, perp2;
		if (i == 1) // Y axis
		{
			perp1 = Vec3(1, 0, 0);
			perp2 = Vec3(0, 0, 1);
		}
		else if (i == 0) // X axis
		{
			perp1 = Vec3(0, 1, 0);
			perp2 = Vec3(0, 0, 1);
		}
		else // Z axis
		{
			perp1 = Vec3(1, 0, 0);
			perp2 = Vec3(0, 1, 0);
		}

		Vec3 base = tip - axes[i] * fArrowLen;
		Vec3 a0 = base + perp1 * fArrowHalf;
		Vec3 a1 = base - perp1 * fArrowHalf;
		Vec3 a2 = base + perp2 * fArrowHalf;
		Vec3 a3 = base - perp2 * fArrowHalf;

		DrawLine(tip, a0, color);
		DrawLine(tip, a1, color);
		DrawLine(tip, a2, color);
		DrawLine(tip, a3, color);
	}
}

void DebugRenderer::RenderLines(Render* pRender, RGHandle target, RHIRenderTargetView rtv,
						   const Mat4& viewProj, float fViewportWidth, float fViewportHeight)
{
	if (m_vLines.empty())
		return;

	// Expand each line into 6 vertices (2 triangles forming a quad)
	uint32 uLineCount = static_cast<uint32>(m_vLines.size());
	uint32 uVertexCount = uLineCount * 6;
	if (uVertexCount > kMaxVertices)
	{
		uLineCount = kMaxVertices / 6;
		uVertexCount = uLineCount * 6;
	}

	auto* pVerts = static_cast<DebugLineVertex*>(m_pMappedData);
	uint32 vi = 0;

	for (uint32 i = 0; i < uLineCount; ++i)
	{
		const RawLine& line = m_vLines[i];

		// Quad: 2 triangles
		// (A,-1)----(B,-1)
		//   | \        |
		//   |  \       |
		// (A,+1)----(B,+1)
		//
		// Tri 1: (A,-1), (B,-1), (A,+1)
		// Tri 2: (B,-1), (B,+1), (A,+1)

		auto emit = [&](const Vec3& pos, const Vec3& other, const Vec4& col, float edge) {
			pVerts[vi].fPosX   = pos.x;
			pVerts[vi].fPosY   = pos.y;
			pVerts[vi].fPosZ   = pos.z;
			pVerts[vi].fOtherX = other.x;
			pVerts[vi].fOtherY = other.y;
			pVerts[vi].fOtherZ = other.z;
			pVerts[vi].fColorR = col.x;
			pVerts[vi].fColorG = col.y;
			pVerts[vi].fColorB = col.z;
			pVerts[vi].fColorA = col.w;
			pVerts[vi].fEdgeDist = edge;
			++vi;
		};

		emit(line.from, line.to,   line.color, -1.0f);  // Tri 1
		emit(line.to,   line.from, line.color, -1.0f);
		emit(line.from, line.to,   line.color, +1.0f);

		emit(line.to,   line.from, line.color, -1.0f);  // Tri 2
		emit(line.to,   line.from, line.color, +1.0f);
		emit(line.from, line.to,   line.color, +1.0f);
	}

	// Capture values for lambda
	RHIPipelineHandle pipeline = m_LinePipeline;
	RHIBufferHandle   buffer   = m_LineBuffer;

	DebugLinePushConstants pc = {};
	pc.viewProj      = viewProj;
	pc.fScreenWidth  = fViewportWidth;
	pc.fScreenHeight = fViewportHeight;
	pc.fLineWidth    = 2.0f;
	pc._pad          = 0.0f;

	float w = fViewportWidth;
	float h = fViewportHeight;
	uint32 count = uVertexCount;

	auto& rg = pRender->GetRenderGraph();
	rg.AddPass("DebugLines", [&](RGPassBuilder& builder) {
		builder.WriteRenderTarget(target, rtv);
	}, [pipeline, buffer, pc, w, h, count](RHICommandList* pCmdList) {
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

		pCmdList->SetPushConstants(&pc, sizeof(DebugLinePushConstants));

		RHIVertexBufferView vbv = {};
		vbv.buffer  = buffer;
		vbv.uStride = sizeof(DebugLineVertex);
		vbv.uSize   = count * sizeof(DebugLineVertex);
		pCmdList->SetVertexBuffer(0, vbv);

		pCmdList->Draw(count);
	});

	m_vLines.clear();
}

} // namespace Evo

