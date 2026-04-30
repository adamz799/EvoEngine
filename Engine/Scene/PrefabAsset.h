#pragma once

#include "Asset/Asset.h"
#include <string>

namespace Evo {

/// PrefabAsset — lightweight data asset describing an entity template.
/// Stores references to mesh and other resources (expandable via Prefab.fbs).
class PrefabAsset : public Asset {
public:
	const char* GetTypeName() const override { return "Prefab"; }

	bool OnLoad(const std::vector<uint8>& rawData) override;
	void OnUnload(RHIDevice* pDevice) override;

	const std::string& GetMeshPath() const { return m_sMeshPath; }

private:
	std::string m_sMeshPath;
};

} // namespace Evo

