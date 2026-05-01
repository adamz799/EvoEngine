#pragma once

#include <string>

namespace Evo {

class Scene;
class AssetManager;

/// Load a FlatBuffers .escene file into a Scene, resolving mesh references via AssetManager.
bool LoadScene(const std::string& sPath, Scene* pScene, AssetManager& assetManager);

} // namespace Evo

