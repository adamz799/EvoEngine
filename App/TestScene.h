#pragma once

#include "Asset/AssetManager.h"
#include "Scene/Scene.h"
#include "Scene/MeshAsset.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/Camera.h"

namespace Evo {

class Renderer;

/// TestScene -- five rotating cubes, with a fixed game camera.
class TestScene {
public:
	bool Initialize(RHIDevice* pDevice, RHIFormat rtFormat);
	void Shutdown(RHIDevice* pDevice);

	/// Update cube rotations (no input, no camera controller).
	void Update(float fDeltaTime);

	/// Render the scene from an externally-supplied view-projection matrix.
	void Render(Renderer& renderer,
	            RGHandle targetTexture, RHIRenderTargetView targetRTV,
	            RGHandle depthTexture, RHIDepthStencilView depthDSV,
	            const Mat4& viewProj,
	            float fViewportWidth, float fViewportHeight);

	/// Render shadow map from light's perspective.
	void RenderShadowMap(Renderer& renderer,
	                     RGHandle shadowTexture, RHIDepthStencilView shadowDSV,
	                     float fShadowMapSize);

	/// Render scene geometry into G-Buffer (MRT).
	void RenderGBuffer(Renderer& renderer,
	                   const GBufferTargets& targets,
	                   const Mat4& viewProj,
	                   float fViewportWidth, float fViewportHeight);

	/// Deferred lighting pass: reads G-Buffer + shadow map, writes lit output.
	void RenderLighting(Renderer& renderer,
	                    const GBufferTargets& gbTargets,
	                    RHIDescriptorSetHandle lightingDescSet,
	                    RGHandle shadowTexture,
	                    RGHandle targetTexture, RHIRenderTargetView targetRTV,
	                    const Mat4& viewProj,
	                    float fViewportWidth, float fViewportHeight);

	/// Post-processing pass: tone mapping + gamma.
	void RenderPostProcess(Renderer& renderer,
	                       RHIDescriptorSetHandle postDescSet,
	                       RGHandle hdrTexture,
	                       RGHandle targetTexture, RHIRenderTargetView targetRTV,
	                       float fViewportWidth, float fViewportHeight);

	/// Forward transparent pass: renders transparent objects with forward lighting.
	void RenderForwardTransparent(Renderer& renderer,
	                              RHIDescriptorSetHandle shadowDescSet,
	                              const Mat4& viewProj,
	                              RGHandle targetTexture, RHIRenderTargetView targetRTV,
	                              RGHandle depthTexture, RHIDepthStencilView depthDSV,
	                              RGHandle shadowTexture,
	                              float fViewportWidth, float fViewportHeight);

	Scene& GetScene() { return m_Scene; }
	Camera& GetGameCamera() { return m_GameCamera; }
	const Camera& GetGameCamera() const { return m_GameCamera; }

	RHIDescriptorSetLayoutHandle GetLightingSetLayout() const { return m_LightingSetLayout; }
	RHIDescriptorSetLayoutHandle GetPostProcessSetLayout() const { return m_PostProcessSetLayout; }
	RHIDescriptorSetLayoutHandle GetTransparentShadowSetLayout() const { return m_TransparentShadowSetLayout; }
	const Mat4& GetLightViewProj() const { return m_LightViewProj; }

private:
	// Asset management
	AssetManager  m_AssetManager;
	AssetHandle   m_ShaderHandle;

	// GPU resources
	RHIPipelineHandle m_Pipeline;
	RHIPipelineHandle m_GBufferPipeline;
	AssetHandle       m_GBufferShaderHandle;

	// Shadow mapping
	RHIPipelineHandle m_ShadowPipeline;
	AssetHandle       m_ShadowShaderHandle;
	Mat4              m_LightViewProj;

	// Deferred lighting
	RHIPipelineHandle            m_LightingPipeline;
	AssetHandle                  m_LightingShaderHandle;
	RHIDescriptorSetLayoutHandle m_LightingSetLayout;

	// Post-processing
	RHIPipelineHandle            m_PostProcessPipeline;
	AssetHandle                  m_PostProcessShaderHandle;
	RHIDescriptorSetLayoutHandle m_PostProcessSetLayout;

	// Forward transparent
	RHIPipelineHandle            m_TransparentPipeline;
	AssetHandle                  m_TransparentShaderHandle;
	RHIDescriptorSetLayoutHandle m_TransparentShadowSetLayout;

	// Scene
	Scene         m_Scene;
	SceneRenderer m_SceneRenderer;

	// Game camera (fixed viewpoint, no controller)
	Camera m_GameCamera;

	float m_fTime = 0.0f;
};

} // namespace Evo
