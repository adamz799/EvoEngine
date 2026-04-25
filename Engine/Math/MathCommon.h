#pragma once

#include <DirectXMath.h>
#include <cmath>
#include <algorithm>

namespace Evo {

// ---- Constants ----
constexpr float Pi        = 3.14159265358979323846f;
constexpr float TwoPi     = Pi * 2.0f;
constexpr float HalfPi    = Pi * 0.5f;
constexpr float InvPi     = 1.0f / Pi;
constexpr float Epsilon   = 1e-6f;
constexpr float Infinity  = std::numeric_limits<float>::infinity();

// ---- Angle conversion ----
inline constexpr float DegToRad(float degrees) { return degrees * (Pi / 180.0f); }
inline constexpr float RadToDeg(float radians) { return radians * (180.0f / Pi); }

// ---- Scalar utilities ----
template<typename T>
inline constexpr T Clamp(T value, T lo, T hi) { return (value < lo) ? lo : (value > hi) ? hi : value; }

inline constexpr float Saturate(float value) { return Clamp(value, 0.0f, 1.0f); }

inline constexpr float Lerp(float a, float b, float t) { return a + (b - a) * t; }

inline float InverseLerp(float a, float b, float value)
{
    float range = b - a;
    return (std::abs(range) > Epsilon) ? (value - a) / range : 0.0f;
}

inline float Remap(float value, float inMin, float inMax, float outMin, float outMax)
{
    float t = InverseLerp(inMin, inMax, value);
    return Lerp(outMin, outMax, t);
}

inline constexpr float Sign(float value) { return (value > 0.0f) ? 1.0f : (value < 0.0f) ? -1.0f : 0.0f; }

inline constexpr float Step(float edge, float x) { return x >= edge ? 1.0f : 0.0f; }

inline float SmoothStep(float edge0, float edge1, float x)
{
    float t = Saturate(InverseLerp(edge0, edge1, x));
    return t * t * (3.0f - 2.0f * t);
}

inline bool NearEqual(float a, float b, float epsilon = Epsilon)
{
    return std::abs(a - b) <= epsilon;
}

} // namespace Evo
