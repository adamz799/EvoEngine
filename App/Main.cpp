#include "Core/Log.h"
#include "Core/EngineConfig.h"
#include "Math/Math.h"
#include "Platform/Window.h"
#include "Platform/Input.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderPipeline.h"
#include "TestScene.h"
#include <chrono>

// ---- D3D12 Agility SDK runtime selection ----
#if EVO_RHI_DX12
extern "C" { __declspec(dllexport) extern const unsigned int D3D12SDKVersion = EVO_D3D12_AGILITY_SDK_VERSION; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
#endif

int main(int /*argc*/, char* /*argv*/[])
{
	// ---- Initialize logging (must come first for config-load messages) ----
	Evo::Log::Initialize();
	EVO_LOG_INFO("EvoApp starting...");

	// ---- Load engine config ----
	auto config = Evo::EngineConfig::Load("Assets/engine_config.json");
	if (!config)
		config = Evo::EngineConfig::Default();

	// ---- Resolve RHI backend from config ----
	Evo::RHIBackendType backend = Evo::RHIBackendType::DX12;
	if (config->rhi.backend == "vulkan") {
#if EVO_RHI_VULKAN
		backend = Evo::RHIBackendType::Vulkan;
#else
		EVO_LOG_WARN("Vulkan requested in config but not compiled in, falling back to DX12");
#endif
	} else if (config->rhi.backend != "dx12") {
		EVO_LOG_WARN("Unknown backend '{}', falling back to DX12", config->rhi.backend);
	}
#if !EVO_RHI_DX12 && EVO_RHI_VULKAN
	if (config->rhi.backend != "vulkan") {
		backend = Evo::RHIBackendType::Vulkan;
		EVO_LOG_WARN("DX12 requested but not compiled in, falling back to Vulkan");
	}
#endif

	// ---- Create window ----
	Evo::Window window;
	Evo::WindowDesc winDesc{};
	winDesc.sTitle  = config->window.sTitle;
	winDesc.uWidth  = config->window.uWidth;
	winDesc.uHeight = config->window.uHeight;

	if (!window.Initialize(winDesc)) {
		EVO_LOG_CRITICAL("Failed to create window");
		Evo::Log::Shutdown();
		return -1;
	}

	// ---- Create renderer ----
	Evo::Renderer renderer;
	Evo::RendererDesc renderDesc{};
	renderDesc.backend                 = backend;
	renderDesc.bEnableDebugLayer        = config->rhi.bEnableDebugLayer;
	renderDesc.bEnableGPUBasedValidation = config->rhi.bEnableGPUBasedValidation;

	if (!renderer.Initialize(renderDesc, window)) {
		EVO_LOG_ERROR("Failed to initialize renderer");
		Evo::Log::Shutdown();
		return -1;
	}

	auto* pDevice = renderer.GetDevice();
	auto* pSwapChain = renderer.GetSwapChain();

	// ---- Create render pipeline ----
	Evo::RenderPipeline pipeline;
	if (!pipeline.Initialize(pDevice, pSwapChain->GetFormat())) {
		EVO_LOG_ERROR("Failed to initialize render pipeline");
		renderer.Shutdown();
		window.Shutdown();
		Evo::Log::Shutdown();
		return -1;
	}

	// ---- Create viewport frame for the game ----
	Evo::ViewportFrame gameViewport;
	gameViewport.Initialize(pDevice,
		pipeline.MakeViewportFrameDesc(window.GetWidth(), window.GetHeight(), "Game"));

	// ---- Initialize test scene ----
	Evo::TestScene testScene;
	if (!testScene.Initialize(pDevice)) {
		EVO_LOG_ERROR("Failed to initialize test scene");
	}

	// ---- Camera + controller ----
	Evo::Camera camera;
	Evo::FreeCameraController cameraController;
	camera.SetPerspective(Evo::DegToRad(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
	camera.SetPosition(Evo::Vec3(0.0f, 3.0f, -8.0f));
	camera.LookAt(Evo::Vec3::Zero);

	// ---- Main loop ----
	EVO_LOG_INFO("Entering main loop");
	auto lastTime = std::chrono::high_resolution_clock::now();
	Evo::Input input;

	while (window.PollEvents(&input)) {
		if (window.IsMinimized())
			continue;

		// Handle window resize
		renderer.HandleResize(window.GetWidth(), window.GetHeight());
		gameViewport.Resize(pDevice, window.GetWidth(), window.GetHeight());

		// Delta time
		auto now = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float>(now - lastTime).count();
		lastTime = now;

		// Update camera
		float w = static_cast<float>(window.GetWidth());
		float h = static_cast<float>(window.GetHeight());
		if (h > 0.0f)
			camera.SetAspect(w / h);
		cameraController.Update(camera, input, window, dt);

		// Update scene
		testScene.Update(dt);

		renderer.BeginFrame();

		pipeline.RenderShadow(renderer, testScene.GetScene());
		pipeline.RenderViewport(renderer, testScene.GetScene(), gameViewport,
			camera.GetViewProjectionMatrix(),
			renderer.GetBackBufferRG(),
			pSwapChain->GetCurrentBackBufferRTV());

		renderer.EndFrame();
	}

	// ---- Shutdown (reverse order) ----
	EVO_LOG_INFO("Shutting down...");
	pDevice->WaitIdle();
	testScene.Shutdown(pDevice);
	gameViewport.Shutdown(pDevice);
	pipeline.Shutdown(pDevice);
	renderer.Shutdown();
	window.Shutdown();
	Evo::Log::Shutdown();

	return 0;
}

