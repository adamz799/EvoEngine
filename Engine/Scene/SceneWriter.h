#pragma once

#include <string>

namespace Evo {

class Scene;

/// Serialize a Scene to a FlatBuffers .escene file.
bool WriteScene(const std::string& sPath, Scene* pScene);

} // namespace Evo

