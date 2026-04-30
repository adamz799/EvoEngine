#include "Scene/MaterialAsset.h"
#include "Core/Log.h"
#include "Material_generated.h"

namespace Evo {

bool MaterialAsset::OnLoad(const std::vector<uint8>& rawData)
{
	flatbuffers::Verifier verifier(rawData.data(), rawData.size());
	if (!Schema::VerifyMaterialBuffer(verifier))
	{
		EVO_LOG_ERROR("MaterialAsset: FlatBuffer verification failed for '{}'", m_sPath);
		return false;
	}

	auto* pMaterial = Schema::GetMaterial(rawData.data());

	if (auto* pAlbedo = pMaterial->albedo())
		m_vAlbedo = Vec3(pAlbedo->x(), pAlbedo->y(), pAlbedo->z());
	else
		m_vAlbedo = Vec3(0.8f, 0.8f, 0.8f);

	m_fRoughness = pMaterial->roughness();
	m_fMetallic  = pMaterial->metallic();
	m_fAlpha     = pMaterial->alpha();

	EVO_LOG_INFO("MaterialAsset: loaded '{}' (albedo=[{:.2f},{:.2f},{:.2f}] rough={:.2f} metal={:.2f} alpha={:.2f})",
				 m_sPath, m_vAlbedo.x, m_vAlbedo.y, m_vAlbedo.z, m_fRoughness, m_fMetallic, m_fAlpha);
	return true;
}

void MaterialAsset::OnUnload(RHIDevice* /*pDevice*/)
{
	m_vAlbedo    = Vec3(0.8f, 0.8f, 0.8f);
	m_fRoughness = 0.5f;
	m_fMetallic  = 0.0f;
	m_fAlpha     = 1.0f;
}

} // namespace Evo

