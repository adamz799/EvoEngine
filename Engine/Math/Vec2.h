#pragma once

#include "Math/MathCommon.h"

namespace Evo {

struct Vec2 {
	union {
		struct { float x, y; };
		float data[2];
	};

	// ---- Constructors ----
	Vec2() : x(0.0f), y(0.0f) {}
	explicit Vec2(float s) : x(s), y(s) {}
	Vec2(float x, float y) : x(x), y(y) {}
	Vec2(const DirectX::XMFLOAT2& v) : x(v.x), y(v.y) {}

	// ---- DirectXMath interop ----
	inline DirectX::XMVECTOR ToSIMD() const { return DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(data)); }
	inline static Vec2 FromSIMD(DirectX::XMVECTOR v) { Vec2 r; DirectX::XMStoreFloat2(reinterpret_cast<DirectX::XMFLOAT2*>(r.data), v); return r; }
	inline DirectX::XMFLOAT2 ToXM() const { return { x, y }; }

	// ---- Operators ----
	inline Vec2 operator+(const Vec2& rhs) const { return FromSIMD(DirectX::XMVectorAdd(ToSIMD(), rhs.ToSIMD())); }
	inline Vec2 operator-(const Vec2& rhs) const { return FromSIMD(DirectX::XMVectorSubtract(ToSIMD(), rhs.ToSIMD())); }
	inline Vec2 operator*(float s)         const { return FromSIMD(DirectX::XMVectorScale(ToSIMD(), s)); }
	inline Vec2 operator/(float s)         const { return FromSIMD(DirectX::XMVectorScale(ToSIMD(), 1.0f / s)); }
	inline Vec2 operator*(const Vec2& rhs) const { return FromSIMD(DirectX::XMVectorMultiply(ToSIMD(), rhs.ToSIMD())); }
	inline Vec2 operator-()                const { return FromSIMD(DirectX::XMVectorNegate(ToSIMD())); }

	inline Vec2& operator+=(const Vec2& rhs) { *this = *this + rhs; return *this; }
	inline Vec2& operator-=(const Vec2& rhs) { *this = *this - rhs; return *this; }
	inline Vec2& operator*=(float s)         { *this = *this * s;   return *this; }
	inline Vec2& operator/=(float s)         { *this = *this / s;   return *this; }

	inline float  operator[](int i) const { return data[i]; }
	inline float& operator[](int i)       { return data[i]; }

	inline bool operator==(const Vec2& rhs) const { return NearEqual(x, rhs.x) && NearEqual(y, rhs.y); }
	inline bool operator!=(const Vec2& rhs) const { return !(*this == rhs); }

	// ---- Methods ----
	inline float Length()   const { return DirectX::XMVectorGetX(DirectX::XMVector2Length(ToSIMD())); }
	inline float LengthSq() const { return DirectX::XMVectorGetX(DirectX::XMVector2LengthSq(ToSIMD())); }
	inline Vec2  Normalized() const { return FromSIMD(DirectX::XMVector2Normalize(ToSIMD())); }

	// ---- Static methods ----
	inline static float Dot(const Vec2& a, const Vec2& b) { return DirectX::XMVectorGetX(DirectX::XMVector2Dot(a.ToSIMD(), b.ToSIMD())); }
	inline static Vec2  Lerp(const Vec2& a, const Vec2& b, float t) { return FromSIMD(DirectX::XMVectorLerp(a.ToSIMD(), b.ToSIMD(), t)); }
	inline static Vec2  Min(const Vec2& a, const Vec2& b) { return FromSIMD(DirectX::XMVectorMin(a.ToSIMD(), b.ToSIMD())); }
	inline static Vec2  Max(const Vec2& a, const Vec2& b) { return FromSIMD(DirectX::XMVectorMax(a.ToSIMD(), b.ToSIMD())); }
	inline static float Distance(const Vec2& a, const Vec2& b) { return (a - b).Length(); }

	// ---- Constants ----
	static const Vec2 Zero;
	static const Vec2 One;
	static const Vec2 UnitX;
	static const Vec2 UnitY;
};

// ---- Constant definitions ----
inline const Vec2 Vec2::Zero  = Vec2(0.0f, 0.0f);
inline const Vec2 Vec2::One   = Vec2(1.0f, 1.0f);
inline const Vec2 Vec2::UnitX = Vec2(1.0f, 0.0f);
inline const Vec2 Vec2::UnitY = Vec2(0.0f, 1.0f);

// ---- Free operators ----
inline Vec2 operator*(float s, const Vec2& v) { return v * s; }

} // namespace Evo

