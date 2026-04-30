#pragma once

#include "Scene/Entity.h"
#include "Scene/Components.h"
#include <vector>
#include <string>

namespace Evo {

/// Scene — manages entities and their components.
class Scene {
public:
	EntityHandle CreateEntity(const char* pName = nullptr);
	void         DestroyEntity(EntityHandle entity);
	bool         IsAlive(EntityHandle entity) const;

	const std::string& GetEntityName(EntityHandle entity) const;
	void               SetEntityName(EntityHandle entity, const std::string& sName);

	const std::string& GetEntityPrefab(EntityHandle entity) const;
	void               SetEntityPrefab(EntityHandle entity, const std::string& sPath);

	const std::string& GetEntityMaterial(EntityHandle entity) const;
	void               SetEntityMaterial(EntityHandle entity, const std::string& sPath);

	uint32 GetEntityCount() const { return m_uEntityCount; }

	/// Iterate all alive entities. Fn signature: void(EntityHandle)
	template<typename Fn>
	void ForEachEntity(Fn&& fn) const
	{
		for (uint32 i = 0; i < static_cast<uint32>(m_vAlive.size()); ++i)
		{
			if (!m_vAlive[i]) continue;
			EntityHandle handle;
			handle.uIndex      = i;
			handle.uGeneration = m_vGenerations[i];
			fn(handle);
		}
	}

	ComponentArray<TransformComponent>& Transforms() { return m_Transforms; }
	ComponentArray<MeshComponent>&      Meshes()     { return m_Meshes; }
	ComponentArray<MaterialComponent>&  Materials()  { return m_Materials; }
	ComponentArray<CameraComponent>&    Cameras()    { return m_Cameras; }

	const ComponentArray<TransformComponent>& Transforms() const { return m_Transforms; }
	const ComponentArray<MeshComponent>&      Meshes()     const { return m_Meshes; }
	const ComponentArray<MaterialComponent>&  Materials()  const { return m_Materials; }
	const ComponentArray<CameraComponent>&    Cameras()    const { return m_Cameras; }

private:
	std::vector<uint16>      m_vGenerations;
	std::vector<bool>        m_vAlive;
	std::vector<std::string> m_vNames;
	std::vector<std::string> m_vPrefabPaths;
	std::vector<std::string> m_vMaterialPaths;
	std::vector<uint32>      m_vFreeList;
	uint32                   m_uEntityCount = 0;

	ComponentArray<TransformComponent> m_Transforms;
	ComponentArray<MeshComponent>      m_Meshes;
	ComponentArray<MaterialComponent>  m_Materials;
	ComponentArray<CameraComponent>    m_Cameras;
};

} // namespace Evo
