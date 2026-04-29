#pragma once

#include "Scene/Scene.h"
#include "Scene/MeshAsset.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/Camera.h"
#include <memory>

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
	// GPU resources
	RHIShaderHandle   m_VS;
	RHIShaderHandle   m_PS;
	RHIPipelineHandle m_Pipeline;

	// Scene
	Scene                        m_Scene;
	std::unique_ptr<MeshAsset>   m_pCubeMesh;
	SceneRenderer                m_SceneRenderer;

	// Camera
	Camera               m_Camera;
	FreeCameraController m_CameraController;

	float m_fTime = 0.0f;
};

} // namespace Evo
