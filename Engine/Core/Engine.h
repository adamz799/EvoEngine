#pragma once

#include "Platform/Window.h"
#include "Platform/Input.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderPipeline.h"
#include "Scene/Scene.h"
#include <memory>
#include <chrono>
#include <filesystem>

namespace Evo {

class Editor;

// ============================================================================
// Engine — the application. App only calls:
//   Launch → LoadScene → while(BeginFrame) { EndFrame } → Shutdown
//
// All subsystems (Window, Input, Renderer, Pipeline, Scene) are internal.
// Editor accesses internals via friend.
// ============================================================================

class Engine {
public:
	Engine() = default;
	~Engine();

	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	// ---- Lifecycle ----

	bool Launch();
	void Shutdown();

	// ---- Frame ----

	bool BeginFrame();
	void RenderFrame();   // Pipeline::RenderFrame (Editor injects passes after this)
	void EndFrame();      // Renderer::EndFrame (compile + execute + present)

	// ---- Scene ----

	bool LoadScene(const std::filesystem::path& path);

	/// Use an externally-created scene. Caller retains ownership.
	void SetScene(Scene* pScene);

	// ---- App-visible accessors ----

	Window& GetWindow()    { return *m_pWindow; }
	Input&  GetInput()     { return *m_pInput; }
	float   GetDeltaTime() const { return m_fDeltaTime; }

	/// Current scene — App updates entity state (cameras, transforms) each frame.
	Scene* GetScene() { return m_pScene.get(); }

	/// Renderer — for scene setup only (asset loading). Prefer not to use.
	Render& GetRender() { return *m_pRender; }

private:
	friend class Editor;
	RenderPipeline& GetPipeline() { return *m_pRenderPipeline; }

	std::unique_ptr<Window>         m_pWindow;
	std::unique_ptr<Render>			m_pRender;
	std::unique_ptr<Input>          m_pInput;
	std::unique_ptr<RenderPipeline> m_pRenderPipeline;
	std::unique_ptr<Scene>          m_pScene;
	float                           m_fDeltaTime = 0.0f;

	using Clock = std::chrono::high_resolution_clock;
	Clock::time_point m_LastTime;
};

} // namespace Evo
