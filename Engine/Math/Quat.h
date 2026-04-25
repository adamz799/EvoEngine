#pragma once

#include "Math/Vec3.h"
#include "Math/Mat4.h"

namespace Evo {

struct Quat {
    union {
        struct { float x, y, z, w; };
        float data[4];
    };

    // ---- Constructors ----
    Quat() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}  // Identity
    Quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Quat(const DirectX::XMFLOAT4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

    // ---- DirectXMath interop ----
    inline DirectX::XMVECTOR ToSIMD() const { return DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(data)); }
    inline static Quat FromSIMD(DirectX::XMVECTOR v) { Quat r; DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(r.data), v); return r; }

    // ---- Operators ----
    /// Quaternion multiplication (combines rotations: result = this THEN rhs).
    inline Quat operator*(const Quat& rhs) const
    {
        return FromSIMD(DirectX::XMQuaternionMultiply(ToSIMD(), rhs.ToSIMD()));
    }

    inline Quat& operator*=(const Quat& rhs) { *this = *this * rhs; return *this; }

    inline bool operator==(const Quat& rhs) const
    {
        return NearEqual(x, rhs.x) && NearEqual(y, rhs.y) && NearEqual(z, rhs.z) && NearEqual(w, rhs.w);
    }
    inline bool operator!=(const Quat& rhs) const { return !(*this == rhs); }

    // ---- Methods ----
    inline float Length()    const { return DirectX::XMVectorGetX(DirectX::XMQuaternionLength(ToSIMD())); }
    inline Quat  Normalized() const { return FromSIMD(DirectX::XMQuaternionNormalize(ToSIMD())); }
    inline Quat  Conjugate()  const { return FromSIMD(DirectX::XMQuaternionConjugate(ToSIMD())); }
    inline Quat  Inverse()    const { return FromSIMD(DirectX::XMQuaternionInverse(ToSIMD())); }

    /// Rotate a vector by this quaternion.
    inline Vec3 Rotate(const Vec3& v) const
    {
        return Vec3::FromSIMD(DirectX::XMVector3Rotate(v.ToSIMD(), ToSIMD()));
    }

    /// Convert to 4x4 rotation matrix.
    inline Mat4 ToMatrix() const
    {
        return Mat4::FromSIMD(DirectX::XMMatrixRotationQuaternion(ToSIMD()));
    }

    /// Extract Euler angles (pitch, yaw, roll) in radians.
    inline Vec3 ToEuler() const
    {
        // Extract from rotation matrix to avoid gimbal-lock edge cases
        Mat4 mat = ToMatrix();

        float pitch, yaw, roll;
        float sp = -mat.m[2][1];

        if (std::abs(sp) >= 1.0f) {
            pitch = std::copysign(HalfPi, sp);
            yaw   = std::atan2(-mat.m[0][2], mat.m[0][0]);
            roll  = 0.0f;
        } else {
            pitch = std::asin(sp);
            yaw   = std::atan2(mat.m[2][0], mat.m[2][2]);
            roll  = std::atan2(mat.m[0][1], mat.m[1][1]);
        }
        return Vec3(pitch, yaw, roll);
    }

    // ---- Static factory methods ----
    inline static Quat Identity() { return Quat(0.0f, 0.0f, 0.0f, 1.0f); }

    inline static Quat FromAxisAngle(const Vec3& axis, float radians)
    {
        return FromSIMD(DirectX::XMQuaternionRotationAxis(axis.ToSIMD(), radians));
    }

    /// Create from Euler angles (pitch, yaw, roll) in radians.
    inline static Quat FromEuler(float pitch, float yaw, float roll)
    {
        return FromSIMD(DirectX::XMQuaternionRotationRollPitchYaw(pitch, yaw, roll));
    }

    inline static Quat FromEuler(const Vec3& euler)
    {
        return FromEuler(euler.x, euler.y, euler.z);
    }

    inline static Quat FromMatrix(const Mat4& mat)
    {
        return FromSIMD(DirectX::XMQuaternionRotationMatrix(mat.ToSIMD()));
    }

    // ---- Interpolation ----
    inline static Quat Lerp(const Quat& a, const Quat& b, float t)
    {
        return FromSIMD(DirectX::XMQuaternionSlerp(a.ToSIMD(), b.ToSIMD(), t));
    }

    inline static Quat Slerp(const Quat& a, const Quat& b, float t)
    {
        return FromSIMD(DirectX::XMQuaternionSlerp(a.ToSIMD(), b.ToSIMD(), t));
    }

    inline static float Dot(const Quat& a, const Quat& b)
    {
        return DirectX::XMVectorGetX(DirectX::XMQuaternionDot(a.ToSIMD(), b.ToSIMD()));
    }
};

// ---- Mat4 helper that depends on Quat ----
inline Mat4 Mat4RotationQuaternion(const Quat& q)
{
    return Mat4::FromSIMD(DirectX::XMMatrixRotationQuaternion(q.ToSIMD()));
}

} // namespace Evo
