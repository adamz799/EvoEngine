#pragma once

#include "Asset/AssetManager.h"
#include "Scene/Scene.h"
#include "Renderer/Camera.h"
#include <memory>

namespace Evo {

class Render;

/// TestScene -- five rotating cubes + two transparent cubes, with a fixed game camera entity.
/// Scene setup and update only; rendering is handled by RenderPipeline.
class TestScene {
public:
	bool Initialize(Render* pRender);
	void Shutdown(Render* pRender);

	/// Update cube rotations (no input, no camera controller).
	void Update(float fDeltaTime);

	Scene* GetScene() { return m_pScene.get(); }
	EntityHandle GetGameCameraEntity() const { return m_GameCameraEntity; }

private:
	AssetManager              m_AssetManager;
	std::unique_ptr<Scene>    m_pScene;
	EntityHandle              m_GameCameraEntity;
	float                     m_fTime = 0.0f;
};

} // namespace Evo

