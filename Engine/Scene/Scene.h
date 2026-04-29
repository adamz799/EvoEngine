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

	ComponentArray<TransformComponent>& Transforms() { return m_Transforms; }
	ComponentArray<MeshComponent>&      Meshes()     { return m_Meshes; }

	const ComponentArray<TransformComponent>& Transforms() const { return m_Transforms; }
	const ComponentArray<MeshComponent>&      Meshes()     const { return m_Meshes; }

private:
	std::vector<uint16> m_vGenerations;
	std::vector<uint32> m_vFreeList;
	uint32              m_uEntityCount = 0;

	ComponentArray<TransformComponent> m_Transforms;
	ComponentArray<MeshComponent>      m_Meshes;
};

} // namespace Evo
