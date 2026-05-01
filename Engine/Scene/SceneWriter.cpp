#include "Scene/SceneWriter.h"
#include "Scene/Scene.h"
#include "Platform/FileSystem.h"
#include "Core/Log.h"
#include "Scene_generated.h"
#include <filesystem>

namespace Evo {

bool WriteScene(const std::string& sPath, Scene* pScene)
{
	flatbuffers::FlatBufferBuilder builder(1024);

	std::vector<flatbuffers::Offset<Schema::SceneEntity>> entityOffsets;

	pScene->ForEachEntity([&](EntityHandle entity) {
		const std::string& sName = pScene->GetEntityName(entity);
		auto nameOffset = builder.CreateString(sName);

		// Transform
		const Schema::Vec3* pPosition = nullptr;
		const Schema::Vec4* pRotation = nullptr;
		const Schema::Vec3* pScale    = nullptr;
		Schema::Vec3 position(0, 0, 0);
		Schema::Vec4 rotation(0, 0, 0, 1);
		Schema::Vec3 scale(1, 1, 1);

		auto* pTransform = pScene->Transforms().Get(entity);
		if (pTransform)
		{
			position = Schema::Vec3(pTransform->vPosition.x, pTransform->vPosition.y, pTransform->vPosition.z);
			rotation = Schema::Vec4(pTransform->qRotation.x, pTransform->qRotation.y,
									pTransform->qRotation.z, pTransform->qRotation.w);
			scale    = Schema::Vec3(pTransform->vScale.x, pTransform->vScale.y, pTransform->vScale.z);
		}
		pPosition = &position;
		pRotation = &rotation;
		pScale    = &scale;

		// Prefab reference
		flatbuffers::Offset<flatbuffers::String> prefabPathOffset;
		const std::string& sPrefabPath = pScene->GetEntityPrefab(entity);
		if (!sPrefabPath.empty())
			prefabPathOffset = builder.CreateString(sPrefabPath);

		// Material reference
		flatbuffers::Offset<flatbuffers::String> materialPathOffset;
		const std::string& sMaterialPath = pScene->GetEntityMaterial(entity);
		if (!sMaterialPath.empty())
			materialPathOffset = builder.CreateString(sMaterialPath);

		// Per-instance rendering properties
		uint32 uLODIndex = 0;
		bool bVisible = true;
		auto* pMesh = pScene->Meshes().Get(entity);
		if (pMesh)
		{
			uLODIndex = pMesh->uLODIndex;
			bVisible  = pMesh->bVisible;
		}

		Schema::SceneEntityBuilder entityBuilder(builder);
		entityBuilder.add_name(nameOffset);
		entityBuilder.add_position(pPosition);
		entityBuilder.add_rotation(pRotation);
		entityBuilder.add_scale(pScale);
		if (!prefabPathOffset.IsNull())
			entityBuilder.add_prefab_path(prefabPathOffset);
		entityBuilder.add_lod_index(uLODIndex);
		entityBuilder.add_visible(bVisible);
		if (!materialPathOffset.IsNull())
			entityBuilder.add_material_path(materialPathOffset);

		entityOffsets.push_back(entityBuilder.Finish());
	});

	auto entitiesVec = builder.CreateVector(entityOffsets);
	auto root = Schema::CreateScene(builder, entitiesVec);
	Schema::FinishSceneBuffer(builder, root);

	// Write to disk
	std::filesystem::create_directories(std::filesystem::path(sPath).parent_path());
	auto data = std::span<const uint8>(builder.GetBufferPointer(), builder.GetSize());
	if (!FileSystem::WriteBinary(sPath, data))
	{
		EVO_LOG_ERROR("WriteScene: failed to write '{}'", sPath);
		return false;
	}

	EVO_LOG_INFO("WriteScene: wrote '{}' ({} bytes, {} entities)", sPath, builder.GetSize(), entityOffsets.size());
	return true;
}

} // namespace Evo

