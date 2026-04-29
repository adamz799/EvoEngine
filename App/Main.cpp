#include "Core/Log.h"
#include "Math/Math.h"
#include "Platform/Window.h"
#include "Platform/Input.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderGraph.h"
#include "TestScene.h"
#include <chrono>

// ---- D3D12 Agility SDK runtime selection ----
// These exports tell the D3D12 loader to use the Agility SDK DLLs
// from the ./D3D12/ subdirectory next to the executable.
#if EVO_RHI_DX12
extern "C" { __declspec(dllexport) extern const unsigned int D3D12SDKVersion = EVO_D3D12_AGILITY_SDK_VERSION; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
#endif

int main(int /*argc*/, char* /*argv*/[])
{
    // ---- Initialize logging ----
    Evo::Log::Initialize();
    EVO_LOG_INFO("EvoApp starting...");

    // ---- Create window ----
    Evo::Window window;
    Evo::WindowDesc winDesc{};
    winDesc.sTitle  = "EvoApp";
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

    // ---- Create depth buffer (also readable as SRV for deferred lighting) ----
    auto* pDevice = renderer.GetDevice();
    Evo::RHITextureDesc depthDesc{};
    depthDesc.uWidth     = window.GetWidth();
    depthDesc.uHeight    = window.GetHeight();
    depthDesc.format     = Evo::RHIFormat::D32_FLOAT;
    depthDesc.usage      = Evo::RHITextureUsage::DepthStencil | Evo::RHITextureUsage::ShaderResource;
    depthDesc.sDebugName = "DepthBuffer";
    auto depthTex = pDevice->CreateTexture(depthDesc);
    auto depthDSV = pDevice->CreateDepthStencilView(depthTex);

    // ---- Create shadow map ----
    constexpr Evo::uint32 kShadowMapSize = 2048;
    Evo::RHITextureDesc shadowDesc{};
    shadowDesc.uWidth     = kShadowMapSize;
    shadowDesc.uHeight    = kShadowMapSize;
    shadowDesc.format     = Evo::RHIFormat::D32_FLOAT;
    shadowDesc.usage      = Evo::RHITextureUsage::DepthStencil | Evo::RHITextureUsage::ShaderResource;
    shadowDesc.sDebugName = "ShadowMap";
    auto shadowTex = pDevice->CreateTexture(shadowDesc);
    auto shadowDSV = pDevice->CreateDepthStencilView(shadowTex);

    // ---- Create G-Buffer textures ----
    Evo::uint32 gbW = window.GetWidth();
    Evo::uint32 gbH = window.GetHeight();

    Evo::RHITextureDesc albedoDesc{};
    albedoDesc.uWidth     = gbW;
    albedoDesc.uHeight    = gbH;
    albedoDesc.format     = Evo::RHIFormat::R8G8B8A8_UNORM;
    albedoDesc.usage      = Evo::RHITextureUsage::RenderTarget | Evo::RHITextureUsage::ShaderResource;
    albedoDesc.sDebugName = "GBuffer_Albedo";
    albedoDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
    auto gbAlbedoTex = pDevice->CreateTexture(albedoDesc);
    auto gbAlbedoRTV = pDevice->CreateRenderTargetView(gbAlbedoTex);

    Evo::RHITextureDesc normalDesc{};
    normalDesc.uWidth     = gbW;
    normalDesc.uHeight    = gbH;
    normalDesc.format     = Evo::RHIFormat::R16G16B16A16_FLOAT;
    normalDesc.usage      = Evo::RHITextureUsage::RenderTarget | Evo::RHITextureUsage::ShaderResource;
    normalDesc.sDebugName = "GBuffer_Normal";
    normalDesc.clearColor = { 0.5f, 0.5f, 1.0f, 0.0f };
    auto gbNormalTex = pDevice->CreateTexture(normalDesc);
    auto gbNormalRTV = pDevice->CreateRenderTargetView(gbNormalTex);

    Evo::RHITextureDesc roughMetDesc{};
    roughMetDesc.uWidth     = gbW;
    roughMetDesc.uHeight    = gbH;
    roughMetDesc.format     = Evo::RHIFormat::R8G8B8A8_UNORM;
    roughMetDesc.usage      = Evo::RHITextureUsage::RenderTarget | Evo::RHITextureUsage::ShaderResource;
    roughMetDesc.sDebugName = "GBuffer_RoughMet";
    roughMetDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
    auto gbRoughMetTex = pDevice->CreateTexture(roughMetDesc);
    auto gbRoughMetRTV = pDevice->CreateRenderTargetView(gbRoughMetTex);

    // ---- Initialize test scene ----
    Evo::TestScene testScene;
    if (!testScene.Initialize(renderer.GetDevice(), renderer.GetSwapChain()->GetFormat()))
    {
        EVO_LOG_ERROR("Failed to initialize test scene");
    }

    // ---- Allocate descriptor set for lighting pass ----
    auto lightingDescSet = pDevice->AllocateDescriptorSet(testScene.GetLightingSetLayout());
    {
        Evo::RHIDescriptorWrite writes[5] = {};
        writes[0].uBinding = 0;
        writes[0].type     = Evo::RHIDescriptorType::ShaderResource;
        writes[0].texture  = gbAlbedoTex;
        writes[1].uBinding = 1;
        writes[1].type     = Evo::RHIDescriptorType::ShaderResource;
        writes[1].texture  = gbNormalTex;
        writes[2].uBinding = 2;
        writes[2].type     = Evo::RHIDescriptorType::ShaderResource;
        writes[2].texture  = gbRoughMetTex;
        writes[3].uBinding = 3;
        writes[3].type     = Evo::RHIDescriptorType::ShaderResource;
        writes[3].texture  = depthTex;
        writes[4].uBinding = 4;
        writes[4].type     = Evo::RHIDescriptorType::ShaderResource;
        writes[4].texture  = shadowTex;
        pDevice->WriteDescriptorSet(lightingDescSet, writes, 5);
    }

    // ---- Create HDR intermediate texture for post-processing ----
    Evo::RHITextureDesc hdrDesc{};
    hdrDesc.uWidth     = window.GetWidth();
    hdrDesc.uHeight    = window.GetHeight();
    hdrDesc.format     = Evo::RHIFormat::R16G16B16A16_FLOAT;
    hdrDesc.usage      = Evo::RHITextureUsage::RenderTarget | Evo::RHITextureUsage::ShaderResource;
    hdrDesc.sDebugName = "HDRIntermediate";
    hdrDesc.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    auto hdrTex = pDevice->CreateTexture(hdrDesc);
    auto hdrRTV = pDevice->CreateRenderTargetView(hdrTex);

    // ---- Allocate descriptor set for post-processing ----
    auto postDescSet = pDevice->AllocateDescriptorSet(testScene.GetPostProcessSetLayout());
    {
        Evo::RHIDescriptorWrite write = {};
        write.uBinding = 0;
        write.type     = Evo::RHIDescriptorType::ShaderResource;
        write.texture  = hdrTex;
        pDevice->WriteDescriptorSet(postDescSet, &write, 1);
    }

    // ---- Allocate descriptor set for forward transparent pass (shadow map SRV) ----
    auto transparentShadowDescSet = pDevice->AllocateDescriptorSet(testScene.GetTransparentShadowSetLayout());
    {
        Evo::RHIDescriptorWrite write = {};
        write.uBinding = 0;
        write.type     = Evo::RHIDescriptorType::ShaderResource;
        write.texture  = shadowTex;
        pDevice->WriteDescriptorSet(transparentShadowDescSet, &write, 1);
    }

    // ---- Own camera + controller for runtime play ----
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

        auto bbRG = renderer.GetBackBufferRG();
        auto* swapChain = renderer.GetSwapChain();

        auto& rg = renderer.GetRenderGraph();
        Evo::RGHandle depthRG = rg.ImportTexture("DepthBuffer", depthTex,
            Evo::RHITextureLayout::Common,
            Evo::RHITextureLayout::Common);

        // Import shadow map
        Evo::RGHandle shadowRG = rg.ImportTexture("ShadowMap", shadowTex,
            Evo::RHITextureLayout::Common,
            Evo::RHITextureLayout::Common);

        // Import G-Buffer textures
        Evo::RGHandle gbAlbedoRG = rg.ImportTexture("GBuffer_Albedo", gbAlbedoTex,
            Evo::RHITextureLayout::Common, Evo::RHITextureLayout::Common);
        Evo::RGHandle gbNormalRG = rg.ImportTexture("GBuffer_Normal", gbNormalTex,
            Evo::RHITextureLayout::Common, Evo::RHITextureLayout::Common);
        Evo::RGHandle gbRoughMetRG = rg.ImportTexture("GBuffer_RoughMet", gbRoughMetTex,
            Evo::RHITextureLayout::Common, Evo::RHITextureLayout::Common);

        // G-Buffer pass
        Evo::GBufferTargets gbTargets;
        gbTargets.albedoTexture   = gbAlbedoRG;
        gbTargets.normalTexture   = gbNormalRG;
        gbTargets.roughMetTexture = gbRoughMetRG;
        gbTargets.depthTexture    = depthRG;
        gbTargets.albedoRTV       = gbAlbedoRTV;
        gbTargets.normalRTV       = gbNormalRTV;
        gbTargets.roughMetRTV     = gbRoughMetRTV;
        gbTargets.depthDSV        = depthDSV;

        // Import HDR intermediate
        Evo::RGHandle hdrRG = rg.ImportTexture("HDRIntermediate", hdrTex,
            Evo::RHITextureLayout::Common, Evo::RHITextureLayout::Common);

        Evo::Mat4 vp = camera.GetViewProjectionMatrix();

        // Shadow pass
        testScene.RenderShadowMap(renderer, shadowRG, shadowDSV,
                                  static_cast<float>(kShadowMapSize));

        // G-Buffer pass
        testScene.RenderGBuffer(renderer, gbTargets, vp, w, h);

        // Deferred lighting pass: G-Buffer + shadow -> HDR intermediate
        testScene.RenderLighting(renderer, gbTargets, lightingDescSet,
            shadowRG, hdrRG, hdrRTV, vp, w, h);

        // Forward transparent pass: transparent objects -> HDR intermediate
        testScene.RenderForwardTransparent(renderer, transparentShadowDescSet,
            vp, hdrRG, hdrRTV, depthRG, depthDSV, shadowRG, w, h);

        // Post-processing pass: HDR -> back buffer (tone mapping + gamma)
        testScene.RenderPostProcess(renderer, postDescSet, hdrRG,
            bbRG, swapChain->GetCurrentBackBufferRTV(), w, h);

        renderer.EndFrame();
    }

    // ---- Shutdown (reverse order) ----
    EVO_LOG_INFO("Shutting down...");
    renderer.GetDevice()->WaitIdle();
    testScene.Shutdown(renderer.GetDevice());
    pDevice->FreeDescriptorSet(transparentShadowDescSet);
    pDevice->FreeDescriptorSet(postDescSet);
    pDevice->DestroyRenderTargetView(hdrRTV);
    pDevice->DestroyTexture(hdrTex);
    pDevice->FreeDescriptorSet(lightingDescSet);
    pDevice->DestroyDepthStencilView(shadowDSV);
    pDevice->DestroyTexture(shadowTex);
    pDevice->DestroyRenderTargetView(gbRoughMetRTV);
    pDevice->DestroyTexture(gbRoughMetTex);
    pDevice->DestroyRenderTargetView(gbNormalRTV);
    pDevice->DestroyTexture(gbNormalTex);
    pDevice->DestroyRenderTargetView(gbAlbedoRTV);
    pDevice->DestroyTexture(gbAlbedoTex);
    pDevice->DestroyDepthStencilView(depthDSV);
    pDevice->DestroyTexture(depthTex);
    renderer.Shutdown();
    window.Shutdown();
    Evo::Log::Shutdown();

    return 0;
}
