#include "Core/Engine.h"
#include "Core/EngineConfig.h"
#include "Core/Log.h"
#include "Math/Math.h"
#include "Renderer/RenderGraph.h"
#include "Editor.h"
#include "EditorRenderPipeline.h"
#include "TestScene.h"

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

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
	Evo::Engine EvoEngine;
	if (!EvoEngine.Launch())
		return -1;

	auto* pRender = &EvoEngine.GetRender();

	// ---- Create runtime window (separate OS window with its own swap chain) ----
	SDL_Window* pRuntimeWindow = SDL_CreateWindow("Runtime", 640, 480, SDL_WINDOW_RESIZABLE);
	if (!pRuntimeWindow)
		EVO_LOG_ERROR("Failed to create runtime window: {}", SDL_GetError());

	// ---- Create secondary window target for runtime window ----
	Evo::WindowTarget runtimeWin;
	if (pRuntimeWindow)
	{
		void* hRuntimeWnd = SDL_GetPointerProperty(
			SDL_GetWindowProperties(pRuntimeWindow),
			SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

		runtimeWin = pRender->CreateWindowTarget(hRuntimeWnd, 640, 480);
	}

	// ---- Initialize editor render pipeline ----
	Evo::EditorRenderPipeline pipeline;
	if (!pipeline.Initialize(pRender))
		EVO_LOG_ERROR("Failed to initialize render pipeline");

	// ---- Initialize test scene ----
	Evo::TestScene testScene;
	if (!testScene.Initialize(pRender))
		EVO_LOG_ERROR("Failed to initialize test scene");

	pipeline.SetScene(testScene.GetScene());

	// ---- Initialize editor ----
	Evo::Editor editor;
	if (!editor.Initialize(pRender, EvoEngine.GetWindow(), pipeline))
		EVO_LOG_ERROR("Failed to initialize editor");

	// ---- Runtime viewport resources ----
	Evo::ViewportFrame runtimeViewport;
	if (pRuntimeWindow)
	{
		runtimeViewport.Initialize(pRender,
			pipeline.MakeViewportFrameDesc(
				runtimeWin.GetWidth(), runtimeWin.GetHeight(), "Runtime"));
	}

	// ---- Editor camera (free-roaming) ----
	Evo::Camera editorCamera;
	Evo::FreeCameraController cameraController;
	editorCamera.SetPerspective(Evo::DegToRad(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
	editorCamera.SetPosition(Evo::Vec3(0.0f, 5.0f, -12.0f));
	editorCamera.LookAt(Evo::Vec3::Zero);

	// ---- Phase 3: Main loop ----
	while (EvoEngine.BeginFrame()) {
		float dt = EvoEngine.GetDeltaTime();
		auto& input = EvoEngine.GetInput();
		auto& window = EvoEngine.GetWindow();

		// Handle runtime window resize
		if (pRuntimeWindow)
		{
			int rtW = 0, rtH = 0;
			SDL_GetWindowSize(pRuntimeWindow, &rtW, &rtH);
			if (rtW > 0 && rtH > 0
				&& (static_cast<Evo::uint32>(rtW) != runtimeWin.GetWidth()
				 || static_cast<Evo::uint32>(rtH) != runtimeWin.GetHeight()))
			{
				pRender->WaitIdle();
				pRender->ResizeWindowTarget(runtimeWin,
				static_cast<Evo::uint32>(rtW), static_cast<Evo::uint32>(rtH));
				runtimeViewport.Resize(
					runtimeWin.GetWidth(), runtimeWin.GetHeight());
			}
		}

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
		auto& rg = pRender->GetRenderGraph();

		// Shadow pass (once for all viewports)
		pipeline.RenderShadow();

		// Import editor viewport texture (used by scene render + ImGui read)
		Evo::RGHandle vpRG = rg.ImportTexture("EditorViewport",
			editor.GetViewportTexture(),
			Evo::RHITextureLayout::Common,
			Evo::RHITextureLayout::Common);

		// ---- Editor scene viewport ----
		Evo::Mat4 editorVP = editorCamera.GetViewProjectionMatrix();

		// GBuffer -> Lighting -> Transparent -> PostProcess -> viewport texture
		pipeline.RenderViewport(editor.GetViewportFrame(), editorVP,
			vpRG, editor.GetViewportRTV());

		// Debug overlay (camera frustum, icon, gizmo) -> viewport texture
		{
			auto* pScene = testScene.GetScene();
			auto camEntity = testScene.GetGameCameraEntity();
			auto* pCamTransform = pScene->Transforms().Get(camEntity);
			auto* pCamComp = pScene->Cameras().Get(camEntity);
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
			if (selectedEntity.IsValid() && pScene->IsAlive(selectedEntity))
			{
				auto* pTransform = pScene->Transforms().Get(selectedEntity);
				if (pTransform)
				{
					pipeline.GetDebugRenderer().DrawTranslationGizmo(
						pTransform->vPosition, 2.0f, editor.GetHoveredAxis());
				}
			}
		}
		pipeline.RenderDebugOverlay(pRender, vpRG, editor.GetViewportRTV(),
			editorVP, vpW, vpH);

		// ---- Runtime viewport -> runtime swap chain ----
		if (pRuntimeWindow)
		{
			Evo::RGHandle rtBB = rg.ImportTexture("RuntimeBackBuffer",
				runtimeWin.GetCurrentBackBuffer(),
				Evo::RHITextureLayout::Common,
				Evo::RHITextureLayout::Common);

			float rtW = static_cast<float>(runtimeWin.GetWidth());
			float rtH = static_cast<float>(runtimeWin.GetHeight());

			auto* pScene = testScene.GetScene();
			auto camEntity = testScene.GetGameCameraEntity();
			auto* pCamTransform = pScene->Transforms().Get(camEntity);
			auto* pCamComp = pScene->Cameras().Get(camEntity);
			if (pCamTransform && pCamComp)
			{
				float rtAspect = (rtH > 0.0f) ? rtW / rtH : 1.0f;
				Evo::Camera gameCamera = Evo::BuildCameraFromEntity(*pCamTransform, *pCamComp, rtAspect);

				pipeline.RenderViewport(runtimeViewport,
					gameCamera.GetViewProjectionMatrix(),
					rtBB, runtimeWin.GetCurrentBackBufferRTV());
			}
		}

		// ---- ImGui pass -> editor back buffer ----
		editor.CompositeToBackBuffer(pRender);

		EvoEngine.EndFrame();

		// Present runtime window via its own swap chain
		if (pRuntimeWindow)
			pRender->PresentWindowTarget(runtimeWin);
	}

	// ---- Shutdown ----
	pRender->WaitIdle();
	runtimeViewport.Shutdown();
	editor.Shutdown();
	testScene.Shutdown(pRender);
	pipeline.Shutdown();
	// runtimeWin destroyed automatically
	if (pRuntimeWindow)
		SDL_DestroyWindow(pRuntimeWindow);
	EvoEngine.Shutdown();

	return 0;
}
