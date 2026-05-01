#include "TestScene.h"
#include "Renderer/Renderer.h"
#include "Asset/ShaderAsset.h"
#include "Asset/TextureAsset.h"
#include "Scene/MeshAsset.h"
#include "Scene/MaterialAsset.h"
#include "Scene/PrefabAsset.h"
#include "Scene/SceneLoader.h"
#include "Core/Log.h"

#include <cmath>

namespace Evo {

bool TestScene::Initialize(Render* pRender)
{
	m_pScene = std::make_unique<Scene>();
	auto* pDevice = pRender->GetDevice();
#if EVO_RHI_DX12
	// ---- Initialize asset manager ----
	m_AssetManager.Initialize(pDevice);
	m_AssetManager.RegisterFactory(".emesh",     [] { return std::make_unique<MeshAsset>(); });
	m_AssetManager.RegisterFactory(".eprefab",   [] { return std::make_unique<PrefabAsset>(); });
	m_AssetManager.RegisterFactory(".ematerial", [] { return std::make_unique<MaterialAsset>(); });
	m_AssetManager.RegisterFactory(".png",       [] { return std::make_unique<TextureAsset>(); });
	m_AssetManager.RegisterFactory(".jpg",       [] { return std::make_unique<TextureAsset>(); });
	m_AssetManager.RegisterFactory(".tga",       [] { return std::make_unique<TextureAsset>(); });

	// ---- Load scene from .escene file ----
	if (!LoadScene("Assets/Scenes/CubeScene.escene", m_pScene.get(), m_AssetManager))
	{
		EVO_LOG_ERROR("TestScene: failed to load CubeScene.escene");
		return false;
	}

	// Find the game camera entity (created by EditorAssetFactory)
	m_GameCameraEntity = m_pScene->FindEntity("GameCamera");
	if (!m_GameCameraEntity.IsValid())
	{
		// Fallback: use first camera found in the scene
		auto cameras = m_pScene->GetCameraEntities();
		if (!cameras.empty())
			m_GameCameraEntity = cameras[0];
	}

	EVO_LOG_INFO("TestScene initialized: {} entities, camera={}",
		m_pScene->GetEntityCount(),
		m_GameCameraEntity.IsValid() ? "found" : "missing");
	return true;
#else
	return false;
#endif
}

void TestScene::Shutdown(Render* /*pRender*/)
{
	m_AssetManager.Shutdown();
}

void TestScene::Update(float fDeltaTime)
{
	m_fTime += fDeltaTime;
	m_AssetManager.Update();

	int index = 0;
	m_pScene->Transforms().ForEach([&](EntityHandle entity, TransformComponent& transform) {
		if (m_pScene->Cameras().Has(entity)) return;
		float speed = 0.5f + index * 0.3f;
		transform.qRotation = Quat::FromEuler(
			m_fTime * speed * 0.7f,
			m_fTime * speed,
			m_fTime * speed * 0.3f);
		index++;
	});
}

} // namespace Evo
