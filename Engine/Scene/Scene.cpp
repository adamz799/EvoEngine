#include "Scene/Scene.h"

namespace Evo {

EntityHandle Scene::CreateEntity(const char* /*pName*/)
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
		handle.uIndex = m_uEntityCount;
		m_vGenerations.push_back(0);
		handle.uGeneration = 0;
	}

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

} // namespace Evo
