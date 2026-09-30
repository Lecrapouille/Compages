//=====================================================================
// Compages: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#pragma once


// *****************************************************************************
// This file has been inspired by the following documents:
// https://github.com/Reedbeta/reed-util
// and http://www.reedbeta.com/blog/on-vector-math-libraries/
// and http://www.ogre3d.org
// and "Vectors in Julia" by Reese Pathak, David Zeng, Keegan Go, Stephen Boyd,
// Stanford University and the book: "Game Physics Engine Development" by Ian
// Millington
// *****************************************************************************
//
// Portability notes (see also comments on individual members):
// - Default construction leaves \c m_data uninitialized (performance).
// - Named members (\c x, \c y, …) live in anonymous structs inside a union:
//   widely supported as a compiler extension, not strict ISO C++.
// - \c operator T(&)[N] and \c data() yield a C array reference; combined with
//   converting constructors this can trigger surprising implicit conversions
//   when passing vectors to functions expecting raw pointers.
// - \c norm() / \c normalize() on integral \c T use \c sqrt (float) for
//   length but member \c normalize() still divides in \c T; use float/double
//   vectors for geometric normalization.
//
// *****************************************************************************

#include "Compages/Core/Maths.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <ostream>
#include <type_traits>

namespace compages::core
{


// *****************************************************************************
//! \brief Macro for building constructors of any dimension (N).
// *****************************************************************************
#define VECTOR_DIM(N)                                                          \
                                                                               \
public:                                                                        \
                                                                               \
    /*! \brief Empty constructor. Marked noexcept because a vertex struct made \
     * of vectors ends up in a std::vector, and a container asks whether it    \
     * can build its elements without an exception before choosing how to      \
     * grow.                                                                   \
     * \warning components are left UNINITIALIZED for performance: do not      \
     * read them before assigning a value. */                                  \
    Vector() noexcept                                                          \
    {                                                                          \
        static_assert(N >= 2u, "Minimum dimension for a vector is 2");         \
    }                                                                          \
                                                                               \
    /*! \brief Constructor with initialization list */                         \
    Vector(std::initializer_list<T> initList)                                  \
    {                                                                          \
        const size_t m = std::min(size_t(N), initList.size());                 \
        auto iter = initList.begin();                                          \
        for (size_t i = 0u; i < m; ++i)                                        \
        {                                                                      \
            m_data[i] = *iter;                                                 \
            ++iter;                                                            \
        }                                                                      \
                                                                               \
        /* Zero-fill any remaining elements */                                 \
        for (size_t i = m; i < N; ++i)                                         \
        {                                                                      \
            m_data[i] = zero<T>();                                      \
        }                                                                      \
    }                                                                          \
                                                                               \
    /*! \brief Constructor with uniform value */                               \
    explicit Vector(T const scalar)                                            \
    {                                                                          \
        size_t i = N;                                                          \
        while (i--)                                                            \
        {                                                                      \
            m_data[i] = scalar;                                                \
        }                                                                      \
    }                                                                          \
                                                                               \
    /*! \brief Constructor by copy */                                          \
    template <typename U, size_t nOther>                                       \
    explicit Vector(Vector<U, nOther> const& other)                            \
    {                                                                          \
        const size_t m = std::min(size_t(N), nOther);                          \
        size_t i = m;                                                          \
        while (i--)                                                            \
        {                                                                      \
            m_data[i] = T(other[i]);                                           \
        }                                                                      \
                                                                               \
        /* Zero-fill any remaining elements */                                 \
        for (i = m; i < N; ++i)                                                \
        {                                                                      \
            m_data[i] = zero<T>();                                      \
        }                                                                      \
    }                                                                          \
                                                                               \
    /*! \brief Flips all the components of the vector */                       \
    void invert()                                                              \
    {                                                                          \
        size_t i = N;                                                          \
        while (i--)                                                            \
        {                                                                      \
            m_data[i] = -m_data[i];                                            \
        }                                                                      \
    }                                                                          \
                                                                               \
    /*! \brief Add the given vector to this, scaled by the given amount */     \
    void addScaled(Vector<T, N> const& other, T const scale)                   \
    {                                                                          \
        size_t i = N;                                                          \
        while (i--)                                                            \
        {                                                                      \
            m_data[i] += (scale * other.m_data[i]);                            \
        }                                                                      \
    }                                                                          \
                                                                               \
    /*! \brief Multiply each component by a scalar. */                         \
    template <typename U>                                                      \
    Vector<T, N>& operator*=(U const& other)                                   \
    {                                                                          \
        size_t i = N;                                                          \
        while (i--)                                                            \
        {                                                                      \
            m_data[i] *= T(other);                                             \
        }                                                                      \
        return *this;                                                          \
    }                                                                          \
                                                                               \
    /*! \brief Return the dimension */                                         \
    inline size_t size() const                                                 \
    {                                                                          \
        return N;                                                              \
    }                                                                          \
                                                                               \
    /*! \brief Return the dimension */                                         \
    inline size_t length() const                                               \
    {                                                                          \
        return N;                                                              \
    }                                                                          \
                                                                               \
    /* Accessors */                                                            \
    T& operator[](size_t const i)                                              \
    {                                                                          \
        return m_data[i];                                                      \
    }                                                                          \
    const T& operator[](size_t const i) const                                  \
    {                                                                          \
        return m_data[i];                                                      \
    }                                                                          \
    T& operator[](int const i)                                                 \
    {                                                                          \
        return m_data[i];                                                      \
    }                                                                          \
    const T& operator[](int const i) const                                     \
    {                                                                          \
        return m_data[i];                                                      \
    }                                                                          \
                                                                               \
    /* C array conversions */                                                  \
    typedef T(&array_t)[N];                                                    \
    typedef const T(&const_array_t)[N];                                        \
    operator array_t()                                                         \
    {                                                                          \
        return m_data;                                                         \
    }                                                                          \
    operator const_array_t() const                                             \
    {                                                                          \
        return m_data;                                                         \
    }                                                                          \
    array_t& data()                                                            \
    {                                                                          \
        return m_data;                                                         \
    }                                                                          \
    const_array_t& data() const                                                \
    {                                                                          \
        return m_data;                                                         \
    }                                                                          \
                                                                               \
private:                                                                       \
                                                                               \
    /* Disallow bool conversions (without this, they'd happen */               \
    /* implicitly via the array conversions) */                                \
    operator bool() = delete;

// *****************************************************************************
//! \brief A mathematic element to represent coordinates in space. A Vector is
//! always a **column vector**: a transform applies as \c y = M * x, and
//! \c M2 * M1 * x applies \c M1 first. There is no \c x * M product; use
//! \c castToRowVector() (a 1 x n Matrix) or \c transpose(M) * x when a row
//! vector is really needed.
//!
//! \tparam T the type of the vector (float, int).
//! \tparam n the vector dimension.
//!
//! \warning Keep \c n small enough for stack storage; there is no guard against
//!   stack overflow. Storage is a plain \c T m_data[n] member array.
// *****************************************************************************
template <typename T, size_t n>
class Vector
{
    //--------------------------------------------------------------------------
    // Generic methods
    //--------------------------------------------------------------------------
    VECTOR_DIM(n);

public:

