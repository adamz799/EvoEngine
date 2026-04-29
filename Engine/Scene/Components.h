#pragma once

#include "Math/Math.h"

namespace Evo {

class MeshAsset;

/// Transform component — position, rotation, scale → world matrix.
struct TransformComponent {
	Vec3 vPosition = Vec3::Zero;
	Quat qRotation = Quat::Identity();
	Vec3 vScale    = Vec3::One;

	Mat4 GetWorldMatrix() const
	{
		return Mat4::Scaling(vScale)
		     * qRotation.ToMatrix()
		     * Mat4::Translation(vPosition);
	}
};

/// Mesh component — references a MeshAsset for rendering.
struct MeshComponent {
	MeshAsset* pMesh     = nullptr;
	uint32     uLODIndex = 0;
	bool       bVisible  = true;
};

} // namespace Evo
