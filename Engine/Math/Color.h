#pragma once

#include "Math/Vec4.h"

namespace Evo {

struct Color {
    union {
        struct { float r, g, b, a; };
        float data[4];
    };

    // ---- Constructors ----
    Color() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
    explicit Color(const Vec4& v) : r(v.x), g(v.y), b(v.z), a(v.w) {}

    /// Construct from 0-255 integer values.
    static Color FromRGBA8(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
    {
        constexpr float inv = 1.0f / 255.0f;
        return Color(r * inv, g * inv, b * inv, a * inv);
    }

    /// Construct from 0xRRGGBBAA packed uint32.
    static Color FromHex(uint32_t hex)
    {
        return FromRGBA8(
            static_cast<uint8_t>((hex >> 24) & 0xFF),
            static_cast<uint8_t>((hex >> 16) & 0xFF),
            static_cast<uint8_t>((hex >> 8)  & 0xFF),
            static_cast<uint8_t>((hex)       & 0xFF)
        );
    }

    // ---- Conversion ----
    inline Vec4 ToVec4() const { return Vec4(r, g, b, a); }
    inline const float* Ptr() const { return data; }

    // ---- Operators ----
    inline Color operator*(float s)          const { return Color(r * s, g * s, b * s, a * s); }
    inline Color operator*(const Color& rhs) const { return Color(r * rhs.r, g * rhs.g, b * rhs.b, a * rhs.a); }
    inline Color operator+(const Color& rhs) const { return Color(r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a); }

    inline float  operator[](int i) const { return data[i]; }
    inline float& operator[](int i)       { return data[i]; }

    // ---- Static methods ----
    inline static Color Lerp(const Color& a, const Color& b, float t)
    {
        return Color(
            Evo::Lerp(a.r, b.r, t),
            Evo::Lerp(a.g, b.g, t),
            Evo::Lerp(a.b, b.b, t),
            Evo::Lerp(a.a, b.a, t)
        );
    }

    // ---- Constants ----
    static const Color White;
    static const Color Black;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color Yellow;
    static const Color Cyan;
    static const Color Magenta;
    static const Color CornflowerBlue;  // Classic DX clear color
    static const Color Transparent;
};

// ---- Constant definitions ----
inline const Color Color::White          = Color(1.0f, 1.0f, 1.0f, 1.0f);
inline const Color Color::Black          = Color(0.0f, 0.0f, 0.0f, 1.0f);
inline const Color Color::Red            = Color(1.0f, 0.0f, 0.0f, 1.0f);
inline const Color Color::Green          = Color(0.0f, 1.0f, 0.0f, 1.0f);
inline const Color Color::Blue           = Color(0.0f, 0.0f, 1.0f, 1.0f);
inline const Color Color::Yellow         = Color(1.0f, 1.0f, 0.0f, 1.0f);
inline const Color Color::Cyan           = Color(0.0f, 1.0f, 1.0f, 1.0f);
inline const Color Color::Magenta        = Color(1.0f, 0.0f, 1.0f, 1.0f);
inline const Color Color::CornflowerBlue = Color(0.392f, 0.584f, 0.929f, 1.0f);
inline const Color Color::Transparent    = Color(0.0f, 0.0f, 0.0f, 0.0f);

} // namespace Evo
