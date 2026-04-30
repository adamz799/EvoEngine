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

/// Material component — per-entity material properties for deferred rendering.
struct MaterialComponent {
	Vec3  vAlbedoColor = Vec3(0.8f, 0.8f, 0.8f);
	float fRoughness   = 0.5f;
	float fMetallic    = 0.0f;
	float fAlpha       = 1.0f;
};

/// Camera component — perspective projection parameters for a camera entity.
struct CameraComponent {
	float fFovY  = DegToRad(60.0f);
	float fNearZ = 0.1f;
	float fFarZ  = 100.0f;
};

} // namespace Evo

