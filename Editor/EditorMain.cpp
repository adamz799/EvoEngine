#include "Core/Log.h"
#include "Math/Math.h"
#include "Platform/Window.h"
#include "Platform/Input.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderGraph.h"
#include "Editor.h"
#include "CubeDemo.h"
#include <chrono>

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

    // ---- Create window ----
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

    // ---- Initialize demo ----
    Evo::CubeDemo cubeDemo;
    if (!cubeDemo.Initialize(renderer.GetDevice(), renderer.GetSwapChain()->GetFormat()))
    {
        EVO_LOG_ERROR("Failed to initialize cube demo");
    }

    // ---- Initialize editor ----
    Evo::Editor editor;
    if (!editor.Initialize(renderer.GetDevice(), window, renderer.GetSwapChain()->GetFormat()))
    {
        EVO_LOG_ERROR("Failed to initialize editor");
    }

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

        // ImGui new frame
        editor.BeginFrame();

        cubeDemo.Update(dt, input, window,
                        editor.GetViewportWidth(), editor.GetViewportHeight());
        editor.Update(cubeDemo.GetScene(), cubeDemo.GetCamera(), dt);

        renderer.BeginFrame();

        auto& rg = renderer.GetRenderGraph();
        auto* swapChain = renderer.GetSwapChain();

        // 1. Import viewport texture into render graph
        Evo::RGHandle vpRG = rg.ImportTexture("EditorViewport",
            editor.GetViewportTexture(),
            Evo::RHITextureLayout::Common,
            Evo::RHITextureLayout::Common);

        // 2. Scene renders to off-screen viewport texture
        cubeDemo.Render(renderer, vpRG, editor.GetViewportRTV(),
            static_cast<float>(editor.GetViewportWidth()),
            static_cast<float>(editor.GetViewportHeight()));

        // 3. ImGui renders to back buffer (viewport texture shown via ImGui::Image)
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
    }

    // ---- Shutdown (reverse order) ----
    EVO_LOG_INFO("Shutting down...");
    renderer.GetDevice()->WaitIdle();
    editor.Shutdown();
    cubeDemo.Shutdown(renderer.GetDevice());
    renderer.Shutdown();
    window.Shutdown();
    Evo::Log::Shutdown();

    return 0;
}
