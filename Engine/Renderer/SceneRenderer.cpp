#include "Renderer/SceneRenderer.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"
#include "Scene/MeshAsset.h"
#include "Core/Log.h"

#include <algorithm>

namespace Evo {

struct GBufferPushConstants {
	Mat4  mvp;
	Vec3  vAlbedoColor;
	float fRoughness;
	float fMetallic;
	Vec3  _pad;
};

void SceneRenderer::RenderScene(Scene* pScene, Render* pRender,
								RHIPipelineHandle defaultPipeline,
								const Mat4& viewProj,
								RGHandle targetTexture,
								RHIRenderTargetView targetRTV,
								RGHandle depthTexture,
								RHIDepthStencilView depthDSV,
								float fViewportWidth, float fViewportHeight)
{
	m_vDrawItems.clear();
	CollectDrawItems(pScene, defaultPipeline);
	AddOpaquePass(pRender, viewProj, targetTexture, targetRTV,
				  depthTexture, depthDSV, fViewportWidth, fViewportHeight);
}

void SceneRenderer::CollectDrawItems(Scene* pScene, RHIPipelineHandle defaultPipeline)
{
	// Iterate entities that have both Transform and Mesh components
	pScene->Meshes().ForEach([&](EntityHandle entity, MeshComponent& mesh) {
		if (!mesh.bVisible) return;
		auto* pTransform = pScene->Transforms().Get(entity);
		if (!pTransform || !mesh.pMesh)
			return;

		const MeshLOD& lod = mesh.pMesh->GetLOD(mesh.uLODIndex);
		Mat4 worldMatrix = pTransform->GetWorldMatrix();

		// Read material if present
		Vec3  albedo    = Vec3(0.8f, 0.8f, 0.8f);
		float roughness = 0.5f;
		float metallic  = 0.0f;
		float alpha     = 1.0f;
		auto* pMaterial = pScene->Materials().Get(entity);
		if (pMaterial)
		{
			albedo    = pMaterial->vAlbedoColor;
			roughness = pMaterial->fRoughness;
			metallic  = pMaterial->fMetallic;
			alpha     = pMaterial->fAlpha;
		}

		bool bTransparent = alpha < 1.0f;

		for (const auto& subMesh : lod.vSubMeshes)
		{
			DrawItem item;
			item.pipeline      = defaultPipeline;
			item.vertexBuffer  = lod.vertexBuffer;
			item.indexBuffer   = lod.indexBuffer;
			item.uVertexStride = lod.uVertexStride;
			item.uIndexOffset  = subMesh.uIndexOffset;
			item.uIndexCount   = subMesh.uIndexCount;
			item.uVertexOffset = subMesh.uVertexOffset;
			item.worldMatrix   = worldMatrix;
			item.vAlbedoColor  = albedo;
			item.fRoughness    = roughness;
			item.fMetallic     = metallic;
			item.fAlpha        = alpha;
			item.bTransparent  = bTransparent;
			m_vDrawItems.push_back(item);
		}
	});
}

void SceneRenderer::AddOpaquePass(Render* pRender, const Mat4& viewProj,
								  RGHandle targetTexture, RHIRenderTargetView targetRTV,
								  RGHandle depthTexture, RHIDepthStencilView depthDSV,
								  float fViewportWidth, float fViewportHeight)
{
	auto& graph = pRender->GetRenderGraph();

	float w = fViewportWidth;
	float h = fViewportHeight;
	RHIColor clearColor = { 0.2f, 0.3f, 0.4f, 1.0f };

	// Capture draw items + viewProj by value for the execute lambda
	auto drawItems = m_vDrawItems;
	Mat4 vp = viewProj;

	graph.AddPass("OpaquePass",
		[targetTexture, targetRTV, clearColor, depthTexture, depthDSV](RGPassBuilder& builder) {
			builder.WriteRenderTarget(targetTexture, targetRTV, &clearColor);
			if (depthTexture.IsValid())
				builder.WriteDepthStencil(depthTexture, depthDSV);
		},
		[drawItems, vp, w, h](RHICommandList* pCmdList) {
			RHIViewport viewport = { 0, 0, w, h, 0, 1 };
			pCmdList->SetViewport(viewport);
			RHIScissorRect scissor = { 0, 0, static_cast<int32>(w), static_cast<int32>(h) };
			pCmdList->SetScissorRect(scissor);

			for (const auto& item : drawItems)
			{
				pCmdList->SetPipeline(item.pipeline);

				// Per-object MVP via push constants
				Mat4 mvp = item.worldMatrix * vp;
				pCmdList->SetPushConstants(&mvp, sizeof(Mat4));

				RHIVertexBufferView vbView = {};
				vbView.buffer  = item.vertexBuffer;
				vbView.uStride = item.uVertexStride;

				RHIIndexBufferView ibView = {};
				ibView.buffer = item.indexBuffer;
				ibView.format = RHIIndexFormat::U32;

				pCmdList->SetVertexBuffer(0, vbView);
				pCmdList->SetIndexBuffer(ibView);

				pCmdList->DrawIndexed(item.uIndexCount, 1,
									  item.uIndexOffset, item.uVertexOffset, 0);
			}
		});
}

void SceneRenderer::RenderGBuffer(Scene* pScene, Render* pRender,
								  RHIPipelineHandle gbufferPipeline,
								  const Mat4& viewProj,
								  const GBufferTargets& targets,
								  float fViewportWidth, float fViewportHeight)
{
	m_vDrawItems.clear();
	CollectDrawItems(pScene, gbufferPipeline);
	AddGBufferPass(pRender, viewProj, targets, fViewportWidth, fViewportHeight);
}

void SceneRenderer::AddGBufferPass(Render* pRender, const Mat4& viewProj,
								   const GBufferTargets& targets,
								   float fViewportWidth, float fViewportHeight)
{
	auto& graph = pRender->GetRenderGraph();

	float w = fViewportWidth;
	float h = fViewportHeight;
	RHIColor clearAlbedo   = { 0.0f, 0.0f, 0.0f, 0.0f };
	RHIColor clearNormal   = { 0.5f, 0.5f, 1.0f, 0.0f };
	RHIColor clearRoughMet = { 0.0f, 0.0f, 0.0f, 0.0f };

	auto drawItems = m_vDrawItems;
	// Filter to opaque-only for G-Buffer
	drawItems.erase(std::remove_if(drawItems.begin(), drawItems.end(),
		[](const DrawItem& item) { return item.bTransparent; }), drawItems.end());
	Mat4 vp = viewProj;
	GBufferTargets t = targets;

	graph.AddPass("GBufferPass",
		[t, clearAlbedo, clearNormal, clearRoughMet](RGPassBuilder& builder) {
			builder.WriteRenderTarget(t.albedoTexture, t.albedoRTV, &clearAlbedo);
			builder.WriteRenderTarget(t.normalTexture, t.normalRTV, &clearNormal);
			builder.WriteRenderTarget(t.roughMetTexture, t.roughMetRTV, &clearRoughMet);
			if (t.depthTexture.IsValid())
				builder.WriteDepthStencil(t.depthTexture, t.depthDSV);
		},
		[drawItems, vp, w, h](RHICommandList* pCmdList) {
			RHIViewport viewport = { 0, 0, w, h, 0, 1 };
			pCmdList->SetViewport(viewport);
			RHIScissorRect scissor = { 0, 0, static_cast<int32>(w), static_cast<int32>(h) };
			pCmdList->SetScissorRect(scissor);

			for (const auto& item : drawItems)
			{
				pCmdList->SetPipeline(item.pipeline);

				GBufferPushConstants pc;
				pc.mvp          = item.worldMatrix * vp;
				pc.vAlbedoColor = item.vAlbedoColor;
				pc.fRoughness   = item.fRoughness;
				pc.fMetallic    = item.fMetallic;
				pc._pad         = Vec3::Zero;
				pCmdList->SetPushConstants(&pc, sizeof(GBufferPushConstants));

				RHIVertexBufferView vbView = {};
				vbView.buffer  = item.vertexBuffer;
				vbView.uStride = item.uVertexStride;

				RHIIndexBufferView ibView = {};
				ibView.buffer = item.indexBuffer;
				ibView.format = RHIIndexFormat::U32;

				pCmdList->SetVertexBuffer(0, vbView);
				pCmdList->SetIndexBuffer(ibView);

				pCmdList->DrawIndexed(item.uIndexCount, 1,
									  item.uIndexOffset, item.uVertexOffset, 0);
			}
		});
}

void SceneRenderer::RenderShadowMap(Scene* pScene, Render* pRender,
									RHIPipelineHandle shadowPipeline,
									const Mat4& lightViewProj,
									RGHandle shadowTexture, RHIDepthStencilView shadowDSV,
									float fShadowMapSize)
{
	m_vDrawItems.clear();
	CollectDrawItems(pScene, shadowPipeline);
	AddShadowPass(pRender, shadowPipeline, lightViewProj, shadowTexture, shadowDSV, fShadowMapSize);
}

void SceneRenderer::AddShadowPass(Render* pRender, RHIPipelineHandle shadowPipeline,
								  const Mat4& lightViewProj,
								  RGHandle shadowTexture, RHIDepthStencilView shadowDSV,
								  float fShadowMapSize)
{
	auto& graph = pRender->GetRenderGraph();

	auto drawItems = m_vDrawItems;
	// Filter to opaque-only for shadow casting
	drawItems.erase(std::remove_if(drawItems.begin(), drawItems.end(),
		[](const DrawItem& item) { return item.bTransparent; }), drawItems.end());
	Mat4 lvp = lightViewProj;
	float sz = fShadowMapSize;
	RHIPipelineHandle pipeline = shadowPipeline;

	graph.AddPass("ShadowPass",
		[shadowTexture, shadowDSV](RGPassBuilder& builder) {
			builder.WriteDepthStencil(shadowTexture, shadowDSV);
		},
		[drawItems, lvp, sz, pipeline](RHICommandList* pCmdList) {
			RHIViewport viewport = { 0, 0, sz, sz, 0, 1 };
			pCmdList->SetViewport(viewport);
			RHIScissorRect scissor = { 0, 0, static_cast<int32>(sz), static_cast<int32>(sz) };
			pCmdList->SetScissorRect(scissor);

			for (const auto& item : drawItems)
			{
				pCmdList->SetPipeline(pipeline);

				Mat4 lightMVP = item.worldMatrix * lvp;
				pCmdList->SetPushConstants(&lightMVP, sizeof(Mat4));

				RHIVertexBufferView vbView = {};
				vbView.buffer  = item.vertexBuffer;
				vbView.uStride = item.uVertexStride;

				RHIIndexBufferView ibView = {};
				ibView.buffer = item.indexBuffer;
				ibView.format = RHIIndexFormat::U32;

				pCmdList->SetVertexBuffer(0, vbView);
				pCmdList->SetIndexBuffer(ibView);

				pCmdList->DrawIndexed(item.uIndexCount, 1,
									  item.uIndexOffset, item.uVertexOffset, 0);
			}
		});
}

void SceneRenderer::AddLightingPass(Render* pRender,
									RHIPipelineHandle lightingPipeline,
									RHIDescriptorSetHandle lightingDescSet,
									const GBufferTargets& gbTargets,
									RGHandle shadowTexture,
									RGHandle targetTexture, RHIRenderTargetView targetRTV,
									const LightingPushConstants& lightPC,
									float fViewportWidth, float fViewportHeight)
{
	auto& graph = pRender->GetRenderGraph();

	float w = fViewportWidth;
	float h = fViewportHeight;
	RHIColor clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	RHIPipelineHandle pipeline = lightingPipeline;
	RHIDescriptorSetHandle descSet = lightingDescSet;
	GBufferTargets t = gbTargets;
	LightingPushConstants pc = lightPC;

	graph.AddPass("LightingPass",
		[targetTexture, targetRTV, clearColor, t, shadowTexture](RGPassBuilder& builder) {
			builder.WriteRenderTarget(targetTexture, targetRTV, &clearColor);
			builder.ReadTexture(t.albedoTexture);
			builder.ReadTexture(t.normalTexture);
			builder.ReadTexture(t.roughMetTexture);
			builder.ReadTexture(t.depthTexture);
			builder.ReadTexture(shadowTexture);
		},
		[pipeline, descSet, pc, w, h](RHICommandList* pCmdList) {
			RHIViewport viewport = { 0, 0, w, h, 0, 1 };
			pCmdList->SetViewport(viewport);
			RHIScissorRect scissor = { 0, 0, static_cast<int32>(w), static_cast<int32>(h) };
			pCmdList->SetScissorRect(scissor);

			pCmdList->SetPipeline(pipeline);
			pCmdList->SetDescriptorSet(0, descSet);
			pCmdList->SetPushConstants(&pc, sizeof(LightingPushConstants));
			pCmdList->Draw(3, 1, 0, 0);
		});
}

void SceneRenderer::AddPostProcessPass(Render* pRender,
									   RHIPipelineHandle postPipeline,
									   RHIDescriptorSetHandle postDescSet,
									   RGHandle hdrTexture,
									   RGHandle targetTexture, RHIRenderTargetView targetRTV,
									   float fViewportWidth, float fViewportHeight)
{
	auto& graph = pRender->GetRenderGraph();

	float w = fViewportWidth;
	float h = fViewportHeight;
	RHIPipelineHandle pipeline = postPipeline;
	RHIDescriptorSetHandle descSet = postDescSet;

	RHIColor clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	graph.AddPass("PostProcessPass",
		[targetTexture, targetRTV, hdrTexture, clearColor](RGPassBuilder& builder) {
			builder.WriteRenderTarget(targetTexture, targetRTV, &clearColor);
			builder.ReadTexture(hdrTexture);
		},
		[pipeline, descSet, w, h](RHICommandList* pCmdList) {
			RHIViewport viewport = { 0, 0, w, h, 0, 1 };
			pCmdList->SetViewport(viewport);
			RHIScissorRect scissor = { 0, 0, static_cast<int32>(w), static_cast<int32>(h) };
			pCmdList->SetScissorRect(scissor);

			pCmdList->SetPipeline(pipeline);
			pCmdList->SetDescriptorSet(0, descSet);
			pCmdList->Draw(3, 1, 0, 0);
		});
}

void SceneRenderer::RenderForwardTransparent(Scene* pScene, Render* pRender,
											 RHIPipelineHandle transparentPipeline,
											 RHIDescriptorSetHandle shadowDescSet,
											 const Mat4& viewProj,
											 const TransparentPushConstants& basePc,
											 RGHandle targetTexture, RHIRenderTargetView targetRTV,
											 RGHandle depthTexture, RHIDepthStencilView depthDSV,
											 RGHandle shadowTexture,
											 float fViewportWidth, float fViewportHeight)
{
	m_vDrawItems.clear();
	CollectDrawItems(pScene, transparentPipeline);

	// Filter to transparent-only
	m_vDrawItems.erase(std::remove_if(m_vDrawItems.begin(), m_vDrawItems.end(),
		[](const DrawItem& item) { return !item.bTransparent; }), m_vDrawItems.end());

	if (m_vDrawItems.empty())
		return;

	AddForwardTransparentPass(pRender, transparentPipeline, shadowDescSet, basePc, viewProj,
							  targetTexture, targetRTV, depthTexture, depthDSV, shadowTexture,
							  fViewportWidth, fViewportHeight);
}

void SceneRenderer::AddForwardTransparentPass(Render* pRender,
											  RHIPipelineHandle transparentPipeline,
											  RHIDescriptorSetHandle shadowDescSet,
											  const TransparentPushConstants& basePc,
											  const Mat4& viewProj,
											  RGHandle targetTexture, RHIRenderTargetView targetRTV,
											  RGHandle depthTexture, RHIDepthStencilView depthDSV,
											  RGHandle shadowTexture,
											  float fViewportWidth, float fViewportHeight)
{
	auto& graph = pRender->GetRenderGraph();

	float w = fViewportWidth;
	float h = fViewportHeight;
	auto drawItems = m_vDrawItems;
	Mat4 vp = viewProj;
	RHIPipelineHandle pipeline = transparentPipeline;
	RHIDescriptorSetHandle descSet = shadowDescSet;
	TransparentPushConstants pc = basePc;

	graph.AddPass("ForwardTransparentPass",
		[targetTexture, targetRTV, depthTexture, depthDSV, shadowTexture](RGPassBuilder& builder) {
			builder.WriteRenderTarget(targetTexture, targetRTV);
			builder.ReadDepthStencil(depthTexture, depthDSV);
			builder.ReadTexture(shadowTexture);
		},
		[drawItems, vp, w, h, pipeline, descSet, pc](RHICommandList* pCmdList) {
			RHIViewport viewport = { 0, 0, w, h, 0, 1 };
			pCmdList->SetViewport(viewport);
			RHIScissorRect scissor = { 0, 0, static_cast<int32>(w), static_cast<int32>(h) };
			pCmdList->SetScissorRect(scissor);

			for (const auto& item : drawItems)
			{
				pCmdList->SetPipeline(pipeline);
				pCmdList->SetDescriptorSet(0, descSet);

				TransparentPushConstants itemPc = pc;
				itemPc.mvp = item.worldMatrix * vp;
				itemPc.vAlbedoColor[0] = item.vAlbedoColor.x;
				itemPc.vAlbedoColor[1] = item.vAlbedoColor.y;
				itemPc.vAlbedoColor[2] = item.vAlbedoColor.z;
				itemPc.fRoughness      = item.fRoughness;
				itemPc.fMetallic       = item.fMetallic;
				itemPc.fAlpha          = item.fAlpha;
				pCmdList->SetPushConstants(&itemPc, sizeof(TransparentPushConstants));

				RHIVertexBufferView vbView = {};
				vbView.buffer  = item.vertexBuffer;
				vbView.uStride = item.uVertexStride;

				RHIIndexBufferView ibView = {};
				ibView.buffer = item.indexBuffer;
				ibView.format = RHIIndexFormat::U32;

				pCmdList->SetVertexBuffer(0, vbView);
				pCmdList->SetIndexBuffer(ibView);

				pCmdList->DrawIndexed(item.uIndexCount, 1,
									  item.uIndexOffset, item.uVertexOffset, 0);
			}
		});
}

} // namespace Evo

