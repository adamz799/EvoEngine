#include "EditorRenderPipeline.h"
#include "Renderer/Renderer.h"
#include "Asset/ShaderAsset.h"
#include "Core/Log.h"

namespace Evo {

bool EditorRenderPipeline::Initialize(Render* pRender)
{
	if (!RenderPipeline::Initialize(pRender))
		return false;

	// Initialize debug renderer with its own asset manager
	auto* pDevice = pRender->GetDevice();
	m_DebugAssetManager.Initialize(pDevice);
	m_DebugAssetManager.RegisterFactory(".hlsl", [] { return std::make_unique<ShaderAsset>(); });

	if (!m_DebugRenderer.Initialize(pRender, m_DebugAssetManager))
	{
		EVO_LOG_ERROR("EditorRenderPipeline: failed to initialize debug renderer");
		return false;
	}

	EVO_LOG_INFO("EditorRenderPipeline initialized");
	return true;
}

void EditorRenderPipeline::Shutdown()
{
	m_DebugRenderer.Shutdown(m_pRender);
	m_DebugAssetManager.Shutdown();
	RenderPipeline::Shutdown();
}

void EditorRenderPipeline::RenderDebugOverlay(Render* pRender,
											  RGHandle target, RHIRenderTargetView rtv,
											  const Mat4& viewProj,
											  float fWidth, float fHeight)
{
	m_DebugRenderer.RenderLines(pRender, target, rtv, viewProj, fWidth, fHeight);
}

} // namespace Evo

