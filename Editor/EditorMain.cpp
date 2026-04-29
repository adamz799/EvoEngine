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
    Evo::RHITextureHandle runtimeDepthTex;
    Evo::RHIDepthStencilView runtimeDepthDSV;
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

        // Depth buffer for runtime window
        Evo::RHITextureDesc rtDepthDesc{};
        rtDepthDesc.uWidth     = 640;
        rtDepthDesc.uHeight    = 480;
        rtDepthDesc.format     = Evo::RHIFormat::D32_FLOAT;
        rtDepthDesc.usage      = Evo::RHITextureUsage::DepthStencil | Evo::RHITextureUsage::ShaderResource;
        rtDepthDesc.sDebugName = "RuntimeDepthBuffer";
        runtimeDepthTex = renderer.GetDevice()->CreateTexture(rtDepthDesc);
        runtimeDepthDSV = renderer.GetDevice()->CreateDepthStencilView(runtimeDepthTex);
#endif
    }

    // ---- Create runtime G-Buffer textures ----
    Evo::RHITextureHandle rtGBAlbedoTex, rtGBNormalTex, rtGBRoughMetTex;
    Evo::RHIRenderTargetView rtGBAlbedoRTV, rtGBNormalRTV, rtGBRoughMetRTV;
    if (pRuntimeWindow)
    {
#if EVO_RHI_DX12
        auto* pDev = renderer.GetDevice();

        Evo::RHITextureDesc albedoDesc{};
        albedoDesc.uWidth     = 640;
        albedoDesc.uHeight    = 480;
        albedoDesc.format     = Evo::RHIFormat::R8G8B8A8_UNORM;
        albedoDesc.usage      = Evo::RHITextureUsage::RenderTarget | Evo::RHITextureUsage::ShaderResource;
        albedoDesc.sDebugName = "RuntimeGBuffer_Albedo";
        rtGBAlbedoTex = pDev->CreateTexture(albedoDesc);
        rtGBAlbedoRTV = pDev->CreateRenderTargetView(rtGBAlbedoTex);

        Evo::RHITextureDesc normalDesc{};
        normalDesc.uWidth     = 640;
        normalDesc.uHeight    = 480;
        normalDesc.format     = Evo::RHIFormat::R16G16B16A16_FLOAT;
        normalDesc.usage      = Evo::RHITextureUsage::RenderTarget | Evo::RHITextureUsage::ShaderResource;
        normalDesc.sDebugName = "RuntimeGBuffer_Normal";
        rtGBNormalTex = pDev->CreateTexture(normalDesc);
        rtGBNormalRTV = pDev->CreateRenderTargetView(rtGBNormalTex);

        Evo::RHITextureDesc roughMetDesc{};
        roughMetDesc.uWidth     = 640;
        roughMetDesc.uHeight    = 480;
        roughMetDesc.format     = Evo::RHIFormat::R8G8B8A8_UNORM;
        roughMetDesc.usage      = Evo::RHITextureUsage::RenderTarget | Evo::RHITextureUsage::ShaderResource;
        roughMetDesc.sDebugName = "RuntimeGBuffer_RoughMet";
        rtGBRoughMetTex = pDev->CreateTexture(roughMetDesc);
        rtGBRoughMetRTV = pDev->CreateRenderTargetView(rtGBRoughMetTex);
#endif
    }

    // ---- Create shared shadow map ----
    constexpr Evo::uint32 kShadowMapSize = 2048;
    Evo::RHITextureHandle shadowTex;
    Evo::RHIDepthStencilView shadowDSV;
    {
        auto* pDev = renderer.GetDevice();
        Evo::RHITextureDesc shadowDesc{};
        shadowDesc.uWidth     = kShadowMapSize;
        shadowDesc.uHeight    = kShadowMapSize;
        shadowDesc.format     = Evo::RHIFormat::D32_FLOAT;
        shadowDesc.usage      = Evo::RHITextureUsage::DepthStencil | Evo::RHITextureUsage::ShaderResource;
        shadowDesc.sDebugName = "ShadowMap";
        shadowTex = pDev->CreateTexture(shadowDesc);
        shadowDSV = pDev->CreateDepthStencilView(shadowTex);
    }

    // ---- Initialize test scene ----
    Evo::TestScene testScene;
    if (!testScene.Initialize(renderer.GetDevice(), renderer.GetSwapChain()->GetFormat()))
    {
        EVO_LOG_ERROR("Failed to initialize test scene");
    }

    // ---- Allocate lighting descriptor sets ----
    auto* pDevice = renderer.GetDevice();

    // Editor viewport lighting descriptor set
    auto editorLightingDescSet = pDevice->AllocateDescriptorSet(testScene.GetLightingSetLayout());

    // Runtime viewport lighting descriptor set
    Evo::RHIDescriptorSetHandle runtimeLightingDescSet;
    if (pRuntimeWindow && rtGBAlbedoTex.IsValid())
    {
        runtimeLightingDescSet = pDevice->AllocateDescriptorSet(testScene.GetLightingSetLayout());
        Evo::RHIDescriptorWrite writes[5] = {};
        writes[0].uBinding = 0; writes[0].type = Evo::RHIDescriptorType::ShaderResource; writes[0].texture = rtGBAlbedoTex;
        writes[1].uBinding = 1; writes[1].type = Evo::RHIDescriptorType::ShaderResource; writes[1].texture = rtGBNormalTex;
        writes[2].uBinding = 2; writes[2].type = Evo::RHIDescriptorType::ShaderResource; writes[2].texture = rtGBRoughMetTex;
        writes[3].uBinding = 3; writes[3].type = Evo::RHIDescriptorType::ShaderResource; writes[3].texture = runtimeDepthTex;
        writes[4].uBinding = 4; writes[4].type = Evo::RHIDescriptorType::ShaderResource; writes[4].texture = shadowTex;
        pDevice->WriteDescriptorSet(runtimeLightingDescSet, writes, 5);
    }

    // ---- Create runtime HDR intermediate texture ----
    Evo::RHITextureHandle rtHDRTex;
    Evo::RHIRenderTargetView rtHDRRTV;
    if (pRuntimeWindow)
    {
        Evo::RHITextureDesc hdrDesc{};
        hdrDesc.uWidth     = 640;
        hdrDesc.uHeight    = 480;
        hdrDesc.format     = Evo::RHIFormat::R16G16B16A16_FLOAT;
        hdrDesc.usage      = Evo::RHITextureUsage::RenderTarget | Evo::RHITextureUsage::ShaderResource;
        hdrDesc.sDebugName = "RuntimeHDRIntermediate";
        rtHDRTex = pDevice->CreateTexture(hdrDesc);
        rtHDRRTV = pDevice->CreateRenderTargetView(rtHDRTex);
    }

    // ---- Allocate post-process descriptor sets ----
    auto editorPostDescSet = pDevice->AllocateDescriptorSet(testScene.GetPostProcessSetLayout());
    // Editor post-process descriptor set is re-written each frame (HDR texture may change on resize)

    Evo::RHIDescriptorSetHandle runtimePostDescSet;
    if (pRuntimeWindow && rtHDRTex.IsValid())
    {
        runtimePostDescSet = pDevice->AllocateDescriptorSet(testScene.GetPostProcessSetLayout());
        Evo::RHIDescriptorWrite write = {};
        write.uBinding = 0;
        write.type     = Evo::RHIDescriptorType::ShaderResource;
        write.texture  = rtHDRTex;
        pDevice->WriteDescriptorSet(runtimePostDescSet, &write, 1);
    }

    // ---- Allocate transparent shadow descriptor sets ----
    auto editorTransShadowDescSet = pDevice->AllocateDescriptorSet(testScene.GetTransparentShadowSetLayout());
    {
        Evo::RHIDescriptorWrite write = {};
        write.uBinding = 0;
        write.type     = Evo::RHIDescriptorType::ShaderResource;
        write.texture  = shadowTex;
        pDevice->WriteDescriptorSet(editorTransShadowDescSet, &write, 1);
    }

    Evo::RHIDescriptorSetHandle runtimeTransShadowDescSet;
    if (pRuntimeWindow)
    {
        runtimeTransShadowDescSet = pDevice->AllocateDescriptorSet(testScene.GetTransparentShadowSetLayout());
        Evo::RHIDescriptorWrite write = {};
        write.uBinding = 0;
        write.type     = Evo::RHIDescriptorType::ShaderResource;
        write.texture  = shadowTex;
        pDevice->WriteDescriptorSet(runtimeTransShadowDescSet, &write, 1);
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

        // Update editor lighting descriptor set (textures may have changed due to resize)
        {
            Evo::RHIDescriptorWrite writes[5] = {};
            writes[0].uBinding = 0; writes[0].type = Evo::RHIDescriptorType::ShaderResource; writes[0].texture = editor.GetGBAlbedoTexture();
            writes[1].uBinding = 1; writes[1].type = Evo::RHIDescriptorType::ShaderResource; writes[1].texture = editor.GetGBNormalTexture();
            writes[2].uBinding = 2; writes[2].type = Evo::RHIDescriptorType::ShaderResource; writes[2].texture = editor.GetGBRoughMetTexture();
            writes[3].uBinding = 3; writes[3].type = Evo::RHIDescriptorType::ShaderResource; writes[3].texture = editor.GetDepthTexture();
            writes[4].uBinding = 4; writes[4].type = Evo::RHIDescriptorType::ShaderResource; writes[4].texture = shadowTex;
            pDevice->WriteDescriptorSet(editorLightingDescSet, writes, 5);
        }

        // Update editor post-process descriptor set (HDR texture may change on resize)
        {
            Evo::RHIDescriptorWrite write = {};
            write.uBinding = 0;
            write.type     = Evo::RHIDescriptorType::ShaderResource;
            write.texture  = editor.GetHDRTexture();
            pDevice->WriteDescriptorSet(editorPostDescSet, &write, 1);
        }

        auto& rg = renderer.GetRenderGraph();
        auto* swapChain = renderer.GetSwapChain();

        // Import shadow map
        Evo::RGHandle shadowRG = rg.ImportTexture("ShadowMap", shadowTex,
            Evo::RHITextureLayout::DepthStencilWrite,
            Evo::RHITextureLayout::DepthStencilWrite);

        // Shadow pass (once for all viewports)
        testScene.RenderShadowMap(renderer, shadowRG, shadowDSV,
                                  static_cast<float>(kShadowMapSize));

        // Import editor viewport texture
        Evo::RGHandle vpRG = rg.ImportTexture("EditorViewport",
            editor.GetViewportTexture(),
            Evo::RHITextureLayout::Common,
            Evo::RHITextureLayout::Common);

        // Import editor depth texture
        Evo::RGHandle vpDepthRG = rg.ImportTexture("EditorViewportDepth",
            editor.GetDepthTexture(),
            Evo::RHITextureLayout::DepthStencilWrite,
            Evo::RHITextureLayout::DepthStencilWrite);

        // Import editor G-Buffer textures
        Evo::RGHandle vpGBAlbedoRG = rg.ImportTexture("EditorGBuffer_Albedo",
            editor.GetGBAlbedoTexture(),
            Evo::RHITextureLayout::Common, Evo::RHITextureLayout::Common);
        Evo::RGHandle vpGBNormalRG = rg.ImportTexture("EditorGBuffer_Normal",
            editor.GetGBNormalTexture(),
            Evo::RHITextureLayout::Common, Evo::RHITextureLayout::Common);
        Evo::RGHandle vpGBRoughMetRG = rg.ImportTexture("EditorGBuffer_RoughMet",
            editor.GetGBRoughMetTexture(),
            Evo::RHITextureLayout::Common, Evo::RHITextureLayout::Common);

        // Import editor HDR intermediate
        Evo::RGHandle editorHDRRG = rg.ImportTexture("EditorHDR",
            editor.GetHDRTexture(),
            Evo::RHITextureLayout::Common, Evo::RHITextureLayout::Common);

        // Pass A0: Scene G-Buffer -> editor viewport (editor camera)
        Evo::Mat4 editorVP = editorCamera.GetViewProjectionMatrix();
        {
            Evo::GBufferTargets gbTargets;
            gbTargets.albedoTexture   = vpGBAlbedoRG;
            gbTargets.normalTexture   = vpGBNormalRG;
            gbTargets.roughMetTexture = vpGBRoughMetRG;
            gbTargets.depthTexture    = vpDepthRG;
            gbTargets.albedoRTV       = editor.GetGBAlbedoRTV();
            gbTargets.normalRTV       = editor.GetGBNormalRTV();
            gbTargets.roughMetRTV     = editor.GetGBRoughMetRTV();
            gbTargets.depthDSV        = editor.GetDepthDSV();
            testScene.RenderGBuffer(renderer, gbTargets, editorVP, vpW, vpH);

            // Pass A1: Deferred lighting -> HDR intermediate
            testScene.RenderLighting(renderer, gbTargets, editorLightingDescSet,
                shadowRG, editorHDRRG, editor.GetHDRRTV(), editorVP, vpW, vpH);
        }

        // Pass A1.5: Forward transparent -> HDR intermediate
        testScene.RenderForwardTransparent(renderer, editorTransShadowDescSet,
            editorVP, editorHDRRG, editor.GetHDRRTV(),
            vpDepthRG, editor.GetDepthDSV(), shadowRG, vpW, vpH);

        // Pass A2: Post-process (tone mapping) -> editor viewport
        testScene.RenderPostProcess(renderer, editorPostDescSet, editorHDRRG,
            vpRG, editor.GetViewportRTV(), vpW, vpH);

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

            Evo::RGHandle rtDepthRG = rg.ImportTexture("RuntimeDepthBuffer",
                runtimeDepthTex,
                Evo::RHITextureLayout::DepthStencilWrite,
                Evo::RHITextureLayout::DepthStencilWrite);

            float rtW = static_cast<float>(pRuntimeSC->GetWidth());
            float rtH = static_cast<float>(pRuntimeSC->GetHeight());
            if (rtH > 0.0f)
                testScene.GetGameCamera().SetAspect(rtW / rtH);

            Evo::Mat4 gameVP = testScene.GetGameCamera().GetViewProjectionMatrix();

            // G-Buffer pass for runtime window
            Evo::RGHandle rtGBAlbedoRG = rg.ImportTexture("RuntimeGBuffer_Albedo",
                rtGBAlbedoTex,
                Evo::RHITextureLayout::Common, Evo::RHITextureLayout::Common);
            Evo::RGHandle rtGBNormalRG = rg.ImportTexture("RuntimeGBuffer_Normal",
                rtGBNormalTex,
                Evo::RHITextureLayout::Common, Evo::RHITextureLayout::Common);
            Evo::RGHandle rtGBRoughMetRG = rg.ImportTexture("RuntimeGBuffer_RoughMet",
                rtGBRoughMetTex,
                Evo::RHITextureLayout::Common, Evo::RHITextureLayout::Common);

            // Import runtime HDR intermediate
            Evo::RGHandle rtHDRRG = rg.ImportTexture("RuntimeHDR",
                rtHDRTex,
                Evo::RHITextureLayout::Common, Evo::RHITextureLayout::Common);

            {
                Evo::GBufferTargets gbTargets;
                gbTargets.albedoTexture   = rtGBAlbedoRG;
                gbTargets.normalTexture   = rtGBNormalRG;
                gbTargets.roughMetTexture = rtGBRoughMetRG;
                gbTargets.depthTexture    = rtDepthRG;
                gbTargets.albedoRTV       = rtGBAlbedoRTV;
                gbTargets.normalRTV       = rtGBNormalRTV;
                gbTargets.roughMetRTV     = rtGBRoughMetRTV;
                gbTargets.depthDSV        = runtimeDepthDSV;
                testScene.RenderGBuffer(renderer, gbTargets, gameVP, rtW, rtH);

                // Deferred lighting -> runtime HDR intermediate
                testScene.RenderLighting(renderer, gbTargets, runtimeLightingDescSet,
                    shadowRG, rtHDRRG, rtHDRRTV, gameVP, rtW, rtH);
            }

            // Forward transparent -> runtime HDR intermediate
            testScene.RenderForwardTransparent(renderer, runtimeTransShadowDescSet,
                gameVP, rtHDRRG, rtHDRRTV,
                rtDepthRG, runtimeDepthDSV, shadowRG, rtW, rtH);

            // Post-process (tone mapping) -> runtime back buffer
            testScene.RenderPostProcess(renderer, runtimePostDescSet, rtHDRRG,
                rtBB, pRuntimeSC->GetCurrentBackBufferRTV(), rtW, rtH);
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
    if (editorLightingDescSet.IsValid())
        pDevice->FreeDescriptorSet(editorLightingDescSet);
    if (runtimeLightingDescSet.IsValid())
        pDevice->FreeDescriptorSet(runtimeLightingDescSet);
    if (editorPostDescSet.IsValid())
        pDevice->FreeDescriptorSet(editorPostDescSet);
    if (runtimePostDescSet.IsValid())
        pDevice->FreeDescriptorSet(runtimePostDescSet);
    if (editorTransShadowDescSet.IsValid())
        pDevice->FreeDescriptorSet(editorTransShadowDescSet);
    if (runtimeTransShadowDescSet.IsValid())
        pDevice->FreeDescriptorSet(runtimeTransShadowDescSet);
    if (rtHDRTex.IsValid())
    {
        renderer.GetDevice()->DestroyRenderTargetView(rtHDRRTV);
        renderer.GetDevice()->DestroyTexture(rtHDRTex);
    }
    if (shadowTex.IsValid())
    {
        renderer.GetDevice()->DestroyDepthStencilView(shadowDSV);
        renderer.GetDevice()->DestroyTexture(shadowTex);
    }
    if (runtimeDepthTex.IsValid())
    {
        renderer.GetDevice()->DestroyDepthStencilView(runtimeDepthDSV);
        renderer.GetDevice()->DestroyTexture(runtimeDepthTex);
    }
    if (rtGBRoughMetTex.IsValid())
    {
        renderer.GetDevice()->DestroyRenderTargetView(rtGBRoughMetRTV);
        renderer.GetDevice()->DestroyTexture(rtGBRoughMetTex);
    }
    if (rtGBNormalTex.IsValid())
    {
        renderer.GetDevice()->DestroyRenderTargetView(rtGBNormalRTV);
        renderer.GetDevice()->DestroyTexture(rtGBNormalTex);
    }
    if (rtGBAlbedoTex.IsValid())
    {
        renderer.GetDevice()->DestroyRenderTargetView(rtGBAlbedoRTV);
        renderer.GetDevice()->DestroyTexture(rtGBAlbedoTex);
    }
    pRuntimeSC.reset();
    renderer.Shutdown();
    if (pRuntimeWindow)
        SDL_DestroyWindow(pRuntimeWindow);
    window.Shutdown();
    Evo::Log::Shutdown();

    return 0;
}
