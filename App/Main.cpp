#include "Core/Log.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"

int main(int /*argc*/, char* /*argv*/[])
{
    // ---- Initialize logging ----
    Evo::Log::Initialize();
    EVO_LOG_INFO("EvoEngine starting...");

    // ---- Create window ----
    Evo::Window window;
    Evo::WindowDesc winDesc{};
    winDesc.title  = "EvoEngine";
    winDesc.width  = 1280;
    winDesc.height = 720;

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
    renderDesc.enableDebug = true;

    if (!renderer.Initialize(renderDesc, window)) {
        EVO_LOG_ERROR("Failed to initialize renderer (continuing with window only)");
    }

    // ---- Main loop ----
    EVO_LOG_INFO("Entering main loop");
    while (window.PollEvents()) {
        if (window.IsMinimized())
            continue;

        renderer.BeginFrame();

        // TODO: Scene update, render passes, UI, etc.

        renderer.EndFrame();
    }

    // ---- Shutdown (reverse order) ----
    EVO_LOG_INFO("Shutting down...");
    renderer.Shutdown();
    window.Shutdown();
    Evo::Log::Shutdown();

    return 0;
}
