#pragma once

#include "RHI/RHI.h"
#include "Asset/AssetManager.h"
#include "Renderer/RenderGraph.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/ViewportFrame.h"
#include "Scene/Scene.h"
#include "Math/Math.h"

namespace Evo {

class Renderer;

/// Base rendering pipeline: owns shared PSOs, shadow map, and light data.
/// Provides the 5-pass deferred rendering sequence per viewport.
class RenderPipeline {
public:
	RenderPipeline() = default;
	virtual ~RenderPipeline();

	bool Initialize(RHIDevice* pDevice, RHIFormat rtFormat);
	virtual void Shutdown(RHIDevice* pDevice);

	/// Build a ViewportFrameDesc with proper layout handles and shadow texture.
	ViewportFrameDesc MakeViewportFrameDesc(
		uint32 uWidth, uint32 uHeight, const std::string& sName) const;

	/// Import shadow map and render shadow pass. Call once per frame.
	void RenderShadow(Renderer& renderer, Scene& scene);

	/// Full per-viewport pipeline: GBuffer -> Lighting -> Transparent -> PostProcess.
	virtual void RenderViewport(Renderer& renderer, Scene& scene,
								ViewportFrame& viewport, const Mat4& viewProj,
								RGHandle outputTarget, RHIRenderTargetView outputRTV);

	// Accessors
	RHITextureHandle             GetShadowTexture()             const { return m_ShadowTexture; }
	RHIDescriptorSetLayoutHandle GetLightingSetLayout()         const { return m_LightingSetLayout; }
	RHIDescriptorSetLayoutHandle GetPostProcessSetLayout()      const { return m_PostProcessSetLayout; }
	RHIDescriptorSetLayoutHandle GetTransparentShadowSetLayout() const { return m_TransShadowSetLayout; }
	const Mat4&                  GetLightViewProj()             const { return m_LightViewProj; }

protected:
	// Shadow map
	static constexpr uint32 kShadowMapSize = 2048;
	RHITextureHandle    m_ShadowTexture;
	RHIDepthStencilView m_ShadowDSV;

	// Pipelines (PSOs)
	RHIPipelineHandle m_GBufferPipeline;
	RHIPipelineHandle m_ShadowPipeline;
	RHIPipelineHandle m_LightingPipeline;
	RHIPipelineHandle m_PostProcessPipeline;
	RHIPipelineHandle m_TransparentPipeline;

	// Descriptor set layouts
	RHIDescriptorSetLayoutHandle m_LightingSetLayout;
	RHIDescriptorSetLayoutHandle m_PostProcessSetLayout;
	RHIDescriptorSetLayoutHandle m_TransShadowSetLayout;

	// Light data
	Mat4 m_LightViewProj;
	Vec3 m_vLightDir;
	Vec3 m_vLightColor;

	// Utility
	SceneRenderer m_SceneRenderer;
	AssetManager  m_AssetManager;   // owns shader assets

	// Per-frame shadow RG handle (valid between RenderShadow and EndFrame)
	RGHandle m_ShadowRG;
};

} // namespace Evo

