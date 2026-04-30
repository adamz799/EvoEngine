#pragma once

#include "Math/Vec3.h"
#include "Math/Vec4.h"

namespace Evo {

struct Quat;  // forward declaration

struct Mat4 {
	union {
		float m[4][4];
		float data[16];
		struct {
			float m00, m01, m02, m03;
			float m10, m11, m12, m13;
			float m20, m21, m22, m23;
			float m30, m31, m32, m33;
		};
	};

	// ---- Constructors ----
	Mat4()
	{
		DirectX::XMFLOAT4X4 identity;
		DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
		std::memcpy(data, &identity, sizeof(float) * 16);
	}

	explicit Mat4(float diagonal)
	{
		std::memset(data, 0, sizeof(data));
		m[0][0] = m[1][1] = m[2][2] = m[3][3] = diagonal;
	}

	Mat4(const DirectX::XMFLOAT4X4& xm) { std::memcpy(data, &xm, sizeof(float) * 16); }
	Mat4(const DirectX::XMMATRIX& xm) { DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(data), xm); }

	// ---- DirectXMath interop ----
	inline DirectX::XMMATRIX ToSIMD() const { return DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(data)); }
	inline static Mat4 FromSIMD(DirectX::XMMATRIX xm) { Mat4 r; DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(r.data), xm); return r; }

	// ---- Operators ----
	inline Mat4 operator*(const Mat4& rhs) const { return FromSIMD(DirectX::XMMatrixMultiply(ToSIMD(), rhs.ToSIMD())); }
	inline Mat4& operator*=(const Mat4& rhs) { *this = *this * rhs; return *this; }

	inline Vec4 operator*(const Vec4& v) const
	{
		return Vec4::FromSIMD(DirectX::XMVector4Transform(v.ToSIMD(), ToSIMD()));
	}

	inline float* operator[](int row)       { return m[row]; }
	inline const float* operator[](int row) const { return m[row]; }

	// ---- Methods ----
	inline Mat4 Transposed() const { return FromSIMD(DirectX::XMMatrixTranspose(ToSIMD())); }

	inline Mat4 Inverse() const
	{
		DirectX::XMVECTOR det;
		return FromSIMD(DirectX::XMMatrixInverse(&det, ToSIMD()));
	}

	inline float Determinant() const
	{
		return DirectX::XMVectorGetX(DirectX::XMMatrixDeterminant(ToSIMD()));
	}

	/// Transform a point (w=1, applies translation).
	inline Vec3 TransformPoint(const Vec3& p) const
	{
		DirectX::XMVECTOR v = DirectX::XMVector3TransformCoord(p.ToSIMD(), ToSIMD());
		return Vec3::FromSIMD(v);
	}

	/// Transform a direction (w=0, ignores translation).
	inline Vec3 TransformDirection(const Vec3& d) const
	{
		DirectX::XMVECTOR v = DirectX::XMVector3TransformNormal(d.ToSIMD(), ToSIMD());
		return Vec3::FromSIMD(v);
	}

	/// Get a row as Vec4.
	inline Vec4 GetRow(int row) const { return Vec4(m[row][0], m[row][1], m[row][2], m[row][3]); }

	/// Get translation component (row 3, xyz).
	inline Vec3 GetTranslation() const { return Vec3(m[3][0], m[3][1], m[3][2]); }

	// ---- Static factory methods ----
	inline static Mat4 Identity() { return Mat4(1.0f); }

	inline static Mat4 Translation(const Vec3& t)
	{
		return FromSIMD(DirectX::XMMatrixTranslation(t.x, t.y, t.z));
	}

	inline static Mat4 Translation(float x, float y, float z)
	{
		return FromSIMD(DirectX::XMMatrixTranslation(x, y, z));
	}

	inline static Mat4 Scaling(const Vec3& s)
	{
		return FromSIMD(DirectX::XMMatrixScaling(s.x, s.y, s.z));
	}

	inline static Mat4 Scaling(float s)
	{
		return FromSIMD(DirectX::XMMatrixScaling(s, s, s));
	}

	inline static Mat4 RotationX(float radians) { return FromSIMD(DirectX::XMMatrixRotationX(radians)); }
	inline static Mat4 RotationY(float radians) { return FromSIMD(DirectX::XMMatrixRotationY(radians)); }
	inline static Mat4 RotationZ(float radians) { return FromSIMD(DirectX::XMMatrixRotationZ(radians)); }

	inline static Mat4 RotationAxis(const Vec3& axis, float radians)
	{
		return FromSIMD(DirectX::XMMatrixRotationAxis(axis.ToSIMD(), radians));
	}

	// RotationQuaternion is defined after Quat (see Quat.h)

	// ---- View / Projection (Left-Handed, DX convention) ----
	inline static Mat4 LookAtLH(const Vec3& eye, const Vec3& target, const Vec3& up)
	{
		return FromSIMD(DirectX::XMMatrixLookAtLH(eye.ToSIMD(), target.ToSIMD(), up.ToSIMD()));
	}

	inline static Mat4 PerspectiveFovLH(float fovY, float aspect, float nearZ, float farZ)
	{
		return FromSIMD(DirectX::XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ));
	}

	inline static Mat4 OrthographicLH(float width, float height, float nearZ, float farZ)
	{
		return FromSIMD(DirectX::XMMatrixOrthographicLH(width, height, nearZ, farZ));
	}

	// ---- View / Projection (Right-Handed, Vulkan/OpenGL convention) ----
	inline static Mat4 LookAtRH(const Vec3& eye, const Vec3& target, const Vec3& up)
	{
		return FromSIMD(DirectX::XMMatrixLookAtRH(eye.ToSIMD(), target.ToSIMD(), up.ToSIMD()));
	}

	inline static Mat4 PerspectiveFovRH(float fovY, float aspect, float nearZ, float farZ)
	{
		return FromSIMD(DirectX::XMMatrixPerspectiveFovRH(fovY, aspect, nearZ, farZ));
	}

	inline static Mat4 OrthographicRH(float width, float height, float nearZ, float farZ)
	{
		return FromSIMD(DirectX::XMMatrixOrthographicRH(width, height, nearZ, farZ));
	}
};

} // namespace Evo

