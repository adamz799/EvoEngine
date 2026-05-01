#pragma once

#include "Math/Vec3.h"
#include <string>

namespace Evo {

/// Editor asset creation — callable from editor UI actions.
/// Wraps the Engine/Scene writer functions behind a clean API.
class EditorAssetFactory {
public:
	/// Create a cube mesh asset.
	static bool CreateCubeMesh(const std::string& sPath);

	/// Create a prefab referencing a mesh.
	static bool CreatePrefab(const std::string& sPath, const std::string& sMeshPath);

	/// Create a material asset.
	static bool CreateMaterial(const std::string& sPath,
							   const Vec3& vAlbedo, float fRoughness,
							   float fMetallic, float fAlpha = 1.0f);

	/// Create a scene with cube entities positioned at given locations.
	static bool CreateCubeScene(const std::string& sPath,
								const Vec3* pPositions, const Vec3* pScales,
								const std::string* pMaterialNames,
								int nCount,
								const std::string& sPrefabPath,
								const std::string& sMaterialDir);

	/// Create the demo test scene (5 opaque + 2 transparent cubes + camera).
	/// Idempotent — skips if assets already exist. Call before loading the scene.
	static void EnsureDemoAssets();
};

} // namespace Evo
