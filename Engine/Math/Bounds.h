#pragma once

#include "Core/Types.h"
#include "Math/Vec3.h"

namespace Evo {

/// Axis-Aligned Bounding Box.
struct AABB {
	Vec3 vMin;
	Vec3 vMax;

	AABB() : vMin(Infinity), vMax(-Infinity) {}
	AABB(const Vec3& min, const Vec3& max) : vMin(min), vMax(max) {}

	Vec3 GetCenter()  const { return (vMin + vMax) * 0.5f; }
	Vec3 GetExtents() const { return (vMax - vMin) * 0.5f; }
	Vec3 GetSize()    const { return vMax - vMin; }

	bool IsValid() const { return vMin.x <= vMax.x && vMin.y <= vMax.y && vMin.z <= vMax.z; }

	/// Expand this AABB to contain a point.
	void ExpandToInclude(const Vec3& point)
	{
		vMin = Vec3::Min(vMin, point);
		vMax = Vec3::Max(vMax, point);
	}

	/// Expand this AABB to contain another AABB.
	void ExpandToInclude(const AABB& other)
	{
		vMin = Vec3::Min(vMin, other.vMin);
		vMax = Vec3::Max(vMax, other.vMax);
	}

	static AABB FromMinMax(const Vec3& min, const Vec3& max) { return AABB(min, max); }

	static AABB FromPoints(const Vec3* pPoints, uint32 uCount)
	{
		AABB result;
		for (uint32 i = 0; i < uCount; ++i)
			result.ExpandToInclude(pPoints[i]);
		return result;
	}
};

/// Bounding sphere.
struct BoundingSphere {
	Vec3  vCenter;
	float fRadius = 0.0f;

	BoundingSphere() = default;
	BoundingSphere(const Vec3& center, float radius) : vCenter(center), fRadius(radius) {}

	static BoundingSphere FromAABB(const AABB& aabb)
	{
		Vec3 center = aabb.GetCenter();
		float radius = Vec3::Distance(center, aabb.vMax);
		return BoundingSphere(center, radius);
	}

	static BoundingSphere FromPoints(const Vec3* pPoints, uint32 uCount)
	{
		if (uCount == 0) return {};

		// Simple bounding sphere: compute AABB center, then max distance
		AABB aabb = AABB::FromPoints(pPoints, uCount);
		Vec3 center = aabb.GetCenter();
		float maxDistSq = 0.0f;
		for (uint32 i = 0; i < uCount; ++i)
		{
			float distSq = (pPoints[i] - center).LengthSq();
			if (distSq > maxDistSq)
				maxDistSq = distSq;
		}
		return BoundingSphere(center, std::sqrt(maxDistSq));
	}
};

} // namespace Evo

