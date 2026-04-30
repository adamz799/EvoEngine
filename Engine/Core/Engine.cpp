#include "Core/Engine.h"
#include "Core/EngineConfig.h"
#include "Core/Log.h"
#include "Platform/Window.h"
#include "Platform/Input.h"
#include "Renderer/Renderer.h"

namespace Evo {

Engine::~Engine() {
	Shutdown();
}

bool Engine::Launch() {
	EVO_LOG_INFO("EvoEngine launching...");

	auto& cfg = EngineConfig::GetInstance();

	// ---- 2. Input ----
	m_pInput = std::make_unique<Input>();

	// ---- 3. Window ----
	m_pWindow = std::make_unique<Window>();
	WindowDesc winDesc{};
	winDesc.sTitle  = cfg.window.sTitle;
	winDesc.uWidth  = cfg.window.uWidth;
	winDesc.uHeight = cfg.window.uHeight;

	if (!m_pWindow->Initialize(winDesc)) {
		EVO_LOG_CRITICAL("Failed to create window");
		Log::Shutdown();
		return false;
	}

	// ---- 4. Renderer (RHI device + swap chain) ----
	m_pRender = std::make_unique<Render>();
	RendererDesc renderDesc{};
	renderDesc.backend                  = cfg.ResolveBackend();
	renderDesc.bEnableDebugLayer         = cfg.rhi.bEnableDebugLayer;
	renderDesc.bEnableGPUBasedValidation = cfg.rhi.bEnableGPUBasedValidation;

	if (!m_pRender->Initialize(renderDesc, *m_pWindow)) {
		EVO_LOG_ERROR("Failed to initialize renderer");
		m_pWindow->Shutdown();
		Log::Shutdown();
		return false;
	}

	// ---- 5. Pipeline ----
	m_pRenderPipeline = std::make_unique<RenderPipeline>();
	if (!m_pRenderPipeline->Initialize(m_pRender.get())) {
		EVO_LOG_ERROR("Failed to initialize render pipeline");
		m_pRender->Shutdown();
		m_pWindow->Shutdown();
		Log::Shutdown();
		return false;
	}

	// ---- 6. Timing ----
	m_LastTime = Clock::now();

	EVO_LOG_INFO("Engine launched (backend: {})", cfg.rhi.backend);
	return true;
}

void Engine::Shutdown() {
	EVO_LOG_INFO("Shutting down...");

	if (m_pRenderPipeline) {
		m_pRenderPipeline->Shutdown();
		m_pRenderPipeline.reset();
	}
	m_pScene.reset();
	if (m_pRender) {
		m_pRender->GetDevice()->WaitIdle();
		m_pRender->Shutdown();
		m_pRender.reset();
	}
	m_pInput.reset();
	if (m_pWindow) {
		m_pWindow->Shutdown();
		m_pWindow.reset();
	}
	Log::Shutdown();
}

void Engine::SetScene(Scene* pScene) {
	m_pRenderPipeline->SetScene(pScene);
}

bool Engine::LoadScene(const std::filesystem::path& path) {
	m_pScene = std::make_unique<Scene>();
	m_pRenderPipeline->SetScene(m_pScene.get());
	EVO_LOG_INFO("Scene loaded: {}", path.string());
	return true;
}

bool Engine::BeginFrame() {
	// Future: sync — wait for render thread to finish previous frame
	// if (m_bRenderThreadBusy) {
	// 	m_RenderThread.join();
	// 	m_bRenderThreadBusy = false;
	// }

	if (!m_pWindow->PollEvents(m_pInput.get()))
		return false;

	if (m_pWindow->IsMinimized())
		return true;

	m_fDeltaTime = std::chrono::duration<float>(
		Clock::now() - m_LastTime).count();
	m_LastTime = Clock::now();

	m_pRender->HandleResize(
		m_pWindow->GetWidth(), m_pWindow->GetHeight());

	m_pRender->BeginFrame();

	return true;
}

void Engine::RenderFrame() {
	if (m_pWindow->IsMinimized())
		return;

	if (m_pRenderPipeline && m_pScene)
		m_pRenderPipeline->RenderFrame();
}

void Engine::EndFrame() {
	if (m_pWindow->IsMinimized())
		return;

	m_pRender->EndFrame();
}

} // namespace Evo
