//=====================================================================
// OpenGLCppWrapper: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenGLCppWrapper is distributedin the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#ifndef OPENGLCPPWRAPPER_COLOR_HPP
#define OPENGLCPPWRAPPER_COLOR_HPP

#include "Math/Random.hpp"
#include "Math/Vector.hpp"
#include <cstdint>
#include <ostream>
#include <utility>

// *****************************************************************************
//! \brief
// *****************************************************************************
class Color // FIXME should be compatible with Vector4f
{
public:

    inline friend std::ostream& operator<<(std::ostream& os, Color const& c)
    {
        os << "Color(" << c.r << ", " << c.g << ", " << c.b << ", " << c.a
           << ")";
        return os;
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    Color()
    {
        fromABGR(dist::integer(0u, 0xFFFFFFu));
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    template <typename T>
    explicit Color(Vector<T, 4> const& v) : Color(v[0], v[1], v[2], v[3])
    {
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    template <typename T>
    explicit Color(Vector<T, 3> const& v)
        : Color(v[0], v[1], v[2], maths::one<T>())
    {
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    Color(Color const& other) : r(other.r), g(other.g), b(other.b), a(other.a)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief
    //! \note value ranges shall be [0 .. 1]. They are clamped.
    //--------------------------------------------------------------------------
    explicit Color(float const red,
                   float const green,
                   float const blue,
                   float const alpha = 1.0f)
    {
        r = maths::clamp(red, 0.0f, 1.0f);
        g = maths::clamp(green, 0.0f, 1.0f);
        b = maths::clamp(blue, 0.0f, 1.0f);
        a = maths::clamp(alpha, 0.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    //! \brief
    //! \note value ranges ared [0 .. 255]. They are normalized to [0 .. 1].
    //--------------------------------------------------------------------------
    explicit Color(uint32_t const red,
                   uint32_t const green,
                   uint32_t const blue,
                   uint32_t const alpha = 255U)
    {
        r = static_cast<float>(maths::clamp(red, 0u, 255u)) / 255.0f;
        g = static_cast<float>(maths::clamp(green, 0u, 255u)) / 255.0f;
        b = static_cast<float>(maths::clamp(blue, 0u, 255u)) / 255.0f;
        a = static_cast<float>(maths::clamp(alpha, 0u, 255u)) / 255.0f;
    }

    inline void swap(Color& other)
    {
        std::swap(r, other.r);
        std::swap(g, other.g);
        std::swap(b, other.b);
        std::swap(a, other.a);
    }

    inline float& operator[](int const i)
    {
        return rgba[i];
    }

    inline float operator[](int const i) const
    {
        return rgba[i];
    }

    inline bool operator==(Color const& c) const
    {
        // Convert to ARGB (uint32_t) to avoid comparing floats.
        return this->toARGB() == c.toARGB();
    }

    inline bool operator!=(Color const& other) const
    {
        return !(*this == other);
    }

    inline Color& operator=(Vector4f const& other)
    {
        rgba[0] = other[0];
        rgba[1] = other[1];
        rgba[2] = other[2];
        rgba[3] = other[3];
        return *this;
    }

    inline Color& operator=(Vector3f const& other)
    {
        rgba[0] = other[0];
        rgba[1] = other[1];
        rgba[2] = other[2];
        return *this;
    }

    inline Color& operator=(Color const& other)
    {
        if (this != &other)
        {
            r = other.r;
            g = other.g;
            b = other.b;
            a = other.a;
        }
        return *this;
    }

    inline Color const operator+(Color const& other) const
    {
        return Color(r + other.r, g + other.g, b + other.b, a + other.a);
    }

    inline Color const operator+(float const scalar) const
    {
        return Color(r + scalar, g + scalar, b + scalar, a + scalar);
    }

    inline Color const operator+(uint8_t const scalar) const
    {
        float const ns = static_cast<float>(scalar) / 255.0f;
        return Color(r + ns, g + ns, b + ns, a + ns);
    }

    inline Color const operator-(Color const& other) const
    {
        return Color(r - other.r, g - other.g, b - other.b, a - other.a);
    }

    inline Color const operator-(float const scalar) const
    {
        return Color(r - scalar, g - scalar, b - scalar, a - scalar);
    }

    inline Color const operator-(uint8_t const scalar) const
    {
        float const ns = static_cast<float>(scalar) / 255.0f;
        return Color(r - ns, g - ns, b - ns, a - ns);
    }

    inline Color const operator*(float const scalar) const
    {
        return Color(r * scalar, g * scalar, b * scalar, a * scalar);
    }

    inline Color const operator*(uint8_t const scalar) const
    {
        float const ns = static_cast<float>(scalar) / 255.0f;
        return Color(r * ns, g * ns, b * ns, a * ns);
    }

    inline Color const operator/(float const scalar) const
    {
        float const finv = 1.0f / scalar;
        return Color(r * finv, g * finv, b * finv, a * finv);
    }

    inline Color const operator/(uint8_t const scalar) const
    {
        float const finv = 255.0f / static_cast<float>(scalar);
        return Color(r * finv, g * finv, b * finv, a * finv);
    }

    inline void constrain()
    {
        r = maths::clamp(r, 0.0f, 1.0f);
        g = maths::clamp(g, 0.0f, 1.0f);
        b = maths::clamp(b, 0.0f, 1.0f);
        a = maths::clamp(a, 0.0f, 1.0f);
    }

    static Color const& constrain(Color& color)
    {
        color.r = maths::clamp(color.r, 0.0f, 1.0f);
        color.g = maths::clamp(color.g, 0.0f, 1.0f);
        color.b = maths::clamp(color.b, 0.0f, 1.0f);
        color.a = maths::clamp(color.a, 0.0f, 1.0f);
        return color;
    }

    //--------------------------------------------------------------------------
    //! \brief From Hexa value
    //--------------------------------------------------------------------------
    void fromARGB(uint32_t const hex)
    {
        a = static_cast<float>((hex >> 24) & 255) / 255.0f;
        r = static_cast<float>((hex >> 16) & 255) / 255.0f;
        g = static_cast<float>((hex >> 8) & 255) / 255.0f;
        b = static_cast<float>((hex >> 0) & 255) / 255.0f;
    }

    //--------------------------------------------------------------------------
    //! \brief To Hexa value
    //--------------------------------------------------------------------------
    uint32_t toARGB() const
    {
        uint32_t ur = static_cast<uint32_t>(r * 255.0f);
        uint32_t ug = static_cast<uint32_t>(g * 255.0f);
        uint32_t ub = static_cast<uint32_t>(b * 255.0f);
        uint32_t ua = static_cast<uint32_t>(a * 255.0f);

        return (ua << 24) | (ur << 16) | (ug << 8) | (ub << 0);
    }

    //--------------------------------------------------------------------------
    //! \brief To Hexa value
    //--------------------------------------------------------------------------
    uint32_t toABGR() const
    {
        uint32_t ur = static_cast<uint32_t>(r * 255.0f);
        uint32_t ug = static_cast<uint32_t>(g * 255.0f);
        uint32_t ub = static_cast<uint32_t>(b * 255.0f);
        uint32_t ua = static_cast<uint32_t>(a * 255.0f);

        return (ua << 24) | (ub << 16) | (ug << 8) | (ur << 0);
    }

    //--------------------------------------------------------------------------
    //! \brief From Hexa value
    //--------------------------------------------------------------------------
    void fromABGR(uint32_t const hex)
    {
        a = static_cast<float>((hex >> 24) & 255) / 255.0f;
        b = static_cast<float>((hex >> 16) & 255) / 255.0f;
        g = static_cast<float>((hex >> 8) & 255) / 255.0f;
        r = static_cast<float>((hex >> 0) & 255) / 255.0f;
    }

    //--------------------------------------------------------------------------
    //! \brief To Hexa value
    //--------------------------------------------------------------------------
    uint32_t toRGBA() const
    {
        uint32_t ur = static_cast<uint32_t>(r * 255.0f);
        uint32_t ug = static_cast<uint32_t>(g * 255.0f);
        uint32_t ub = static_cast<uint32_t>(b * 255.0f);
        uint32_t ua = static_cast<uint32_t>(a * 255.0f);

        return (ur << 24) | (ug << 16) | (ub << 8) | (ua << 0);
    }

    Vector3f toVector3f()
    {
        return Vector3f(r, g, b);
    }

    Vector4f toVector4f()
    {
        return Vector4f(r, g, b, a);
    }

    //--------------------------------------------------------------------------
    //! \brief From Hexa value
    //--------------------------------------------------------------------------
    void fromRGBA(uint32_t const hex)
    {
        r = static_cast<float>((hex >> 24) & 255) / 255.0f;
        g = static_cast<float>((hex >> 16) & 255) / 255.0f;
        b = static_cast<float>((hex >> 8) & 255) / 255.0f;
        a = static_cast<float>((hex >> 0) & 255) / 255.0f;
    }

    static Color const Black;
    static Color const White;
    static Color const Red;
    static Color const Green;
    static Color const Blue;
    static Color const Yellow;
    static Color const Magenta;
    static Color const Cyan;
    static Color const Transparent;

    union
    {
        float rgba[4];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        struct
        {
            float r, g, b, a;
        };
#pragma GCC diagnostic pop
    };
};

#endif // OPENGLCPPWRAPPER_COLOR_HPP
