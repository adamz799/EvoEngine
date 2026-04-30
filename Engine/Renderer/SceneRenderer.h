#pragma once

#include "RHI/RHITypes.h"
#include "Renderer/RenderGraph.h"
#include "Math/Mat4.h"
#include <vector>

namespace Evo {

class Scene;
class Render;

/// Per-draw-call data collected from the scene.
struct DrawItem {
	RHIPipelineHandle pipeline;
	RHIBufferHandle   vertexBuffer;
	RHIBufferHandle   indexBuffer;
	uint32            uVertexStride = 0;
	uint32            uIndexOffset  = 0;
	uint32            uIndexCount   = 0;
	uint32            uVertexOffset = 0;
	Mat4              worldMatrix;
	Vec3              vAlbedoColor = Vec3(0.8f, 0.8f, 0.8f);
	float             fRoughness   = 0.5f;
	float             fMetallic    = 0.0f;
	float             fAlpha       = 1.0f;
	bool              bTransparent = false;
};

/// G-Buffer render targets bundle.
struct GBufferTargets {
	RGHandle            albedoTexture;
	RGHandle            normalTexture;
	RGHandle            roughMetTexture;
	RGHandle            depthTexture;
	RHIRenderTargetView albedoRTV;
	RHIRenderTargetView normalRTV;
	RHIRenderTargetView roughMetRTV;
	RHIDepthStencilView depthDSV;
};

/// Push constants for deferred lighting pass.
struct LightingPushConstants {
	Mat4  invViewProj;
	Mat4  lightViewProj;
	float vLightDir[3];
	float fShadowMapSize;
	float vLightColor[3];
	float _pad;
};

/// Push constants for forward transparent pass.
struct TransparentPushConstants {
	Mat4  mvp;
	Mat4  lightViewProj;
	float vAlbedoColor[3];
	float fRoughness;
	float fMetallic;
	float fAlpha;
	float vLightDir[3];
	float fShadowMapSize;
	float vLightColor[3];
	float _pad;
};

/// SceneRenderer — bridges Scene data and RenderGraph.
/// Collects renderable entities, builds draw items, adds passes.
class SceneRenderer {
public:
	void RenderScene(Scene& scene, Render* pRender,
					 RHIPipelineHandle defaultPipeline,
					 const Mat4& viewProj,
					 RGHandle targetTexture,
					 RHIRenderTargetView targetRTV,
					 RGHandle depthTexture,
					 RHIDepthStencilView depthDSV,
					 float fViewportWidth, float fViewportHeight);

	void RenderGBuffer(Scene& scene, Render* pRender,
					   RHIPipelineHandle gbufferPipeline,
					   const Mat4& viewProj,
					   const GBufferTargets& targets,
					   float fViewportWidth, float fViewportHeight);

	void RenderShadowMap(Scene& scene, Render* pRender,
						 RHIPipelineHandle shadowPipeline,
						 const Mat4& lightViewProj,
						 RGHandle shadowTexture, RHIDepthStencilView shadowDSV,
						 float fShadowMapSize);

	void AddLightingPass(Render* pRender,
						 RHIPipelineHandle lightingPipeline,
						 RHIDescriptorSetHandle lightingDescSet,
						 const GBufferTargets& gbTargets,
						 RGHandle shadowTexture,
						 RGHandle targetTexture, RHIRenderTargetView targetRTV,
						 const LightingPushConstants& lightPC,
						 float fViewportWidth, float fViewportHeight);

	void AddPostProcessPass(Render* pRender,
							RHIPipelineHandle postPipeline,
							RHIDescriptorSetHandle postDescSet,
							RGHandle hdrTexture,
							RGHandle targetTexture, RHIRenderTargetView targetRTV,
							float fViewportWidth, float fViewportHeight);

	void RenderForwardTransparent(Scene& scene, Render* pRender,
								  RHIPipelineHandle transparentPipeline,
								  RHIDescriptorSetHandle shadowDescSet,
								  const Mat4& viewProj,
								  const TransparentPushConstants& basePc,
								  RGHandle targetTexture, RHIRenderTargetView targetRTV,
								  RGHandle depthTexture, RHIDepthStencilView depthDSV,
								  RGHandle shadowTexture,
								  float fViewportWidth, float fViewportHeight);

private:
	std::vector<DrawItem> m_vDrawItems;

	void CollectDrawItems(Scene& scene, RHIPipelineHandle defaultPipeline);
	void AddOpaquePass(Render* pRender, const Mat4& viewProj,
					   RGHandle targetTexture, RHIRenderTargetView targetRTV,
					   RGHandle depthTexture, RHIDepthStencilView depthDSV,
					   float fViewportWidth, float fViewportHeight);
	void AddGBufferPass(Render* pRender, const Mat4& viewProj,
						const GBufferTargets& targets,
						float fViewportWidth, float fViewportHeight);
	void AddShadowPass(Render* pRender, RHIPipelineHandle shadowPipeline,
					   const Mat4& lightViewProj,
					   RGHandle shadowTexture, RHIDepthStencilView shadowDSV,
					   float fShadowMapSize);
	void AddForwardTransparentPass(Render* pRender,
								   RHIPipelineHandle transparentPipeline,
								   RHIDescriptorSetHandle shadowDescSet,
								   const TransparentPushConstants& basePc,
								   const Mat4& viewProj,
								   RGHandle targetTexture, RHIRenderTargetView targetRTV,
								   RGHandle depthTexture, RHIDepthStencilView depthDSV,
								   RGHandle shadowTexture,
								   float fViewportWidth, float fViewportHeight);
};

} // namespace Evo

