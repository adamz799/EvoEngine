#include "Core/Log.h"
#include "Math/Math.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"
#include "TriangleDemo.h"

// ---- D3D12 Agility SDK runtime selection ----
// These exports tell the D3D12 loader to use the Agility SDK DLLs
// from the ./D3D12/ subdirectory next to the executable.
#if EVO_RHI_DX12
extern "C" { __declspec(dllexport) extern const unsigned int D3D12SDKVersion = EVO_D3D12_AGILITY_SDK_VERSION; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
#endif

static void TestMathLibrary()
{
    using namespace Evo;

    // --- Vec3 ---
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);
    Vec3 sum = a + b;
    EVO_LOG_INFO("Vec3: ({},{},{}) + ({},{},{}) = ({},{},{})",
        a.x, a.y, a.z, b.x, b.y, b.z, sum.x, sum.y, sum.z);

    float dot = Vec3::Dot(a, b);
    EVO_LOG_INFO("Vec3: dot = {}", dot);   // 1*4 + 2*5 + 3*6 = 32

    Vec3 cross = Vec3::Cross(a, b);
    EVO_LOG_INFO("Vec3: cross = ({},{},{})", cross.x, cross.y, cross.z);  // (-3,6,-3)

    Vec3 n = a.Normalized();
    EVO_LOG_INFO("Vec3: normalized({},{},{}) len={:.4f}", n.x, n.y, n.z, n.Length());

    // --- Mat4 ---
    Mat4 trans = Mat4::Translation(Vec3(10.0f, 20.0f, 30.0f));
    Vec3 origin(0.0f, 0.0f, 0.0f);
    Vec3 moved = trans.TransformPoint(origin);
    EVO_LOG_INFO("Mat4: translate origin by (10,20,30) = ({},{},{})", moved.x, moved.y, moved.z);

    Mat4 proj = Mat4::PerspectiveFovLH(DegToRad(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
    EVO_LOG_INFO("Mat4: perspective[0][0]={:.4f} (focal length)", proj[0][0]);

    // --- Quat ---
    Quat q = Quat::FromAxisAngle(Vec3::UnitY, DegToRad(90.0f));
    Vec3 fwd(0.0f, 0.0f, 1.0f);
    Vec3 rotated = q.Rotate(fwd);
    EVO_LOG_INFO("Quat: rotate (0,0,1) by 90 deg around Y = ({:.2f},{:.2f},{:.2f})", rotated.x, rotated.y, rotated.z);

    // --- Color ---
    Color c = Color::CornflowerBlue;
    EVO_LOG_INFO("Color: CornflowerBlue = ({:.3f},{:.3f},{:.3f},{:.3f})", c.r, c.g, c.b, c.a);

    // --- Utilities ---
    EVO_LOG_INFO("MathCommon: DegToRad(180) = {:.4f} (should be {:.4f})", DegToRad(180.0f), Pi);
    EVO_LOG_INFO("MathCommon: Lerp(0, 10, 0.3) = {:.1f}", Lerp(0.0f, 10.0f, 0.3f));

    EVO_LOG_INFO("Math library tests passed!");
}

int main(int /*argc*/, char* /*argv*/[])
{
    // ---- Initialize logging ----
    Evo::Log::Initialize();
    EVO_LOG_INFO("EvoEngine starting...");

    // ---- Math library test ----
    TestMathLibrary();

    // ---- Create window ----
    Evo::Window window;
    Evo::WindowDesc winDesc{};
    winDesc.sTitle  = "EvoEngine";
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
    Evo::TriangleDemo triangleDemo;
    if (!triangleDemo.Initialize(renderer.GetDevice(), renderer.GetSwapChain()->GetFormat()))
    {
        EVO_LOG_ERROR("Failed to initialize triangle demo");
    }

    // ---- Main loop ----
    EVO_LOG_INFO("Entering main loop");
    while (window.PollEvents()) {
        if (window.IsMinimized())
            continue;

        renderer.BeginFrame();

        triangleDemo.AddPasses(renderer);

        renderer.EndFrame();
    }

    // ---- Shutdown (reverse order) ----
    EVO_LOG_INFO("Shutting down...");
    renderer.GetDevice()->WaitIdle();
    triangleDemo.Shutdown(renderer.GetDevice());
    renderer.Shutdown();
    window.Shutdown();
    Evo::Log::Shutdown();

    return 0;
}
