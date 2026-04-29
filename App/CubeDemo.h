#pragma once

#include "Asset/AssetManager.h"
#include "Scene/Scene.h"
#include "Scene/MeshAsset.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/Camera.h"

namespace Evo {

class Renderer;
class Input;
class Window;

/// CubeDemo — renders multiple cubes using the Scene/Mesh/SceneRenderer system.
class CubeDemo {
public:
	bool Initialize(RHIDevice* pDevice, RHIFormat rtFormat);
	void Shutdown(RHIDevice* pDevice);

	void Update(float fDeltaTime, const Input& input, Window& window);
	void Render(Renderer& renderer);

private:
	// Asset management
	AssetManager  m_AssetManager;
	AssetHandle   m_ShaderHandle;

	// GPU resources
	RHIPipelineHandle m_Pipeline;

	// Scene
	Scene                        m_Scene;
	SceneRenderer                m_SceneRenderer;

	// Camera
	Camera               m_Camera;
	FreeCameraController m_CameraController;

	float m_fTime = 0.0f;
};

} // namespace Evo
