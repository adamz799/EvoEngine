#include "Core/Log.h"
#include "Math/Math.h"
#include "Platform/Window.h"
#include "Platform/Input.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderGraph.h"
#include "Editor.h"
#include "EditorRenderPipeline.h"
#include "TestScene.h"
#include <chrono>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

// ---- D3D12 Agility SDK runtime selection ----
#if EVO_RHI_DX12
extern "C" { __declspec(dllexport) extern const unsigned int D3D12SDKVersion = EVO_D3D12_AGILITY_SDK_VERSION; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
#endif

int main(int /*argc*/, char* /*argv*/[])
{
	// ---- Initialize logging ----
	Evo::Log::Initialize();
	EVO_LOG_INFO("EvoEditor starting...");

	// ---- Create editor window ----
	Evo::Window window;
	Evo::WindowDesc winDesc{};
	winDesc.sTitle  = "EvoEditor";
	winDesc.uWidth  = 1280;
	winDesc.uHeight = 720;

	if (!window.Initialize(winDesc)) {
		EVO_LOG_CRITICAL("Failed to create window");
		Evo::Log::Shutdown();
		return -1;
	}

	// ---- Create runtime window (separate OS window with its own swap chain) ----
	SDL_Window* pRuntimeWindow = SDL_CreateWindow("Runtime", 640, 480, 0);
	if (!pRuntimeWindow) {
		EVO_LOG_ERROR("Failed to create runtime window: {}", SDL_GetError());
	}

	// ---- Create renderer ----
	Evo::Renderer renderer;
	Evo::RendererDesc renderDesc{};
#if EVO_RHI_DX12
	renderDesc.backend = Evo::RHIBackendType::DX12;
#elif EVO_RHI_VULKAN
	renderDesc.backend = Evo::RHIBackendType::Vulkan;
#endif
	renderDesc.bEnableDebug = true;

	if (!renderer.Initialize(renderDesc, window)) {
		EVO_LOG_ERROR("Failed to initialize renderer (continuing with window only)");
	}

	// ---- Create secondary swap chain for runtime window ----
	std::unique_ptr<Evo::RHISwapChain> pRuntimeSC;
	if (pRuntimeWindow)
	{
#if EVO_RHI_DX12
		void* hRuntimeWnd = SDL_GetPointerProperty(
			SDL_GetWindowProperties(pRuntimeWindow),
			SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

		Evo::RHISwapChainDesc rtScDesc{};
		rtScDesc.pWindowHandle = hRuntimeWnd;
		rtScDesc.uWidth        = 640;
		rtScDesc.uHeight       = 480;
		rtScDesc.uBufferCount  = 2;
		pRuntimeSC = renderer.GetDevice()->CreateSwapChain(rtScDesc);
		if (!pRuntimeSC)
			EVO_LOG_ERROR("Failed to create runtime swap chain");
#endif
	}

	auto* pDevice = renderer.GetDevice();
	auto scFormat = renderer.GetSwapChain()->GetFormat();

	// ---- Initialize render pipeline ----
	Evo::EditorRenderPipeline pipeline;
	if (!pipeline.Initialize(pDevice, scFormat))
	{
		EVO_LOG_ERROR("Failed to initialize render pipeline");
	}

	// ---- Initialize test scene ----
	Evo::TestScene testScene;
	if (!testScene.Initialize(pDevice))
	{
		EVO_LOG_ERROR("Failed to initialize test scene");
	}

	// ---- Initialize editor ----
	Evo::Editor editor;
	if (!editor.Initialize(pDevice, window, scFormat, pipeline))
	{
		EVO_LOG_ERROR("Failed to initialize editor");
	}

	// ---- Runtime viewport resources ----
	Evo::ViewportFrame runtimeViewport;
	if (pRuntimeSC)
	{
		runtimeViewport.Initialize(pDevice,
			pipeline.MakeViewportFrameDesc(
				pRuntimeSC->GetWidth(), pRuntimeSC->GetHeight(), "Runtime"));
	}

	// ---- Editor camera (free-roaming) ----
	Evo::Camera editorCamera;
	Evo::FreeCameraController cameraController;
	editorCamera.SetPerspective(Evo::DegToRad(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
	editorCamera.SetPosition(Evo::Vec3(0.0f, 5.0f, -12.0f));
	editorCamera.LookAt(Evo::Vec3::Zero);

	// ---- Main loop ----
	EVO_LOG_INFO("Entering main loop");
	auto lastTime = std::chrono::high_resolution_clock::now();
	Evo::Input input;

	while (window.PollEvents(&input)) {
		if (window.IsMinimized())
			continue;

		// Handle window resize — swap chain must match window size
		renderer.HandleResize(window.GetWidth(), window.GetHeight());

		// Delta time
		auto now = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float>(now - lastTime).count();
		lastTime = now;

		// Update editor camera
		float vpW = static_cast<float>(editor.GetViewportWidth());
		float vpH = static_cast<float>(editor.GetViewportHeight());
		if (vpH > 0.0f)
			editorCamera.SetAspect(vpW / vpH);
		cameraController.Update(editorCamera, input, window, dt);

		// Update scene (cube rotations only)
		testScene.Update(dt);

		// ImGui new frame
		editor.BeginFrame();
		editor.Update(testScene.GetScene(), editorCamera, dt);

		// ---- Render ----
		renderer.BeginFrame();

		auto& rg = renderer.GetRenderGraph();
		auto* swapChain = renderer.GetSwapChain();

		// Shadow pass (once for all viewports)
		pipeline.RenderShadow(renderer, testScene.GetScene());

		// Import editor viewport texture (used by scene render + ImGui read)
		Evo::RGHandle vpRG = rg.ImportTexture("EditorViewport",
			editor.GetViewportTexture(),
			Evo::RHITextureLayout::Common,
			Evo::RHITextureLayout::Common);

		// ---- Editor scene viewport ----
		Evo::Mat4 editorVP = editorCamera.GetViewProjectionMatrix();

		// GBuffer -> Lighting -> Transparent -> PostProcess -> viewport texture
		pipeline.RenderViewport(renderer, testScene.GetScene(),
			editor.GetViewportFrame(), editorVP,
			vpRG, editor.GetViewportRTV());

		// Debug overlay (camera frustum, icon) -> viewport texture
		pipeline.GetDebugRenderer().DrawFrustum(testScene.GetGameCamera(),
			Evo::Vec4(1.0f, 1.0f, 0.0f, 1.0f));
		pipeline.GetDebugRenderer().DrawCameraIcon(testScene.GetGameCamera(),
			Evo::Vec4(1.0f, 1.0f, 0.0f, 1.0f), 0.5f);
		pipeline.RenderDebugOverlay(renderer, vpRG, editor.GetViewportRTV(),
			editorVP, vpW, vpH);

		// ---- Runtime viewport -> runtime swap chain ----
		if (pRuntimeSC)
		{
			Evo::RGHandle rtBB = rg.ImportTexture("RuntimeBackBuffer",
				pRuntimeSC->GetCurrentBackBuffer(),
				Evo::RHITextureLayout::Common,
				Evo::RHITextureLayout::Common);

			float rtW = static_cast<float>(pRuntimeSC->GetWidth());
			float rtH = static_cast<float>(pRuntimeSC->GetHeight());
			if (rtH > 0.0f)
				testScene.GetGameCamera().SetAspect(rtW / rtH);

			pipeline.RenderViewport(renderer, testScene.GetScene(),
				runtimeViewport, testScene.GetGameCamera().GetViewProjectionMatrix(),
				rtBB, pRuntimeSC->GetCurrentBackBufferRTV());
		}

		// ---- ImGui pass -> editor back buffer ----
		{
			auto bbRG = renderer.GetBackBufferRG();

			rg.AddPass("ImGui", [&](Evo::RGPassBuilder& builder) {
				builder.ReadTexture(vpRG);
				builder.WriteRenderTarget(bbRG, swapChain->GetCurrentBackBufferRTV());
			}, [&](Evo::RHICommandList* pCmdList) {
				editor.Render(pCmdList);
			});
		}

		renderer.EndFrame();

		// Present runtime window via its own swap chain
		if (pRuntimeSC)
			pRuntimeSC->Present();
	}

	// ---- Shutdown (reverse order) ----
	EVO_LOG_INFO("Shutting down...");
	pDevice->WaitIdle();
	runtimeViewport.Shutdown(pDevice);
	editor.Shutdown();
	testScene.Shutdown(pDevice);
	pipeline.Shutdown(pDevice);
	pRuntimeSC.reset();
	renderer.Shutdown();
	if (pRuntimeWindow)
		SDL_DestroyWindow(pRuntimeWindow);
	window.Shutdown();
	Evo::Log::Shutdown();

	return 0;
}
