#pragma once

#include "Asset/AssetManager.h"
#include "Scene/Scene.h"
#include "Renderer/Camera.h"

namespace Evo {

/// TestScene -- five rotating cubes + two transparent cubes, with a fixed game camera.
/// Scene setup and update only; rendering is handled by RenderPipeline.
class TestScene {
public:
	bool Initialize(RHIDevice* pDevice);
	void Shutdown(RHIDevice* pDevice);

	/// Update cube rotations (no input, no camera controller).
	void Update(float fDeltaTime);

	Scene& GetScene() { return m_Scene; }
	Camera& GetGameCamera() { return m_GameCamera; }
	const Camera& GetGameCamera() const { return m_GameCamera; }

private:
	AssetManager  m_AssetManager;
	Scene         m_Scene;
	Camera        m_GameCamera;
	float         m_fTime = 0.0f;
};

} // namespace Evo
