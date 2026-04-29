#pragma once

#include "Asset/AssetManager.h"
#include "Scene/Scene.h"
#include "Scene/MeshAsset.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/Camera.h"

namespace Evo {

class Renderer;

/// TestScene -- five rotating cubes, with a fixed game camera.
class TestScene {
public:
	bool Initialize(RHIDevice* pDevice, RHIFormat rtFormat);
	void Shutdown(RHIDevice* pDevice);

	/// Update cube rotations (no input, no camera controller).
	void Update(float fDeltaTime);

	/// Render the scene from an externally-supplied view-projection matrix.
	void Render(Renderer& renderer,
	            RGHandle targetTexture, RHIRenderTargetView targetRTV,
	            const Mat4& viewProj,
	            float fViewportWidth, float fViewportHeight);

	Scene& GetScene() { return m_Scene; }
	Camera& GetGameCamera() { return m_GameCamera; }
	const Camera& GetGameCamera() const { return m_GameCamera; }

private:
	// Asset management
	AssetManager  m_AssetManager;
	AssetHandle   m_ShaderHandle;

	// GPU resources
	RHIPipelineHandle m_Pipeline;

	// Scene
	Scene         m_Scene;
	SceneRenderer m_SceneRenderer;

	// Game camera (fixed viewpoint, no controller)
	Camera m_GameCamera;

	float m_fTime = 0.0f;
};

} // namespace Evo
