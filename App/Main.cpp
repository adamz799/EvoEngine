#include "Core/Engine.h"
#include "Core/EngineConfig.h"
#include "Core/Log.h"
#include "Math/Math.h"
#include "Renderer/Camera.h"
#include "TestScene.h"

#if EVO_RHI_DX12
extern "C" { __declspec(dllexport) extern const unsigned int D3D12SDKVersion = EVO_D3D12_AGILITY_SDK_VERSION; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
#endif

int main()
{
	Evo::Log::Initialize();
	Evo::EngineConfig::LoadFromFile("Assets/engine_config.json");

	Evo::Engine EvoEngine;
	if (!EvoEngine.Launch())
		return -1;

	// ---- Setup test scene data (assets, entities) ----
	Evo::TestScene testScene;
	testScene.Initialize(EvoEngine.GetRender());
	EvoEngine.SetScene(&testScene.GetScene());

	// ---- Main loop ----
	while (EvoEngine.BeginFrame()) {
		float dt = EvoEngine.GetDeltaTime();
		testScene.Update(dt);
		EvoEngine.RenderFrame();
		EvoEngine.EndFrame();
	}

	testScene.Shutdown(EvoEngine.GetRender());
	EvoEngine.Shutdown();
	return 0;
}
