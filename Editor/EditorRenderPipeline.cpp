#include "EditorRenderPipeline.h"
#include "Asset/ShaderAsset.h"
#include "Core/Log.h"

namespace Evo {

bool EditorRenderPipeline::Initialize(RHIDevice* pDevice, RHIFormat rtFormat)
{
	if (!RenderPipeline::Initialize(pDevice, rtFormat))
		return false;

	// Initialize debug renderer with its own asset manager
	m_DebugAssetManager.Initialize(pDevice);
	m_DebugAssetManager.RegisterFactory(".hlsl", [] { return std::make_unique<ShaderAsset>(); });

	if (!m_DebugRenderer.Initialize(pDevice, m_DebugAssetManager, rtFormat))
	{
		EVO_LOG_ERROR("EditorRenderPipeline: failed to initialize debug renderer");
		return false;
	}

	EVO_LOG_INFO("EditorRenderPipeline initialized");
	return true;
}

void EditorRenderPipeline::Shutdown(RHIDevice* pDevice)
{
	m_DebugRenderer.Shutdown(pDevice);
	m_DebugAssetManager.Shutdown();

	RenderPipeline::Shutdown(pDevice);
}

void EditorRenderPipeline::RenderDebugOverlay(Renderer& renderer,
                                              RGHandle target, RHIRenderTargetView rtv,
                                              const Mat4& viewProj,
                                              float fWidth, float fHeight)
{
	m_DebugRenderer.Render(renderer, target, rtv, viewProj, fWidth, fHeight);
}

} // namespace Evo
