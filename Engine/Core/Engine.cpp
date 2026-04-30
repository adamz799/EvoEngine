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
	m_pRenderer = std::make_unique<Renderer>();
	RendererDesc renderDesc{};
	renderDesc.backend                  = cfg.ResolveBackend();
	renderDesc.bEnableDebugLayer         = cfg.rhi.bEnableDebugLayer;
	renderDesc.bEnableGPUBasedValidation = cfg.rhi.bEnableGPUBasedValidation;

	if (!m_pRenderer->Initialize(renderDesc, *m_pWindow)) {
		EVO_LOG_ERROR("Failed to initialize renderer");
		m_pWindow->Shutdown();
		Log::Shutdown();
		return false;
	}

	// ---- 5. Timing ----
	m_LastTime = Clock::now();

	EVO_LOG_INFO("Engine launched (backend: {})", cfg.rhi.backend);
	return true;
}

void Engine::Shutdown() {
	EVO_LOG_INFO("Shutting down...");

	// Future: join render thread
	// if (m_RenderThread.joinable())
	// 	m_RenderThread.join();

	if (m_pRenderer) {
		m_pRenderer->GetDevice()->WaitIdle();
		m_pRenderer->Shutdown();
		m_pRenderer.reset();
	}
	m_pInput.reset();
	if (m_pWindow) {
		m_pWindow->Shutdown();
		m_pWindow.reset();
	}
	Log::Shutdown();
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

	m_DeltaTime = std::chrono::duration<float>(
		Clock::now() - m_LastTime).count();
	m_LastTime = Clock::now();

	m_pRenderer->HandleResize(
		m_pWindow->GetWidth(), m_pWindow->GetHeight());

	m_pRenderer->BeginFrame();

	return true;
}

void Engine::EndFrame() {
	if (m_pWindow->IsMinimized())
		return;

	m_pRenderer->EndFrame();

	// Future: kick render thread
	// m_bRenderThreadBusy = true;
	// m_RenderThread = std::thread([this]() {
	// 	m_pRenderer->EndFrame();
	// });
}

} // namespace Evo
