#include "Scene/SceneLoader.h"
#include "Scene/Scene.h"
#include "Scene/MeshAsset.h"
#include "Scene/PrefabAsset.h"
#include "Asset/AssetManager.h"
#include "Platform/FileSystem.h"
#include "Core/Log.h"
#include "Scene_generated.h"

namespace Evo {

bool LoadScene(const std::string& sPath, Scene& scene, AssetManager& assetManager)
{
	auto rawData = FileSystem::ReadBinary(sPath);
	if (rawData.empty())
	{
		EVO_LOG_ERROR("LoadScene: failed to read '{}'", sPath);
		return false;
	}

	// Verify FlatBuffer
	flatbuffers::Verifier verifier(rawData.data(), rawData.size());
	if (!Schema::VerifySceneBuffer(verifier))
	{
		EVO_LOG_ERROR("LoadScene: FlatBuffer verification failed for '{}'", sPath);
		return false;
	}

	auto* pScene = Schema::GetScene(rawData.data());
	auto* pEntities = pScene->entities();
	if (!pEntities)
	{
		EVO_LOG_ERROR("LoadScene: no entities in '{}'", sPath);
		return false;
	}

	for (uint32 i = 0; i < pEntities->size(); ++i)
	{
		auto* pEntity = pEntities->Get(i);

		// Create entity
		const char* pName = pEntity->name() ? pEntity->name()->c_str() : nullptr;
		auto entity = scene.CreateEntity(pName);

		// Set transform
		TransformComponent transform;
		if (auto* pPos = pEntity->position())
			transform.vPosition = Vec3(pPos->x(), pPos->y(), pPos->z());
		if (auto* pRot = pEntity->rotation())
			transform.qRotation = Quat(pRot->x(), pRot->y(), pRot->z(), pRot->w());
		if (auto* pScl = pEntity->scale())
			transform.vScale = Vec3(pScl->x(), pScl->y(), pScl->z());
		scene.Transforms().Add(entity, transform);

		// Load prefab → resolve mesh
		if (pEntity->prefab_path() && pEntity->prefab_path()->size() > 0)
		{
			std::string sPrefabPath = pEntity->prefab_path()->str();
			scene.SetEntityPrefab(entity, sPrefabPath);

			auto prefabHandle = assetManager.LoadSync(sPrefabPath);
			auto* pPrefab = assetManager.Get<PrefabAsset>(prefabHandle);

			if (pPrefab && !pPrefab->GetMeshPath().empty())
			{
				auto meshHandle = assetManager.LoadSync(pPrefab->GetMeshPath());
				auto* pMeshAsset = assetManager.Get<MeshAsset>(meshHandle);

				if (pMeshAsset)
				{
					MeshComponent mesh;
					mesh.pMesh     = pMeshAsset;
					mesh.uLODIndex = pEntity->lod_index();
					mesh.bVisible  = pEntity->visible();
					scene.Meshes().Add(entity, mesh);
				}
				else
				{
					EVO_LOG_WARN("LoadScene: failed to load mesh '{}' from prefab '{}' for entity '{}'",
					             pPrefab->GetMeshPath(), sPrefabPath, pName ? pName : "<unnamed>");
				}
			}
			else if (!pPrefab)
			{
				EVO_LOG_WARN("LoadScene: failed to load prefab '{}' for entity '{}'",
				             sPrefabPath, pName ? pName : "<unnamed>");
			}
		}
	}

	EVO_LOG_INFO("LoadScene: loaded '{}' ({} entities)", sPath, pEntities->size());
	return true;
}

} // namespace Evo
