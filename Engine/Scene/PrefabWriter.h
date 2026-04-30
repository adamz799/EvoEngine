#pragma once

#include <string>

namespace Evo {

/// Write a Prefab FlatBuffers .eprefab file.
bool WritePrefab(const std::string& sPath, const std::string& sMeshPath);

} // namespace Evo

