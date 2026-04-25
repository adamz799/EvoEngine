#pragma once

#include "Math/MathCommon.h"

namespace Evo {

struct Vec3 {
    union {
        struct { float x, y, z; };
        float data[3];
    };

    // ---- Constructors ----
    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    explicit Vec3(float s) : x(s), y(s), z(s) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vec3(const DirectX::XMFLOAT3& v) : x(v.x), y(v.y), z(v.z) {}

    // ---- DirectXMath interop ----
    inline DirectX::XMVECTOR ToSIMD() const { return DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(data)); }
    inline static Vec3 FromSIMD(DirectX::XMVECTOR v) { Vec3 r; DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(r.data), v); return r; }
    inline DirectX::XMFLOAT3 ToXM() const { return { x, y, z }; }

    // ---- Operators ----
    inline Vec3 operator+(const Vec3& rhs) const { return FromSIMD(DirectX::XMVectorAdd(ToSIMD(), rhs.ToSIMD())); }
    inline Vec3 operator-(const Vec3& rhs) const { return FromSIMD(DirectX::XMVectorSubtract(ToSIMD(), rhs.ToSIMD())); }
    inline Vec3 operator*(float s)         const { return FromSIMD(DirectX::XMVectorScale(ToSIMD(), s)); }
    inline Vec3 operator/(float s)         const { return FromSIMD(DirectX::XMVectorScale(ToSIMD(), 1.0f / s)); }
    inline Vec3 operator*(const Vec3& rhs) const { return FromSIMD(DirectX::XMVectorMultiply(ToSIMD(), rhs.ToSIMD())); }
    inline Vec3 operator-()                const { return FromSIMD(DirectX::XMVectorNegate(ToSIMD())); }

    inline Vec3& operator+=(const Vec3& rhs) { *this = *this + rhs; return *this; }
    inline Vec3& operator-=(const Vec3& rhs) { *this = *this - rhs; return *this; }
    inline Vec3& operator*=(float s)         { *this = *this * s;   return *this; }
    inline Vec3& operator/=(float s)         { *this = *this / s;   return *this; }

    inline float  operator[](int i) const { return data[i]; }
    inline float& operator[](int i)       { return data[i]; }

    inline bool operator==(const Vec3& rhs) const { return NearEqual(x, rhs.x) && NearEqual(y, rhs.y) && NearEqual(z, rhs.z); }
    inline bool operator!=(const Vec3& rhs) const { return !(*this == rhs); }

    // ---- Methods ----
    inline float Length()    const { return DirectX::XMVectorGetX(DirectX::XMVector3Length(ToSIMD())); }
    inline float LengthSq()  const { return DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(ToSIMD())); }
    inline Vec3  Normalized() const { return FromSIMD(DirectX::XMVector3Normalize(ToSIMD())); }

    // ---- Static methods ----
    inline static float Dot(const Vec3& a, const Vec3& b)   { return DirectX::XMVectorGetX(DirectX::XMVector3Dot(a.ToSIMD(), b.ToSIMD())); }
    inline static Vec3  Cross(const Vec3& a, const Vec3& b) { return FromSIMD(DirectX::XMVector3Cross(a.ToSIMD(), b.ToSIMD())); }
    inline static Vec3  Lerp(const Vec3& a, const Vec3& b, float t) { return FromSIMD(DirectX::XMVectorLerp(a.ToSIMD(), b.ToSIMD(), t)); }
    inline static Vec3  Min(const Vec3& a, const Vec3& b)   { return FromSIMD(DirectX::XMVectorMin(a.ToSIMD(), b.ToSIMD())); }
    inline static Vec3  Max(const Vec3& a, const Vec3& b)   { return FromSIMD(DirectX::XMVectorMax(a.ToSIMD(), b.ToSIMD())); }
    inline static float Distance(const Vec3& a, const Vec3& b) { return (a - b).Length(); }
    inline static Vec3  Reflect(const Vec3& incident, const Vec3& normal) { return FromSIMD(DirectX::XMVector3Reflect(incident.ToSIMD(), normal.ToSIMD())); }

    // ---- Constants ----
    static const Vec3 Zero;
    static const Vec3 One;
    static const Vec3 UnitX;
    static const Vec3 UnitY;
    static const Vec3 UnitZ;
    static const Vec3 Up;
    static const Vec3 Down;
    static const Vec3 Forward;
    static const Vec3 Right;
};

// ---- Constant definitions ----
inline const Vec3 Vec3::Zero    = Vec3(0.0f, 0.0f, 0.0f);
inline const Vec3 Vec3::One     = Vec3(1.0f, 1.0f, 1.0f);
inline const Vec3 Vec3::UnitX   = Vec3(1.0f, 0.0f, 0.0f);
inline const Vec3 Vec3::UnitY   = Vec3(0.0f, 1.0f, 0.0f);
inline const Vec3 Vec3::UnitZ   = Vec3(0.0f, 0.0f, 1.0f);
inline const Vec3 Vec3::Up      = Vec3(0.0f, 1.0f, 0.0f);
inline const Vec3 Vec3::Down    = Vec3(0.0f, -1.0f, 0.0f);
inline const Vec3 Vec3::Forward = Vec3(0.0f, 0.0f, 1.0f);   // Left-handed (DX convention)
inline const Vec3 Vec3::Right   = Vec3(1.0f, 0.0f, 0.0f);

// ---- Free operators ----
inline Vec3 operator*(float s, const Vec3& v) { return v * s; }

} // namespace Evo
