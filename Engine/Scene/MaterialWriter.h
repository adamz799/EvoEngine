#pragma once

#include "Math/Math.h"
#include <string>

namespace Evo {

/// Write a Material FlatBuffers .ematerial file.
bool WriteMaterial(const std::string& sPath, const Vec3& vAlbedo,
                   float fRoughness, float fMetallic, float fAlpha);

} // namespace Evo
