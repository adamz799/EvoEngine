#include "Scene/Scene.h"

namespace Evo {

static const std::string s_EmptyName;

EntityHandle Scene::CreateEntity(const char* pName)
{
	EntityHandle handle;

	if (!m_vFreeList.empty())
	{
		handle.uIndex = m_vFreeList.back();
		m_vFreeList.pop_back();
		handle.uGeneration = m_vGenerations[handle.uIndex];
	}
	else
	{
		handle.uIndex = static_cast<uint32>(m_vGenerations.size());
		m_vGenerations.push_back(0);
		m_vAlive.push_back(false);
		m_vNames.emplace_back();
		m_vPrefabPaths.emplace_back();
		handle.uGeneration = 0;
	}

	m_vAlive[handle.uIndex] = true;
	m_vNames[handle.uIndex] = pName ? pName : "";
	m_uEntityCount++;
	return handle;
}

void Scene::DestroyEntity(EntityHandle entity)
{
	if (!IsAlive(entity))
		return;

	// Remove all components
	m_Transforms.Remove(entity);
	m_Meshes.Remove(entity);
	m_Materials.Remove(entity);

	// Clear name/prefab and mark dead
	m_vNames[entity.uIndex].clear();
	m_vPrefabPaths[entity.uIndex].clear();
	m_vAlive[entity.uIndex] = false;

	// Increment generation and add to free list
	m_vGenerations[entity.uIndex]++;
	m_vFreeList.push_back(entity.uIndex);
	m_uEntityCount--;
}

bool Scene::IsAlive(EntityHandle entity) const
{
	if (entity.uIndex >= m_vGenerations.size())
		return false;
	return m_vGenerations[entity.uIndex] == entity.uGeneration;
}

const std::string& Scene::GetEntityName(EntityHandle entity) const
{
	if (!IsAlive(entity))
		return s_EmptyName;
	return m_vNames[entity.uIndex];
}

void Scene::SetEntityName(EntityHandle entity, const std::string& sName)
{
	if (!IsAlive(entity))
		return;
	m_vNames[entity.uIndex] = sName;
}

const std::string& Scene::GetEntityPrefab(EntityHandle entity) const
{
	if (!IsAlive(entity))
		return s_EmptyName;
	return m_vPrefabPaths[entity.uIndex];
}

void Scene::SetEntityPrefab(EntityHandle entity, const std::string& sPath)
{
	if (!IsAlive(entity))
		return;
	m_vPrefabPaths[entity.uIndex] = sPath;
}

} // namespace Evo
