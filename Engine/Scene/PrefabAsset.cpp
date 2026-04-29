#include "Scene/PrefabAsset.h"
#include "Core/Log.h"
#include "Prefab_generated.h"

namespace Evo {

bool PrefabAsset::OnLoad(const std::vector<uint8>& rawData)
{
	flatbuffers::Verifier verifier(rawData.data(), rawData.size());
	if (!Schema::VerifyPrefabBuffer(verifier))
	{
		EVO_LOG_ERROR("PrefabAsset: FlatBuffer verification failed for '{}'", m_sPath);
		return false;
	}

	auto* pPrefab = Schema::GetPrefab(rawData.data());

	if (pPrefab->mesh_path())
		m_sMeshPath = pPrefab->mesh_path()->str();
	else
		m_sMeshPath.clear();

	EVO_LOG_INFO("PrefabAsset: loaded '{}' (mesh='{}')", m_sPath, m_sMeshPath);
	return true;
}

void PrefabAsset::OnUnload(RHIDevice* /*pDevice*/)
{
	m_sMeshPath.clear();
}

} // namespace Evo
