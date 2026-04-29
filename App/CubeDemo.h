#pragma once

#include "Scene/Scene.h"
#include "Scene/MeshAsset.h"
#include "Renderer/SceneRenderer.h"
#include <memory>

namespace Evo {

class Renderer;

/// CubeDemo — renders multiple cubes using the Scene/Mesh/SceneRenderer system.
class CubeDemo {
public:
	bool Initialize(RHIDevice* pDevice, RHIFormat rtFormat);
	void Shutdown(RHIDevice* pDevice);

	void Update(float fDeltaTime);
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

	float m_fTime = 0.0f;
};

} // namespace Evo
