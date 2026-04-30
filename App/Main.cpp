#include "Core/Engine.h"
#include "Core/EngineConfig.h"
#include "Core/Log.h"
#include "Math/Math.h"
#include "Renderer/RenderPipeline.h"
#include "TestScene.h"

// ---- D3D12 Agility SDK runtime selection ----
#if EVO_RHI_DX12
extern "C" { __declspec(dllexport) extern const unsigned int D3D12SDKVersion = EVO_D3D12_AGILITY_SDK_VERSION; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
#endif

int main(int /*argc*/, char* /*argv*/[])
{
	Evo::Log::Initialize();

	// ---- Phase 1: Config ----
	Evo::EngineConfig::LoadFromFile("Assets/engine_config.json");

	// ---- Phase 2: Launch engine ----
	Evo::Engine engine;
	if (!engine.Launch())
		return -1;

	auto* pDevice = engine.GetRenderer().GetDevice();

	// ---- App-specific setup ----
	Evo::RenderPipeline pipeline;
	if (!pipeline.Initialize(pDevice, engine.GetRenderer().GetSwapChain()->GetFormat())) {
		EVO_LOG_ERROR("Failed to initialize render pipeline");
		engine.Shutdown();
		return -1;
	}

	Evo::ViewportFrame viewport;
	viewport.Initialize(pDevice,
		pipeline.MakeViewportFrameDesc(
			engine.GetWindow().GetWidth(),
			engine.GetWindow().GetHeight(), "Game"));

	Evo::TestScene testScene;
	if (!testScene.Initialize(pDevice)) {
		EVO_LOG_ERROR("Failed to initialize test scene");
	}

	Evo::Camera camera;
	Evo::FreeCameraController cameraController;
	camera.SetPerspective(Evo::DegToRad(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
	camera.SetPosition(Evo::Vec3(0.0f, 3.0f, -8.0f));
	camera.LookAt(Evo::Vec3::Zero);

	// ---- Phase 3: Main loop ----
	while (engine.BeginFrame()) {
		float dt = engine.GetDeltaTime();
		auto& input = engine.GetInput();
		auto& window = engine.GetWindow();

		viewport.Resize(pDevice, window.GetWidth(), window.GetHeight());

		float w = static_cast<float>(window.GetWidth());
		float h = static_cast<float>(window.GetHeight());
		if (h > 0.0f)
			camera.SetAspect(w / h);
		cameraController.Update(camera, input, window, dt);

		testScene.Update(dt);

		pipeline.RenderShadow(engine.GetRenderer(), testScene.GetScene());
		pipeline.RenderViewport(engine.GetRenderer(), testScene.GetScene(),
			viewport, camera.GetViewProjectionMatrix());

		engine.EndFrame();
	}

	// ---- Shutdown ----
	pDevice->WaitIdle();
	testScene.Shutdown(pDevice);
	viewport.Shutdown(pDevice);
	pipeline.Shutdown(pDevice);
	engine.Shutdown();
	return 0;
}
