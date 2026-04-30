#pragma once

#include "Platform/Window.h"
#include "Platform/Input.h"
#include "Renderer/Renderer.h"
#include <memory>
#include <chrono>

namespace Evo {

// ============================================================================
// Engine — application framework singleton.
//
// Lifecycle:
//   EngineConfig::LoadFromFile("...");   // Phase 1: config
//   Engine engine;
//   engine.Launch();                      // Phase 2: init all subsystems
//   while (engine.BeginFrame()) { ... }   // Phase 3: main loop
//   engine.Shutdown();
//
// Thread boundary (future):
//   BeginFrame() = sync point  (main thread waits for previous render frame)
//   EndFrame()   = kick point  (launches render thread to execute GPU commands)
// ============================================================================

class Engine {
public:
	Engine() = default;
	~Engine();

	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	// ---- Lifecycle ----

	/// Initialize all subsystems (log, input, window, renderer).
	/// EngineConfig must already be loaded. Returns false on critical failure.
	bool Launch();

	/// Tear down in reverse init order. Safe if Launch failed.
	void Shutdown();

	// ---- Frame ----

	/// Pump events, calc delta, handle resize, begin GPU frame.
	/// Returns false when quit is requested.
	bool BeginFrame();

	/// Execute render graph and present. Call once per frame.
	void EndFrame();

	// ---- Accessors ----

	Window&     GetWindow()    { return *m_pWindow; }
	Renderer&   GetRenderer()  { return *m_pRenderer; }
	Input&      GetInput()     { return *m_pInput; }
	float       GetDeltaTime() const { return m_DeltaTime; }

private:
	std::unique_ptr<Window>   m_pWindow;
	std::unique_ptr<Renderer> m_pRenderer;
	std::unique_ptr<Input>    m_pInput;
	float                     m_DeltaTime = 0.0f;

	using Clock = std::chrono::high_resolution_clock;
	Clock::time_point m_LastTime;

	// ---- Future: render thread ----
	// std::thread         m_RenderThread;
	// std::atomic<bool>   m_bRenderThreadBusy{false};
};

} // namespace Evo
