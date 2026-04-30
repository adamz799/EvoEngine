#pragma once

#include "RHI/RHI.h"
#include "Asset/AssetManager.h"
#include "Renderer/RenderGraph.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/ViewportFrame.h"
#include "Scene/Scene.h"
#include "Math/Math.h"
#include <vector>

namespace Evo {

class Render;

/// Rendering pipeline — owned by Engine. Holds its own Renderer* and Scene*.
/// No per-frame parameter passing needed. App never sees this class.
class RenderPipeline {
public:
	RenderPipeline() = default;
	virtual ~RenderPipeline();

	bool Initialize(Render* pRender);
	virtual void Shutdown();

	/// Bind the current scene. Called by Engine::LoadScene.
	void SetScene(Scene* pScene) { m_pScene = pScene; }

	/// Single frame entry — called by Engine::EndFrame.
	/// Reads scene cameras, renders shadow + all viewports.
	virtual void RenderFrame();

	// ---- Viewport query (Editor needs output textures for ImGui) ----

	ViewportFrame* GetViewport(size_t i);
	size_t         GetViewportCount() const { return m_vViewports.size(); }
	ViewportFrameDesc MakeViewportFrameDesc(
		uint32 uWidth, uint32 uHeight, const std::string& sName) const;

	RHITextureHandle GetShadowTexture() const { return m_ShadowTexture; }

	// === Per-pass methods (Editor uses these for custom render flow) ===

	virtual void RenderShadow();

	void RenderViewport(ViewportFrame& viewport, const Mat4& viewProj);

	virtual void RenderViewport(ViewportFrame& viewport, const Mat4& viewProj,
								RGHandle outputTarget, RHIRenderTargetView outputRTV);

protected:
	// Scene renderer access (EditorRenderPipeline uses for debug meshes)
	SceneRenderer& GetSceneRenderer() { return m_SceneRenderer; }

	RHIDescriptorSetLayoutHandle GetLightingSetLayout()         const { return m_LightingSetLayout; }
	RHIDescriptorSetLayoutHandle GetPostProcessSetLayout()      const { return m_PostProcessSetLayout; }
	RHIDescriptorSetLayoutHandle GetTransparentShadowSetLayout() const { return m_TransShadowSetLayout; }
	const Mat4& GetLightViewProj() const { return m_LightViewProj; }

	// Pointers (set during Initialize / SetScene)
	Render* m_pRender = nullptr;
	Scene*    m_pScene    = nullptr;

	// Shadow
	static constexpr uint32 kShadowMapSize = 2048;
	RHITextureHandle    m_ShadowTexture;
	RHIDepthStencilView m_ShadowDSV;

	// PSOs
	RHIPipelineHandle m_GBufferPipeline;
	RHIPipelineHandle m_ShadowPipeline;
	RHIPipelineHandle m_LightingPipeline;
	RHIPipelineHandle m_PostProcessPipeline;
	RHIPipelineHandle m_TransparentPipeline;

	// Descriptor layouts
	RHIDescriptorSetLayoutHandle m_LightingSetLayout;
	RHIDescriptorSetLayoutHandle m_PostProcessSetLayout;
	RHIDescriptorSetLayoutHandle m_TransShadowSetLayout;

	// Light data
	Mat4 m_LightViewProj;
	Vec3 m_vLightDir;
	Vec3 m_vLightColor;

	// Utility
	SceneRenderer m_SceneRenderer;
	AssetManager  m_AssetManager;
	RGHandle      m_ShadowRG;

private:
	void SetupViewports();
	std::vector<ViewportFrame> m_vViewports;
};

} // namespace Evo
