#pragma once

#include "Renderer/RenderPipeline.h"
#include "DebugRenderer.h"

namespace Evo {

/// Editor rendering pipeline: extends RenderPipeline with debug overlay (frustum, camera icon).
class EditorRenderPipeline : public RenderPipeline {
public:
	bool Initialize(RHIDevice* pDevice, RHIFormat rtFormat);
	void Shutdown(RHIDevice* pDevice) override;

	/// Draw debug overlays (frustum, camera icon) onto an already-rendered viewport.
	void RenderDebugOverlay(Renderer& renderer,
	                        RGHandle target, RHIRenderTargetView rtv,
	                        const Mat4& viewProj,
	                        float fWidth, float fHeight);

	DebugRenderer& GetDebugRenderer() { return m_DebugRenderer; }

private:
	DebugRenderer m_DebugRenderer;
	AssetManager  m_DebugAssetManager;
};

} // namespace Evo
