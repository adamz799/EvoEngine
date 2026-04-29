#include "Core/Log.h"
#include "Math/Math.h"
#include "Platform/Window.h"
#include "Platform/Input.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderGraph.h"
#include "Asset/ShaderAsset.h"
#include "Editor.h"
#include "DebugRenderer.h"
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

    // ---- Initialize test scene ----
    Evo::TestScene testScene;
    if (!testScene.Initialize(renderer.GetDevice(), renderer.GetSwapChain()->GetFormat()))
    {
        EVO_LOG_ERROR("Failed to initialize test scene");
    }

    // ---- Initialize editor ----
    Evo::Editor editor;
    if (!editor.Initialize(renderer.GetDevice(), window, renderer.GetSwapChain()->GetFormat()))
    {
        EVO_LOG_ERROR("Failed to initialize editor");
    }

    // ---- Initialize debug renderer ----
    Evo::DebugRenderer debugRenderer;
    Evo::AssetManager debugAssetManager;
    debugAssetManager.Initialize(renderer.GetDevice());
    debugAssetManager.RegisterFactory(".hlsl", [] { return std::make_unique<Evo::ShaderAsset>(); });
    if (!debugRenderer.Initialize(renderer.GetDevice(), debugAssetManager,
                                   renderer.GetSwapChain()->GetFormat()))
    {
        EVO_LOG_ERROR("Failed to initialize debug renderer");
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

        // Import editor viewport texture
        Evo::RGHandle vpRG = rg.ImportTexture("EditorViewport",
            editor.GetViewportTexture(),
            Evo::RHITextureLayout::Common,
            Evo::RHITextureLayout::Common);

        // Pass A: Scene -> editor viewport (editor camera)
        Evo::Mat4 editorVP = editorCamera.GetViewProjectionMatrix();
        testScene.Render(renderer, vpRG, editor.GetViewportRTV(),
            editorVP, vpW, vpH);

        // Pass B: Camera gizmo -> editor viewport (debug lines)
        debugRenderer.DrawFrustum(testScene.GetGameCamera(), Evo::Vec4(1.0f, 1.0f, 0.0f, 1.0f));
        debugRenderer.DrawCameraIcon(testScene.GetGameCamera(), Evo::Vec4(1.0f, 1.0f, 0.0f, 1.0f), 0.5f);
        debugRenderer.Render(renderer, vpRG, editor.GetViewportRTV(),
            editorVP, vpW, vpH);

        // Pass C: Scene -> runtime swap chain back buffer (game camera)
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

            Evo::Mat4 gameVP = testScene.GetGameCamera().GetViewProjectionMatrix();
            testScene.Render(renderer, rtBB, pRuntimeSC->GetCurrentBackBufferRTV(),
                gameVP, rtW, rtH);
        }

        // Pass D: ImGui -> editor back buffer
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
    renderer.GetDevice()->WaitIdle();
    debugRenderer.Shutdown(renderer.GetDevice());
    debugAssetManager.Shutdown();
    editor.Shutdown();
    testScene.Shutdown(renderer.GetDevice());
    pRuntimeSC.reset();
    renderer.Shutdown();
    if (pRuntimeWindow)
        SDL_DestroyWindow(pRuntimeWindow);
    window.Shutdown();
    Evo::Log::Shutdown();

    return 0;
}
