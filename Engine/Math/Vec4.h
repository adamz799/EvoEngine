#pragma once

#include "Math/MathCommon.h"

namespace Evo {

struct Vec4 {
	union {
		struct { float x, y, z, w; };
		float data[4];
	};

	// ---- Constructors ----
	Vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
	explicit Vec4(float s) : x(s), y(s), z(s), w(s) {}
	Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	Vec4(const DirectX::XMFLOAT4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

	// ---- DirectXMath interop ----
	inline DirectX::XMVECTOR ToSIMD() const { return DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(data)); }
	inline static Vec4 FromSIMD(DirectX::XMVECTOR v) { Vec4 r; DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(r.data), v); return r; }
	inline DirectX::XMFLOAT4 ToXM() const { return { x, y, z, w }; }

	// ---- Operators ----
	inline Vec4 operator+(const Vec4& rhs) const { return FromSIMD(DirectX::XMVectorAdd(ToSIMD(), rhs.ToSIMD())); }
	inline Vec4 operator-(const Vec4& rhs) const { return FromSIMD(DirectX::XMVectorSubtract(ToSIMD(), rhs.ToSIMD())); }
	inline Vec4 operator*(float s)         const { return FromSIMD(DirectX::XMVectorScale(ToSIMD(), s)); }
	inline Vec4 operator/(float s)         const { return FromSIMD(DirectX::XMVectorScale(ToSIMD(), 1.0f / s)); }
	inline Vec4 operator*(const Vec4& rhs) const { return FromSIMD(DirectX::XMVectorMultiply(ToSIMD(), rhs.ToSIMD())); }
	inline Vec4 operator-()                const { return FromSIMD(DirectX::XMVectorNegate(ToSIMD())); }

	inline Vec4& operator+=(const Vec4& rhs) { *this = *this + rhs; return *this; }
	inline Vec4& operator-=(const Vec4& rhs) { *this = *this - rhs; return *this; }
	inline Vec4& operator*=(float s)         { *this = *this * s;   return *this; }
	inline Vec4& operator/=(float s)         { *this = *this / s;   return *this; }

	inline float  operator[](int i) const { return data[i]; }
	inline float& operator[](int i)       { return data[i]; }

	inline bool operator==(const Vec4& rhs) const { return NearEqual(x, rhs.x) && NearEqual(y, rhs.y) && NearEqual(z, rhs.z) && NearEqual(w, rhs.w); }
	inline bool operator!=(const Vec4& rhs) const { return !(*this == rhs); }

	// ---- Methods ----
	inline float Length()    const { return DirectX::XMVectorGetX(DirectX::XMVector4Length(ToSIMD())); }
	inline float LengthSq()  const { return DirectX::XMVectorGetX(DirectX::XMVector4LengthSq(ToSIMD())); }
	inline Vec4  Normalized() const { return FromSIMD(DirectX::XMVector4Normalize(ToSIMD())); }

	// ---- Static methods ----
	inline static float Dot(const Vec4& a, const Vec4& b)  { return DirectX::XMVectorGetX(DirectX::XMVector4Dot(a.ToSIMD(), b.ToSIMD())); }
	inline static Vec4  Lerp(const Vec4& a, const Vec4& b, float t) { return FromSIMD(DirectX::XMVectorLerp(a.ToSIMD(), b.ToSIMD(), t)); }
	inline static Vec4  Min(const Vec4& a, const Vec4& b)  { return FromSIMD(DirectX::XMVectorMin(a.ToSIMD(), b.ToSIMD())); }
	inline static Vec4  Max(const Vec4& a, const Vec4& b)  { return FromSIMD(DirectX::XMVectorMax(a.ToSIMD(), b.ToSIMD())); }

	// ---- Constants ----
	static const Vec4 Zero;
	static const Vec4 One;
};

// ---- Constant definitions ----
inline const Vec4 Vec4::Zero = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
inline const Vec4 Vec4::One  = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

// ---- Free operators ----
inline Vec4 operator*(float s, const Vec4& v) { return v * s; }

} // namespace Evo

