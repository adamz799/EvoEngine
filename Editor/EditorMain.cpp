#include "Core/Log.h"
#include "Core/EngineConfig.h"
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
	// ---- Initialize logging (must come first for config-load messages) ----
	Evo::Log::Initialize();
	EVO_LOG_INFO("EvoEditor starting...");

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

	// ---- Create editor window ----
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

	// ---- Create runtime window (separate OS window with its own swap chain) ----
	SDL_Window* pRuntimeWindow = SDL_CreateWindow("Runtime", 640, 480, SDL_WINDOW_RESIZABLE);
	if (!pRuntimeWindow) {
		EVO_LOG_ERROR("Failed to create runtime window: {}", SDL_GetError());
	}

	// ---- Create renderer ----
	Evo::Renderer renderer;
	Evo::RendererDesc renderDesc{};
	renderDesc.backend                 = backend;
	renderDesc.bEnableDebugLayer        = config->rhi.bEnableDebugLayer;
	renderDesc.bEnableGPUBasedValidation = config->rhi.bEnableGPUBasedValidation;

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
	editorCamera.SetPerspective(Evo::DegToRad(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
	editorCamera.SetPosition(Evo::Vec3(0.0f, 5.0f, -12.0f));
	editorCamera.LookAt(Evo::Vec3::Zero);

	// ---- Main loop ----
	EVO_LOG_INFO("Entering main loop");
	auto lastTime = std::chrono::high_resolution_clock::now();
	Evo::Input input;

	while (window.PollEvents(&input)) {
		if (window.IsMinimized())
			continue;

		// Handle window resize �?swap chain must match window size
		renderer.HandleResize(window.GetWidth(), window.GetHeight());

		// Handle runtime window resize
		if (pRuntimeSC)
		{
			int rtW = 0, rtH = 0;
			SDL_GetWindowSize(pRuntimeWindow, &rtW, &rtH);
			if (rtW > 0 && rtH > 0
				&& (static_cast<Evo::uint32>(rtW) != pRuntimeSC->GetWidth()
				 || static_cast<Evo::uint32>(rtH) != pRuntimeSC->GetHeight()))
			{
				pDevice->WaitIdle();
				pRuntimeSC->Resize(static_cast<Evo::uint32>(rtW), static_cast<Evo::uint32>(rtH));
				runtimeViewport.Resize(pDevice,
					pRuntimeSC->GetWidth(), pRuntimeSC->GetHeight());
			}
		}

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

		// Debug overlay (camera frustum, icon, gizmo) -> viewport texture
		{
			auto& scene = testScene.GetScene();
			auto camEntity = testScene.GetGameCameraEntity();
			auto* pCamTransform = scene.Transforms().Get(camEntity);
			auto* pCamComp = scene.Cameras().Get(camEntity);
			if (pCamTransform && pCamComp)
			{
				Evo::Camera gameCamera = Evo::BuildCameraFromEntity(*pCamTransform, *pCamComp, 16.0f / 9.0f);
				pipeline.GetDebugRenderer().DrawFrustum(gameCamera,
					Evo::Vec4(1.0f, 1.0f, 0.0f, 1.0f));
				pipeline.GetDebugRenderer().DrawCameraIcon(gameCamera,
					Evo::Vec4(1.0f, 1.0f, 0.0f, 1.0f), 0.5f);
			}

			// Translation gizmo for selected entity
			auto selectedEntity = editor.GetSelectedEntity();
			if (selectedEntity.IsValid() && scene.IsAlive(selectedEntity))
			{
				auto* pTransform = scene.Transforms().Get(selectedEntity);
				if (pTransform)
				{
					pipeline.GetDebugRenderer().DrawTranslationGizmo(
						pTransform->vPosition, 2.0f, editor.GetHoveredAxis());
				}
			}
		}
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

			auto& scene = testScene.GetScene();
			auto camEntity = testScene.GetGameCameraEntity();
			auto* pCamTransform = scene.Transforms().Get(camEntity);
			auto* pCamComp = scene.Cameras().Get(camEntity);
			if (pCamTransform && pCamComp)
			{
				float rtAspect = (rtH > 0.0f) ? rtW / rtH : 1.0f;
				Evo::Camera gameCamera = Evo::BuildCameraFromEntity(*pCamTransform, *pCamComp, rtAspect);

				pipeline.RenderViewport(renderer, testScene.GetScene(),
					runtimeViewport, gameCamera.GetViewProjectionMatrix(),
					rtBB, pRuntimeSC->GetCurrentBackBufferRTV());
			}
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