    //--------------------------------------------------------------------------
    //! \brief Return the norm of this instance.
    //--------------------------------------------------------------------------
    T norm() const
    {
        T length = zero<T>();
        size_t i = n;

        while (i--)
        {
            length += (m_data[i] * m_data[i]);
        }
        return T(sqrt(length));
    }

    //--------------------------------------------------------------------------
    //! \brief Make this instance be a unit vector.
    //! \note Undefined behavior if the norm is zero.
    //! \warning Integral \c T truncates on division; prefer float/double for
    //! direction vectors.
    //--------------------------------------------------------------------------
    void normalize()
    {
        T const l = one<T>() / norm();
        size_t i = n;

        while (i--)
        {
            m_data[i] *= l;
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Self linear interpolation toward \c uv by factor \c alpha.
    //--------------------------------------------------------------------------
    void lerp(Vector<T, n> const& uv, T const alpha)
    {
        size_t i = n;

        while (i--)
        {
            m_data[i] += (uv.m_data[i] - m_data[i]) * alpha;
        }
    }

public:

    T m_data[n];
};

// *****************************************************************************
//! \brief Specialization for vector of dimension 2
// *****************************************************************************
template <typename T>
class Vector<T, size_t(2)>
{
    //--------------------------------------------------------------------------
    // Generic methods
    //--------------------------------------------------------------------------
    VECTOR_DIM(2u);

public:

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //--------------------------------------------------------------------------
    Vector(const T scalar_x, const T scalar_y)
    {
        x = scalar_x;
        y = scalar_y;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the norm of this instance.
    //--------------------------------------------------------------------------
    inline T norm() const
    {
        return T(sqrt(x * x + y * y));
    }

    //--------------------------------------------------------------------------
    //! \brief Make this instance be a unit vector.
    //! \note Undefined behavior if the norm is zero.
    //! \warning Integral \c T truncates on division.
    //--------------------------------------------------------------------------
    void normalize()
    {
        T const l = one<T>() / norm();
        x *= l;
        y *= l;
    }

public:

    //--------------------------------------------------------------------------
    //! \brief static array holding values.
    //! \note anonymous unions of anonymous structs (x,y / u,v) are a widely
    //! supported compiler extension (GCC/Clang/MSVC) but are not strictly
    //! standard C++ (reading an inactive union member is UB per the standard).
    //--------------------------------------------------------------------------
    union
    {
        //! \brief static array holding values.
        T m_data[2u];

#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#endif
        struct
        {
            T x;
            T y;
        };
        struct
        {
            T u;
            T v;
        };
#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic pop
#endif
    };

    //! \brief Create a vector filled with NaN (for float and double only).
    const static Vector<T, 2u> DUMMY;
    //! \brief Create a vector filled with zero<T>().
    const static Vector<T, 2u> ZERO;
    //! \brief Create the vector filled with one<T>().
    const static Vector<T, 2u> UNIT_SCALE;
    //! \brief Create the vector filled with negative one<T>().
    const static Vector<T, 2u> NEGATIVE_UNIT_SCALE;
    //! \brief Create the vector [one<T>() zero<T>()].
    const static Vector<T, 2u> UNIT_X;
    //! \brief Create the vector [zero<T>() one<T>()].
    const static Vector<T, 2u> UNIT_Y;
    //! \brief Create the vector [-one<T>() zero<T>()].
    const static Vector<T, 2u> NEGATIVE_UNIT_X;
    //! \brief Create the vector [zero<T>() -one<T>()].
    const static Vector<T, 2u> NEGATIVE_UNIT_Y;
};

// Predefined vectors
template <typename T>
const Vector<T, 2u> Vector<T, 2u>::DUMMY(nan<T>());
template <typename T>
const Vector<T, 2u> Vector<T, 2u>::ZERO(zero<T>());
template <typename T>
const Vector<T, 2u> Vector<T, 2u>::UNIT_SCALE(one<T>());
template <typename T>
const Vector<T, 2u> Vector<T, 2u>::NEGATIVE_UNIT_SCALE(-one<T>());
template <typename T>
const Vector<T, 2u> Vector<T, 2u>::UNIT_X(one<T>(), zero<T>());
template <typename T>
const Vector<T, 2u> Vector<T, 2u>::UNIT_Y(zero<T>(), one<T>());
template <typename T>
const Vector<T, 2u> Vector<T, 2u>::NEGATIVE_UNIT_X(-one<T>(),
                                                   zero<T>());
template <typename T>
const Vector<T, 2u> Vector<T, 2u>::NEGATIVE_UNIT_Y(zero<T>(),
                                                   -one<T>());

// *****************************************************************************
//! \brief Specialization for vector of dimension 3
// *****************************************************************************
template <typename T>
class Vector<T, 3u>
{
    //--------------------------------------------------------------------------
    // Generic methods
    //--------------------------------------------------------------------------
    VECTOR_DIM(3u);

public:

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //--------------------------------------------------------------------------
    Vector(Vector<T, 2u> const& v, const T scalar_z = zero<T>())
    {
        x = v.x;
        y = v.y;
        z = scalar_z;
    }

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //--------------------------------------------------------------------------
    Vector(const T scalar_x,
           const T scalar_y,
           const T scalar_z = zero<T>())
    {
        x = scalar_x;
        y = scalar_y;
        z = scalar_z;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the norm of this instance.
    //--------------------------------------------------------------------------
    inline T norm() const
    {
        return sqrt(x * x + y * y + z * z);
    }

    //--------------------------------------------------------------------------
    //! \brief Make this instance be a unit vector.
    //! \note Undefined behavior if the norm is zero.
    //--------------------------------------------------------------------------
    void normalize()
    {
        T const l = one<T>() / norm();
        x *= l;
        y *= l;
        z *= l;
    }

    //--------------------------------------------------------------------------
    //! brief Self vector product (aka cross product).
    //--------------------------------------------------------------------------
    Vector<T, 3u>& operator%=(Vector<T, 3u> const& other)
    {
        *this = {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x,
        };
        return *this;
    }

public:

    //--------------------------------------------------------------------------
    //! \brief static array holding values.
    //--------------------------------------------------------------------------
    union
    {
        T m_data[3u];

#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#endif
        struct
        {
            T x;
            T y;
            T z;
        };
        struct
        {
            T r;
            T g;
            T b;
        };
#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic pop
#endif
    };

    //--------------------------------------------------------------------------
    // Predefined vectors
    //--------------------------------------------------------------------------

    //! \brief Create a vector filled with NaN (for float and double only).
    const static Vector<T, 3u> DUMMY;
    //! \brief Create a vector filled with zero<T>().
    const static Vector<T, 3u> ZERO;
    //! \brief Create the vector filled with one<T>().
    const static Vector<T, 3u> ONE;
    //! \brief Create the vector filled with inf<T>().
    const static Vector<T, 3u> POSITIVE_INFINITY;
    //! \brief Create the vector filled with -inf<T>().
    const static Vector<T, 3u> NEGATIVE_INFINITY;
    //! \brief Create the vector filled with one<T>().
    const static Vector<T, 3u> UNIT_SCALE;
    //! \brief Create the vector filled with negative one<T>().
    const static Vector<T, 3u> NEGATIVE_UNIT_SCALE;
    //! \brief Create the vector [one<T>() zero<T>() zero<T>()].
    const static Vector<T, 3u> UNIT_X;
    //! \brief Create the vector [zero<T>() one<T>() zero<T>()].
    const static Vector<T, 3u> UNIT_Y;
    //! \brief Create the vector [zero<T>() zero<T>() one<T>()].
    const static Vector<T, 3u> UNIT_Z;
    //! \brief Create the vector [-one<T>() zero<T>() zero<T>()].
    const static Vector<T, 3u> NEGATIVE_UNIT_X;
    //! \brief Create the vector [zero<T>() -one<T>() zero<T>()].
    const static Vector<T, 3u> NEGATIVE_UNIT_Y;
    //! \brief Create the vector [zero<T>() zero<T>() -one<T>()].
    const static Vector<T, 3u> NEGATIVE_UNIT_Z;
    //! \brief Alias for NEGATIVE_UNIT_X
    const static Vector<T, 3u> LEFT;
    //! \brief Alias for UNIT_X
    const static Vector<T, 3u> RIGHT;
    //! \brief Alias for NEGATIVE_UNIT_Y
    const static Vector<T, 3u> BACK;
    //! \brief Alias for UNIT_Y
    const static Vector<T, 3u> FORWARD;
    //! \brief Alias for NEGATIVE_UNIT_Z
    const static Vector<T, 3u> DOWN;
    //! \brief Alias for UNIT_Z
    const static Vector<T, 3u> UP;
};

//------------------------------------------------------------------------------
// Predefined vectors
//------------------------------------------------------------------------------

template <typename T>
const Vector<T, 3u> Vector<T, 3u>::DUMMY(nan<T>());
template <typename T>
const Vector<T, 3u> Vector<T, 3u>::ZERO(zero<T>());
template <typename T>
const Vector<T, 3u> Vector<T, 3u>::ONE(one<T>());
template <typename T>
const Vector<T, 3u> Vector<T, 3u>::POSITIVE_INFINITY(max<T>());
template <typename T>
const Vector<T, 3u> Vector<T, 3u>::NEGATIVE_INFINITY(-max<T>());

template <typename T>
const Vector<T, 3u> Vector<T, 3u>::UNIT_SCALE(one<T>());
template <typename T>
const Vector<T, 3u> Vector<T, 3u>::NEGATIVE_UNIT_SCALE(-one<T>());
template <typename T>
const Vector<T, 3u>
    Vector<T, 3u>::UNIT_X(one<T>(), zero<T>(), zero<T>());
template <typename T>
const Vector<T, 3u>
    Vector<T, 3u>::UNIT_Y(zero<T>(), one<T>(), zero<T>());
template <typename T>
const Vector<T, 3u>
    Vector<T, 3u>::UNIT_Z(zero<T>(), zero<T>(), one<T>());
template <typename T>
const Vector<T, 3u> Vector<T, 3u>::NEGATIVE_UNIT_X(-one<T>(),
                                                   zero<T>(),
                                                   zero<T>());
template <typename T>
const Vector<T, 3u> Vector<T, 3u>::NEGATIVE_UNIT_Y(zero<T>(),
                                                   -one<T>(),
                                                   zero<T>());
template <typename T>
const Vector<T, 3u> Vector<T, 3u>::NEGATIVE_UNIT_Z(zero<T>(),
                                                   zero<T>(),
                                                   -one<T>());

template <typename T>
const Vector<T, 3u>
    Vector<T, 3u>::LEFT(-one<T>(), zero<T>(), zero<T>());
template <typename T>
const Vector<T, 3u>
    Vector<T, 3u>::RIGHT(one<T>(), zero<T>(), zero<T>());
template <typename T>
const Vector<T, 3u>
    Vector<T, 3u>::BACK(zero<T>(), zero<T>(), -one<T>());
template <typename T>
const Vector<T, 3u>
    Vector<T, 3u>::FORWARD(zero<T>(), zero<T>(), one<T>());
template <typename T>
const Vector<T, 3u>
    Vector<T, 3u>::DOWN(zero<T>(), -one<T>(), zero<T>());
template <typename T>
const Vector<T, 3u>
    Vector<T, 3u>::UP(zero<T>(), one<T>(), zero<T>());

// *****************************************************************************
//! \brief Specialization for vector of dimension 4
// *****************************************************************************
template <typename T>
class Vector<T, 4u>
{
    //--------------------------------------------------------------------------
    // Generic methods
    //--------------------------------------------------------------------------
    VECTOR_DIM(4u);

public:

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //--------------------------------------------------------------------------
    Vector(Vector<T, 3u> const& v, const T scalar_w = zero<T>())
    {
        x = v.x;
        y = v.y;
        z = v.z;
        w = scalar_w;
    }

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //! \note z is zero-filled: the incoming vector only supplies x and y.
    //--------------------------------------------------------------------------
    Vector(Vector<T, 2u> const& v, const T scalar_w = zero<T>())
    {
        x = v.x;
        y = v.y;
        z = zero<T>();
        w = scalar_w;
    }

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //--------------------------------------------------------------------------
    Vector(const T scalar_x,
           const T scalar_y,
           const T scalar_z,
           const T scalar_w)
    {
        x = scalar_x;
        y = scalar_y;
        z = scalar_z;
        w = scalar_w;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the norm of this instance.
    //--------------------------------------------------------------------------
    inline T norm() const
    {
        return sqrt(x * x + y * y + z * z + w * w);
    }

    //--------------------------------------------------------------------------
    //! \brief Make this instance be a unit vector.
    //! \note Undefined behavior if the norm is zero.
    //--------------------------------------------------------------------------
    void normalize()
    {
        T const l = one<T>() / norm();
        x *= l;
        y *= l;
        z *= l;
        w *= l;
    }

public:

    //--------------------------------------------------------------------------
    //! \brief static array holding values.
    //--------------------------------------------------------------------------
    union
    {
        T m_data[4u];

#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#endif
        struct
        {
            T x;
            T y;
            T z;
            T w;
        };
        struct
        {
            T r;
            T g;
            T b;
            T a;
        };
#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic pop
#endif
    };

    //--------------------------------------------------------------------------
    // Predefined vectors
    //--------------------------------------------------------------------------

    //! \brief Create a vector filled with NaN (for float and double only).
    const static Vector<T, 4u> DUMMY;
    //! \brief Create a vector filled with zero<T>().
    const static Vector<T, 4u> ZERO;
    //! \brief Create the vector filled with one<T>().
    const static Vector<T, 4u> UNIT_SCALE;
    //! \brief Create the vector filled with negative one<T>().
    const static Vector<T, 4u> NEGATIVE_UNIT_SCALE;
    //! \brief Create the vector [one<T>() zero<T>() zero<T>() zero<T>()].
    const static Vector<T, 4u> UNIT_X;
    //! \brief Create the vector [zero<T>() one<T>() zero<T>() zero<T>()].
    const static Vector<T, 4u> UNIT_Y;
    //! \brief Create the vector [zero<T>() zero<T>() one<T>() zero<T>()].
    const static Vector<T, 4u> UNIT_Z;
    //! \brief Create the vector [zero<T>() zero<T>() zero<T>() one<T>()].
    const static Vector<T, 4u> UNIT_W;
    //! \brief Create the vector [-one<T>() zero<T>() zero<T>() zero<T>()].
    const static Vector<T, 4u> NEGATIVE_UNIT_X;
    //! \brief Create the vector [zero<T>() -one<T>() zero<T>() zero<T>()].
    const static Vector<T, 4u> NEGATIVE_UNIT_Y;
    //! \brief Create the vector [zero<T>() zero<T>() -one<T>() zero<T>()].
    const static Vector<T, 4u> NEGATIVE_UNIT_Z;
    //! \brief Create the vector [zero<T>() zero<T>() zero<T>() -one<T>()].
    const static Vector<T, 4u> NEGATIVE_UNIT_W;
};

//------------------------------------------------------------------------------
// Predefined vectors
//------------------------------------------------------------------------------

template <typename T>
const Vector<T, 4u> Vector<T, 4u>::DUMMY(nan<T>());
template <typename T>
const Vector<T, 4u> Vector<T, 4u>::ZERO(zero<T>());
template <typename T>
const Vector<T, 4u> Vector<T, 4u>::UNIT_SCALE(one<T>());
template <typename T>
const Vector<T, 4u> Vector<T, 4u>::NEGATIVE_UNIT_SCALE(-one<T>());
template <typename T>
const Vector<T, 4u> Vector<T, 4u>::UNIT_X(one<T>(),
                                          zero<T>(),
                                          zero<T>(),
                                          zero<T>());
template <typename T>
const Vector<T, 4u> Vector<T, 4u>::UNIT_Y(zero<T>(),
                                          one<T>(),
                                          zero<T>(),
                                          zero<T>());
template <typename T>
const Vector<T, 4u> Vector<T, 4u>::UNIT_Z(zero<T>(),
                                          zero<T>(),
                                          one<T>(),
                                          zero<T>());
template <typename T>
const Vector<T, 4u> Vector<T, 4u>::UNIT_W(zero<T>(),
                                          zero<T>(),
                                          zero<T>(),
                                          one<T>());
template <typename T>
const Vector<T, 4u> Vector<T, 4u>::NEGATIVE_UNIT_X(-one<T>(),
                                                   zero<T>(),
                                                   zero<T>(),
                                                   zero<T>());
template <typename T>
const Vector<T, 4u> Vector<T, 4u>::NEGATIVE_UNIT_Y(zero<T>(),
                                                   -one<T>(),
                                                   zero<T>(),
                                                   zero<T>());
template <typename T>
const Vector<T, 4u> Vector<T, 4u>::NEGATIVE_UNIT_Z(zero<T>(),
                                                   zero<T>(),
                                                   -one<T>(),
                                                   zero<T>());
template <typename T>
const Vector<T, 4u> Vector<T, 4u>::NEGATIVE_UNIT_W(zero<T>(),
                                                   zero<T>(),
                                                   zero<T>(),
                                                   -one<T>());

// *****************************************************************************
// Typedefs for the most common types and dimensions
// *****************************************************************************
using Vector2b = Vector<bool, 2u>;
using Vector3b = Vector<bool, 3u>;
using Vector4b = Vector<bool, 4u>;

using Vector2i = Vector<int, 2u>;
using Vector3i = Vector<int, 3u>;
using Vector4i = Vector<int, 4u>;

using Vector2u = Vector<unsigned int, 2u>;
using Vector3u = Vector<unsigned int, 3u>;
using Vector4u = Vector<unsigned int, 4u>;

using Vector2f = Vector<float, 2u>;
using Vector3f = Vector<float, 3u>;
using Vector4f = Vector<float, 4u>;

using Vector2g = Vector<double, 2u>;
using Vector3g = Vector<double, 3u>;
using Vector4g = Vector<double, 4u>;

// *****************************************************************************
// Overloaded math unary operators
// *****************************************************************************

#define DEFINE_UNARY_OPERATOR(op)                   \
    template <typename T, size_t n>                 \
    Vector<T, n> operator op(Vector<T, n> const& a) \
    {                                               \
        Vector<T, n> result;                        \
        size_t i = n;                               \
        while (i--)                                 \
            result[i] = op a[i];                    \
        return result;                              \
    }

// *****************************************************************************
// Overloaded math binary operators
// *****************************************************************************

#define DEFINE_BINARY_OPERATORS(op)                                        \
    /* Vector-Vector op */                                                 \
    template <typename T, size_t n>                                        \
    Vector<T, n> operator op(Vector<T, n> const& a, Vector<T, n> const& b) \
    {                                                                      \
        Vector<T, n> result;                                               \
        size_t i = n;                                                      \
        while (i--)                                                        \
            result[i] = a[i] op b[i];                                      \
        return result;                                                     \
    }                                                                      \
    /* Scalar-Vector op */                                                 \
    template <typename T, size_t n>                                        \
    Vector<T, n> operator op(T const a, Vector<T, n> const& b)             \
    {                                                                      \
        Vector<T, n> result;                                               \
        size_t i = n;                                                      \
        while (i--)                                                        \
            result[i] = a op b[i];                                         \
        return result;                                                     \
    }                                                                      \
    /* Vector-scalar op */                                                 \
    template <typename T, size_t n>                                        \
    Vector<T, n> operator op(Vector<T, n> const& a, T const b)             \
    {                                                                      \
        Vector<T, n> result;                                               \
        size_t i = n;                                                      \
        while (i--)                                                        \
            result[i] = a[i] op b;                                         \
        return result;                                                     \
    }

// *****************************************************************************
// Overloaded math inplace operators
// *****************************************************************************

#define DEFINE_INPLACE_OPERATORS(op)                                  \
    /* Vector-Vector op */                                            \
    template <typename T, size_t n>                                   \
    Vector<T, n>& operator op(Vector<T, n>& a, Vector<T, n> const& b) \
    {                                                                 \
        size_t i = n;                                                 \
        while (i--)                                                   \
            a[i] op b[i];                                             \
        return a;                                                     \
    }                                                                 \
    /* Vector-scalar op */                                            \
    template <typename T, size_t n>                                   \
    Vector<T, n>& operator op(Vector<T, n>& a, T const b)             \
    {                                                                 \
        size_t i = n;                                                 \
        while (i--)                                                   \
            a[i] op b;                                                \
        return a;                                                     \
    }

// *****************************************************************************
// Overloaded math relational operators
// *****************************************************************************

#define DEFINE_RELATIONAL_OPERATORS(op)                                       \
    /* Vector-Vector op */                                                    \
    template <typename T, typename U, size_t n>                               \
    Vector<bool, n> operator op(Vector<T, n> const& a, Vector<U, n> const& b) \
    {                                                                         \
        Vector<bool, n> result;                                               \
        size_t i = n;                                                         \
        while (i--)                                                           \
            result[i] = a[i] op b[i];                                         \
        return result;                                                        \
    }                                                                         \
    /* Scalar-Vector op */                                                    \
    template <typename T, typename U, size_t n>                               \
    Vector<bool, n> operator op(T const& a, Vector<U, n> const& b)            \
    {                                                                         \
        Vector<bool, n> result;                                               \
        size_t i = n;                                                         \
        while (i--)                                                           \
            result[i] = a op b[i];                                            \
        return result;                                                        \
    }                                                                         \
    /* Vector-scalar op */                                                    \
    template <typename T, typename U, size_t n>                               \
    Vector<bool, n> operator op(Vector<T, n> const& a, U const& b)            \
    {                                                                         \
        Vector<bool, n> result;                                               \
        size_t i = n;                                                         \
        while (i--)                                                           \
            result[i] = a[i] op b;                                            \
        return result;                                                        \
    }

// *****************************************************************************
// Overloaded math fun(x) operators
// *****************************************************************************

#define DEFINE_FUN1_OPERATOR(name, op)       \
    template <typename T, size_t n>          \
    Vector<T, n> name(Vector<T, n> const& a) \
    {                                        \
        Vector<T, n> result;                 \
        size_t i = n;                        \
                                             \
        while (i--)                          \
            result[i] = op(a[i]);            \
        return result;                       \
    }

// *****************************************************************************
// Overloaded math fun(x, y) operators
// *****************************************************************************

#define DEFINE_FUN2_OPERATOR(name, op)                              \
    template <typename T, size_t n>                                 \
    Vector<T, n> name(Vector<T, n> const& a, Vector<T, n> const& b) \
    {                                                               \
        Vector<T, n> result;                                        \
        size_t i = n;                                               \
                                                                    \
        while (i--)                                                 \
            result[i] = op(a[i], b[i]);                             \
        return result;                                              \
    }

// *****************************************************************************
// Overloaded math bool operators
// *****************************************************************************

#define DEFINE_BOOL_OPERATOR(name, op)                      \
    template <typename T, size_t n>                         \
    bool name(Vector<T, n> const& a, Vector<T, n> const& b) \
    {                                                       \
        size_t i = n;                                       \
                                                            \
        while (i--)                                         \
        {                                                   \
            if (!(a[i] op b[i]))                            \
                return false;                               \
        }                                                   \
        return true;                                        \
    }

// *****************************************************************************
// Overloaded operators
// *****************************************************************************

DEFINE_BINARY_OPERATORS(+)
DEFINE_BINARY_OPERATORS(-)
DEFINE_UNARY_OPERATOR(-)
DEFINE_UNARY_OPERATOR(+)
DEFINE_BINARY_OPERATORS(/)
DEFINE_BINARY_OPERATORS(&)
DEFINE_BINARY_OPERATORS(|)
DEFINE_BINARY_OPERATORS(^)
DEFINE_UNARY_OPERATOR(!)
DEFINE_UNARY_OPERATOR(~)

DEFINE_INPLACE_OPERATORS(+=)
DEFINE_INPLACE_OPERATORS(-=)
DEFINE_INPLACE_OPERATORS(/=)

DEFINE_RELATIONAL_OPERATORS(==)
DEFINE_RELATIONAL_OPERATORS(!=)
DEFINE_RELATIONAL_OPERATORS(<)
DEFINE_RELATIONAL_OPERATORS(>)
DEFINE_RELATIONAL_OPERATORS(<=)
DEFINE_RELATIONAL_OPERATORS(>=)

// *****************************************************************************
//! \brief Scalar-Vector op. Example: 2 * [x y z]
// *****************************************************************************
template <typename T, size_t n>
Vector<T, n> operator*(T const a, Vector<T, n> const& b)
{
    Vector<T, n> result;
    size_t i = n;
    while (i--)
        result[i] = a * b[i];
    return result;
}

// *****************************************************************************
//! \brief Vector-scalar op. Example: [x y z] * 2
// *****************************************************************************
template <typename T, size_t n>
Vector<T, n> operator*(Vector<T, n> const& a, T const b)
{
    Vector<T, n> result;
    size_t i = n;
    while (i--)
        result[i] = a[i] * b;
    return result;
}

// *****************************************************************************
//! \brief Display a vector (global algorithm).
// *****************************************************************************
template <typename T, size_t n>
std::ostream& operator<<(std::ostream& os, Vector<T, n> const& v)
{
    os << "[" << v[0];
    for (size_t i = 1; i < n; ++i)
    {
        os << ", " << v[i];
    }
    return os << ']';
}

// *****************************************************************************
//! \brief Display a vector (specialization for 2D vectors).
// *****************************************************************************
template <typename T>
std::ostream& operator<<(std::ostream& os, Vector<T, 2u> const& v)
{
    return os << "[" << v[0] << ", " << v[1] << ']';
}

// *****************************************************************************
//! \brief Display a vector (specialization for 3D vectors).
// *****************************************************************************
template <typename T>
std::ostream& operator<<(std::ostream& os, Vector<T, 3u> const& v)
{
    return os << "[" << v[0] << ", " << v[1] << ", " << v[2] << ']';
}

// *****************************************************************************
//! \brief Display a vector (specialization for 4D vectors).
// *****************************************************************************
template <typename T>
std::ostream& operator<<(std::ostream& os, Vector<T, 4u> const& v)
{
    return os << "[" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3]
              << ']';
}

namespace vector
{

// *************************************************************************
//! \brief Compare the integer values for each elements of vectors.
//! \note Do not confuse this function with operator==() or the
//! function equivalent() which do not have the same behavior.
//! \return true if all elements have the same value.
// *************************************************************************
template <typename T, size_t n>
    requires std::is_integral_v<T>
Vector<bool, n>
compare(Vector<T, n> const& a, Vector<T, n> const& b)
{
    Vector<bool, n> result;
    size_t i = n;

    while (i--)
    {
        result.m_data[i] = (a.m_data[i] == b.m_data[i]);
    }

    return result;
}

// *************************************************************************
//! \brief Compare the float values for each elements of vectors.
//! \note Do not confuse this function with operator==() or the
//! function equivalent() which do not have the same behavior.
//! \return true if all elements have the same value.
// *************************************************************************
template <typename T, size_t n>
    requires std::is_floating_point_v<T>
Vector<bool, n>
compare(Vector<T, n> const& a, Vector<T, n> const& b)
{
    Vector<bool, n> result;
    size_t i = n;

    while (i--)
    {
        result.m_data[i] = almostEqual(a.m_data[i], b.m_data[i]);
    }

    return result;
}

DEFINE_FUN2_OPERATOR(min, std::min)
DEFINE_FUN2_OPERATOR(max, std::max)
DEFINE_FUN1_OPERATOR(abs, ::compages::core::abs)
DEFINE_BOOL_OPERATOR(ge, >=)
DEFINE_BOOL_OPERATOR(gt, >)
DEFINE_BOOL_OPERATOR(le, <=)
DEFINE_BOOL_OPERATOR(lt, <)

// *************************************************************************
// *************************************************************************
template <typename T, size_t n>
void swap(Vector<T, n>& a, Vector<T, n>& b)
{
    size_t i = n;
    while (i--)
    {
        std::swap(a[i], b[i]);
    }
}

// *************************************************************************
// *************************************************************************
template <typename T, size_t n>
Vector<T, n> lerp(Vector<T, n> const& a, Vector<T, n> const& b, float t)
{
    Vector<T, n> r;

    size_t i = n;
    while (i--)
    {
        r[i] = ::compages::core::lerp(a[i], b[i], t);
    }

    return r;
}

// *************************************************************************
//! \brief Get the coefficient of collinearity (k) of two vectors (u and v).
//!
//! Two non-null vectors are collinear if and only it exists a
//! scalar k != 0, where: u = k v (note: if u is a null vector, any
//! vector v is collinear to u because k = 0).
//!
//! \return k if vectors are collinear, else return NaN if vectors are not
//! collinear, else return 0 if zero vector.
//!
//! \note Use this function for T a real but not for integers.
// *************************************************************************
template <typename T, size_t n>
T collinearity(Vector<T, n> const& u, Vector<T, n> const& v)
{
    // Null vector ?
    if (almostZero(u[0]) || almostZero(v[0]))
        return zero<T>();

    const T k = u[0] / v[0];
    for (size_t i = 1u; i < n; ++i)
    {
        if (!almostEqual(k * v[i], u[i]))
            return T(NAN);
    }
    return k;
}

// *************************************************************************
//! \brief Check if two vectors are parallels.
//! \note Use this function for T a real but not for integers.
// http://www.educastream.com/vecteurs-colineaires-seconde
// *************************************************************************
template <typename T, size_t n>
bool areCollinear(Vector<T, n> const& u, Vector<T, n> const& v)
{
    T k = collinearity(u, v);
    return !std::isnan(k);
}

// *************************************************************************
//! \brief Check if two vectors are mathematically equivalent: same
//! norm (magnitude), same direction (parallel) and same sign.
//! \note Use this function for T a real but not for integers.
// *************************************************************************
template <typename T, size_t n>
bool areEquivalent(Vector<T, n> const& u, Vector<T, n> const& v)
{
    T k = collinearity(u, v);
    return almostEqual(k, one<T>());
}

// *************************************************************************
//! \brief Check if three points A, B, C are aligned.
//! \note Use this function for T a real but not for integers.
// *************************************************************************
template <typename T, size_t n>
bool arePointsAligned(Vector<T, n> const& a,
                      Vector<T, n> const& b,
                      Vector<T, n> const& c)
{
    return areCollinear(b - a, c - a);
}

// *************************************************************************
//! \brief Constrain each element of the vector to lower and upper bounds.
// *************************************************************************
template <typename T, size_t n>
Vector<T, n> clamp(Vector<T, n> const& a, T const lower, T const upper)
{
    Vector<T, n> result;
    size_t i = n;

    while (i--)
        result[i] = ::compages::core::clamp(a[i], lower, upper);
    return result;
}

// *************************************************************************
//! \brief Calculates and returns a component-wise product of this vector
//! with the given vector.
//! \return a ° b = transpose([a_x, a_y, a_z]) ° transpose([b_x, b_y, b_z]) =
//! transpose([a_x * b_x, a_y * b_y, a_z * b_z])
// *************************************************************************
template <typename T, size_t n>
Vector<T, n> componentProduct(Vector<T, n> const& a, Vector<T, n> const& b)
{
    Vector<T, n> result;
    size_t i = n;
    while (i--)
        result[i] = a[i] * b[i];
    return result;
}

// *************************************************************************
//! \brief Performs a component-wise product with the given vector in place
//! and returns a reference to the updated (first) vector.
//! \param[in,out] a Left-hand vector, updated component-wise.
//! \param[in] b Right-hand vector.
// *************************************************************************
template <typename T, size_t n>
Vector<T, n>& componentProductUpdate(Vector<T, n>& a, Vector<T, n> const& b)
{
    size_t i = n;
    while (i--)
        a[i] *= b[i];
    return a;
}

// *************************************************************************
//! \brief Vector product, aka for cross product (specialization for 2D
//! vectors).
//!
// *************************************************************************
template <typename T>
T cross(Vector<T, 2u> const& a, Vector<T, 2u> const& b)
{
    return a.x * b.y - a.y * b.x;
}

// *************************************************************************
//! \brief Vector product, aka for cross product (specialization for 3D
//! vectors).
// *************************************************************************
template <typename T>
Vector<T, 3u> cross(Vector<T, 3u> const& a, Vector<T, 3u> const& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

// *************************************************************************
//! \brief Dot product, aka scalar product (general algorithm).
//! \return a . b = transpose([a_x a_y a_z ...]) . transpose([b_x b_y b_z ...])
// *************************************************************************
template <typename T, size_t n>
T dot(Vector<T, n> const& a, Vector<T, n> const& b)
{
    T result = zero<T>();
    size_t i = n;

    while (i--)
        result += a[i] * b[i];
    return result;
}

// *************************************************************************
//! \brief Dot product, aka scalar product (specialization for 3D vectors).
//! \return a . b = transpose([a_x a_y a_z]) . transpose([b_x b_y b_z])
//! Fast path when both operands are known to be 3D.
// *************************************************************************
template <typename T>
T dot(Vector<T, 3u> const& a, Vector<T, 3u> const& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// *************************************************************************
//! \brief Dot product, aka scalar product (specialization for 2D vectors).
//! \return a . b = transpose([a_x a_y]) . transpose([b_x b_y])
//! Fast path when both operands are known to be 2D.
// *************************************************************************
template <typename T>
T dot(Vector<T, 2u> const& a, Vector<T, 2u> const& b)
{
    return a.x * b.x + a.y * b.y;
}

// *************************************************************************
//! \brief Gets the squared magnitude of this vector.
//! \note Alias for squaredNorm()
// *************************************************************************
template <typename T, size_t n>
T squaredMagnitude(Vector<T, n> const& a)
{
    return dot(a, a);
}

// *************************************************************************
//! \brief Gets the squared magnitude of this vector.
//! \note Alias for squaredMagnitude()
// *************************************************************************
template <typename T, size_t n>
T squaredNorm(Vector<T, n> const& a)
{
    return dot(a, a);
}

// *************************************************************************
//! \brief Gets the magnitude (norm) of this vector.
//! \note Alias for norm()
// *************************************************************************
template <typename T, size_t n>
T magnitude(Vector<T, n> const& a)
{
    return T(sqrt(dot(a, a)));
}

// *************************************************************************
//! \brief Gets the norm (magnitude) of this vector.
//! \note Alias for magnitude()
// *************************************************************************
template <typename T, size_t n>
T norm(Vector<T, n> const& a)
{
    return magnitude(a);
}

// *************************************************************************
//! \brief Gets the squared magnitude of a - b.
// *************************************************************************
template <typename T, size_t n>
T squaredDistance(Vector<T, n> const& a, Vector<T, n> const& b)
{
    return squaredNorm(a - b);
}

// *************************************************************************
//! \brief Gets the magnitude of a - b.
// *************************************************************************
template <typename T, size_t n>
T distance(Vector<T, n> const& a, Vector<T, n> const& b)
{
    return sqrt(squaredDistance(a, b));
}

// *************************************************************************
//! \brief Turns a non-zero vector into a vector of unit length.
//! \note Undefined behavior if the norm is zero.
//! \warning Integral \c T truncates on division; use floating-point vectors
//! for direction vectors in 2D/3D geometry.
// *************************************************************************
template <typename T, size_t n>
Vector<T, n> normalize(Vector<T, n> const& a)
{
    return a / norm(a);
}

// *************************************************************************
//! \brief British spelling alias for normalize().
// *************************************************************************
template <typename T, size_t n>
Vector<T, n> normalise(Vector<T, n> const& a)
{
    return normalize(a);
}

// *************************************************************************
//! \brief Returns a vector at a point half way between the two
//! given positions.
// *************************************************************************
template <typename T, size_t n>
Vector<T, n> middle(Vector<T, n> const& a, Vector<T, n> const& b)
{
    Vector<T, n> result;
    size_t i = n;

    while (i--)
        result[i] = (a[i] + b[i]) / T(2);
    return result;
}

// *************************************************************************
//! \brief Return the perpendicular vector (specialization for 2D vectors).
// *************************************************************************
template <typename T>
Vector<T, 2u> orthogonal(Vector<T, 2u> const& a)
{
    return { -a.y, a.x };
}

// *************************************************************************
//! \brief Return the perpendicular vector (specialization for 3D vectors).
// *************************************************************************
template <typename T>
Vector<T, 3u> orthogonal(Vector<T, 3u> const& a)
{
    // Implementation due to Sam Hocevar - see blog post:
    // http://lolengine.net/blog/2013/09/21/picking-orthogonal-Vector-combing-coconuts
    if (abs(a.x) > abs(a.z))
        return { -a.y, a.x, zero<T>() };
    else
        return { zero<T>(), -a.z, a.y };
}

// *************************************************************************
//! \brief Check if the given two vectors are perpendicular between them.
// *************************************************************************
template <typename T, size_t n>
    requires std::is_floating_point_v<T>
bool areOrthogonal(Vector<T, n> const& a, Vector<T, n> const& b)
{
    return almostZero(dot(a, b));
}

template <typename T, size_t n>
    requires std::is_integral_v<T>
bool areOrthogonal(Vector<T, n> const& a, Vector<T, n> const& b)
{
    return zero<T>() == T(dot(a, b));
}

// *************************************************************************
//! \brief Gets the angle (in radians) between two vectors.
//! Comes from the definition of scalar product:
//!   a . b = norm(a) * norm(b) cos(angle)
//! => angle = cos^-1(dot(a,b) / (norm(a) * norm(b)))
// *************************************************************************
template <typename T, size_t n>
units::angle::radian_t angleBetween(Vector<T, n> const& org,
                                    Vector<T, n> const& dest)
{
    T const lenProduct = norm(org) * norm(dest);
    if (lenProduct == zero<T>())
    {
        return units::angle::radian_t(zero<T>());
    }

    T f = dot(org, dest) / lenProduct;
    f = std::min(std::max(f, -one<T>()), one<T>());
    return units::angle::radian_t(std::acos(f));
}

// *************************************************************************
//! \brief Calculates a reflection vector to the plane with the given normal.
// *************************************************************************
template <typename T, size_t n>
Vector<T, n> reflect(Vector<T, n> const& v, Vector<T, n> const& normal)
{
    return v - (T(2) * dot(v, normal) * normal);
}

// *************************************************************************
//! \brief Summation
// *************************************************************************
template <typename T, size_t n>
T sum(Vector<T, n> const& v)
{
    T res = zero<T>();
    size_t i = n;
    while (i--)
    {
        res += v[i];
    }

    return res;
}

// *************************************************************************
//! \brief Mean (general algorithm)
// *************************************************************************
template <typename T, size_t n>
T mean(Vector<T, n> const& v)
{
    return sum(v) / T(n);
}

// *************************************************************************
//! \brief Mean (specialization for 2D vectors).
// *************************************************************************
template <typename T>
T mean(Vector<T, 2u> const& v)
{
    return (v[0] + v[1]) / T(2);
}

// *************************************************************************
//! \brief Mean (specialization for 3D vectors).
// *************************************************************************
template <typename T>
T mean(Vector<T, 3u> const& v)
{
    return (v[0] + v[1] + v[2]) / T(3);
}

// *************************************************************************
//! \brief Mean (specialization for 4D vectors).
// *************************************************************************
template <typename T>
T mean(Vector<T, 4u> const& v)
{
    return (v[0] + v[1] + v[2] + v[3]) / T(4);
}

// *************************************************************************
//! \brief Root Mean Square
// *************************************************************************
template <typename T, size_t n>
T rms(Vector<T, n> const& v)
{
    return norm(v) / sqrt(v.size());
}

// *************************************************************************
//! \brief Standard deviation
// *************************************************************************
template <typename T, size_t n>
T std(Vector<T, n> const& v)
{
    return norm(v - vector::mean(v)) / sqrt(v.size());
}

} // namespace vector

// *****************************************************************************
//! \brief Scalar product (aka dot product).
// *****************************************************************************
template <typename T, size_t n>
T operator*(Vector<T, n> const& a, Vector<T, n> const& b)
{
    return vector::dot(a, b);
}

// *****************************************************************************
//! \brief Vector product (aka cross product).
// *****************************************************************************
template <typename T>
Vector<T, 3u> operator%(Vector<T, 3u> const& a, Vector<T, 3u> const& b)
{
    return vector::cross<T>(a, b);
}

// *****************************************************************************
//! \brief Vector product (aka cross product).
// *****************************************************************************
template <typename T>
T operator%(Vector<T, 2u> const& a, Vector<T, 2u> const& b)
{
    return vector::cross<T>(a, b);
}

#undef DEFINE_UNARY_OPERATOR
#undef DEFINE_BINARY_OPERATORS
#undef DEFINE_INPLACE_OPERATORS
#undef DEFINE_RELATIONAL_OPERATORS
#undef DEFINE_FUN1_OPERATOR
#undef DEFINE_FUN2_OPERATOR
#undef DEFINE_BOOL_OPERATOR

} // namespace compages::core

