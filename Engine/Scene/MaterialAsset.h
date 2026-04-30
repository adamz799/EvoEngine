#pragma once

#include "Asset/Asset.h"
#include "Math/Math.h"
#include <string>

namespace Evo {

/// MaterialAsset — PBR material properties loaded from a .ematerial FlatBuffer file.
class MaterialAsset : public Asset {
public:
	const char* GetTypeName() const override { return "Material"; }

	bool OnLoad(const std::vector<uint8>& rawData) override;
	void OnUnload(RHIDevice* pDevice) override;

	Vec3  GetAlbedo()    const { return m_vAlbedo; }
	float GetRoughness() const { return m_fRoughness; }
	float GetMetallic()  const { return m_fMetallic; }
	float GetAlpha()     const { return m_fAlpha; }

private:
	Vec3  m_vAlbedo    = Vec3(0.8f, 0.8f, 0.8f);
	float m_fRoughness = 0.5f;
	float m_fMetallic  = 0.0f;
	float m_fAlpha     = 1.0f;
};

} // namespace Evo

