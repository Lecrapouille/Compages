// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// *****************************************************************************
// Inspired by https://github.com/Reedbeta/reed-util and its
// blog http://www.reedbeta.com/blog/on-vector-math-libraries/
// *****************************************************************************

#include "Compages/Core/Vector.hpp"

namespace compages::core
{
namespace matrix
{
//! \brief Enum used by the constructor for initializing matrices.
enum Type
{
    //! \brief Create an identity matrix.
    Identity,
    //! \brief Create a matrix filled with zeros.
    Zero,
    //! \brief Create a matrix filled with ones.
    One,
};
} // namespace matrix

// *****************************************************************************
//! \brief Class for small and dense matrices (up to 4x4). Elements are
//! consecutive values of type T using the row-major order. Example for a 4x4
//! matrix:
//!     | a1  a2  a3  a4  |
//! M = | a5  a6  a7  a8  |
//!     | a9  a10 a11 a12 |
//!     | a13 a14 a15 a16 |
//!
//! \note **Canonical convention (Scilab):** vectors are columns, transforms
//!   compose as \c y = M * x and \c M3 * (M2 * (M1 * x)) = (M3 * M2 * M1) * x
//!   ( \c M1 applies first ). Matrix-matrix and matrix-vector \c operator*
//!   implement that product on row-major storage. OpenGL expects column-major
//!   memory: transpose once at upload (\c glUniformMatrix4fv(..., GL_TRUE,
//!   ...)) or call \c transpose() before \c GL_FALSE — do not bake GL layout
//!   into every transform helper.
//!
//! \note \c Vector is always a column vector: only \c M * x is defined, there
//!   is no \c x * M. Use \c castToRowVector() or \c transpose() explicitly when
//!   a row is needed.
//!
//! \warning do not use big matrices since this class hold a static array and
//!   therefore not store in the heap but stored in the stack of called
//!   function. Too big matrices will create a stack overflow. Matrices up to
//!   4x4 for float or double are perfect.
// *****************************************************************************
template <typename T, size_t rows, size_t cols>
class Matrix
{
public:

    //--------------------------------------------------------------------------
    //! \brief matrix with unitialized data
    //--------------------------------------------------------------------------
    Matrix() = default;

    //--------------------------------------------------------------------------
    //! \brief Constructor with initialization list in the way. Matrix will be
    //! filled using the row-major order. For example {a1, a2, a3, a4, ... a16 }
    //! for a 4x4 matrix will create the following matrix:
    //!     | a1  a2  a3  a4  |
    //! M = | a5  a6  a7  a8  |
    //!     | a9  a10 a11 a12 |
    //!     | a13 a14 a15 a16 |
    //!
    //! \note If the initializer list number of element is shorter than the
    //! matrix dimension the matrix is completed with the default value passed
    //! as param.
    //!
    //! \note If the initializer list number of element is greater than the
    //! matrix dimension the matrix then extra data are ignored.
    //!
    //! \param[in] initList initialization list.
    //! \param[in] remainder default value used in the case where the
    //! initializer list is too short.
    //--------------------------------------------------------------------------
    template <typename U = T>
    Matrix(std::initializer_list<U> initList,
           T const remainder = zero<T>())
    {
        size_t m = std::min(rows * cols, size_t(initList.size()));
        auto iter = initList.begin();
        for (size_t i = 0u; i < m; ++i)
        {
            m_data[i] = T(*iter);
            ++iter;
        }

        // Zero-fill any remaining elements.
        for (size_t i = m; i < rows * cols; ++i)
        {
            m_data[i] = T(remainder);
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Constructor with an uniform value. Initialize the whole matrix
    //! with a single value
    //! \param[in] a the value for each element of the matrix.
    //--------------------------------------------------------------------------
    template <typename U = T>
    explicit Matrix(U a)
    {
        size_t i = rows * cols;
        while (i--)
        {
            m_data[i] = T(a);
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Constructor for identity matrix.
    //--------------------------------------------------------------------------
    explicit Matrix(matrix::Type const type)
    {
        size_t i = rows * cols;

        switch (type)
        {
            case matrix::Type::Identity:
                static_assert(
                    rows == cols,
                    "Can't construct identity for a non-square matrix");
                while (i--)
                {
                    m_data[i] = zero<T>();
                }
                i = cols;
                while (i--)
                {
                    m_data[cols * i + i] = one<T>();
                }
                break;
            case matrix::Type::Zero:
                while (i--)
                {
                    m_data[i] = zero<T>();
                }
                break;
            case matrix::Type::One:
                while (i--)
                {
                    m_data[i] = one<T>();
                }
                break;
            default:
                break;
        };
    }

    //--------------------------------------------------------------------------
    //! \brief Constructor by copy. Allow to pass bigger or smaller matrix.
    //--------------------------------------------------------------------------
    template <typename U, size_t rowsOther, size_t colsOther>
    explicit Matrix(Matrix<U, rowsOther, colsOther> const& m)
    {
        size_t r = std::min(rows, rowsOther);
        size_t c = std::min(cols, colsOther);
        for (size_t i = 0u; i < r; ++i)
        {
            for (size_t j = 0u; j < c; ++j)
            {
                m_data[cols * i + j] = T(m(i, j));
            }
            // Zero-fill any remaining cols
            for (size_t j = c; j < cols; ++j)
            {
                m_data[cols * i + j] = zero<T>();
            }
        }
        // Zero-fill any remaining rows
        for (size_t i = r * cols; i < rows * cols; ++i)
        {
            m_data[i] = zero<T>();
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Move constructor
    //--------------------------------------------------------------------------
    // Matrix(Matrix&&) noexcept = default;

    //--------------------------------------------------------------------------
    //! \brief Move assignement operator.
    //--------------------------------------------------------------------------
    // Matrix& operator=(Matrix&&) noexcept = default;

    //--------------------------------------------------------------------------
    //! \brief Return the dimension of the matrix.
    //! \param r (OUT) get the number of rows.
    //! \param c (OUT) get the number of columns.
    //--------------------------------------------------------------------------
    void size(size_t& r, size_t& c)
    {
        r = rows;
        c = cols;
    }

    //--------------------------------------------------------------------------
    //! \brief Cache friendly version of operator[i][j]
    //--------------------------------------------------------------------------
    T& operator()(size_t const i, size_t const j)
    {
        return m_data[cols * i + j];
    }

    //--------------------------------------------------------------------------
    //! \brief Cache friendly version of operator[i][j]
    //--------------------------------------------------------------------------
    T const& operator()(size_t const i, size_t const j) const
    {
        return m_data[cols * i + j];
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

    //--------------------------------------------------------------------------
    //! \brief Access to the nth row in write mode.
    //--------------------------------------------------------------------------
    Vector<T, cols>& operator[](size_t const i)
    {
        return reinterpret_cast<Vector<T, cols>&>(m_data[i * cols]);
    }

    //--------------------------------------------------------------------------
    //! \brief Acces to the nth row in read mode.
    //--------------------------------------------------------------------------
    Vector<T, cols> const& operator[](size_t const i) const
    {
        return reinterpret_cast<Vector<T, cols> const&>(m_data[i * cols]);
    }

    //--------------------------------------------------------------------------
    //! \brief Access to the nth row in write mode.
    //--------------------------------------------------------------------------
    Vector<T, cols>& operator[](int const i)
    {
        return reinterpret_cast<Vector<T, cols>&>(m_data[size_t(i) * cols]);
    }

    //--------------------------------------------------------------------------
    //! \brief Acces to the nth row in read mode.
    //--------------------------------------------------------------------------
    Vector<T, cols> const& operator[](int const i) const
    {
        return reinterpret_cast<Vector<T, cols> const&>(
            m_data[size_t(i) * cols]);
    }

#pragma GCC diagnostic pop

    //--------------------------------------------------------------------------
    //! \brief C array conversions
    //--------------------------------------------------------------------------
    typedef T (&array_t)[rows * cols];

    //--------------------------------------------------------------------------
    //! \brief C array conversions
    //--------------------------------------------------------------------------
    operator array_t()
    {
        return m_data;
    }
    array_t& data()
    {
        return m_data;
    }

    //--------------------------------------------------------------------------
    //! \brief Const C array conversions
    //--------------------------------------------------------------------------
    typedef const T (&const_array_t)[rows * cols];

    //--------------------------------------------------------------------------
    //! \brief Const C array conversions
    //--------------------------------------------------------------------------
    operator const_array_t() const
    {
        return m_data;
    }
    const_array_t& data() const
    {
        return m_data;
    }

private:

    //--------------------------------------------------------------------------
    // Disallow bool conversions (without this, they'd happen implicitly
    // via the array conversions)
    //--------------------------------------------------------------------------
    operator bool();

public:

    T m_data[rows * cols];
};

// *****************************************************************************
// Typedefs for the most common types and dimensions
// *****************************************************************************
typedef Matrix<bool, 2u, 2u> Matrix22b;
typedef Matrix<bool, 2u, 3u> Matrix23b;
typedef Matrix<bool, 3u, 3u> Matrix33b;
typedef Matrix<bool, 3u, 2u> Matrix32b;
typedef Matrix<bool, 4u, 4u> Matrix44b;

typedef Matrix<int, 2u, 2u> Matrix22i;
typedef Matrix<int, 2u, 3u> Matrix23i;
typedef Matrix<int, 3u, 3u> Matrix33i;
typedef Matrix<int, 3u, 2u> Matrix32i;
typedef Matrix<int, 4u, 4u> Matrix44i;

typedef Matrix<double, 2u, 2u> Matrix22g;
typedef Matrix<double, 2u, 3u> Matrix23g;
typedef Matrix<double, 3u, 3u> Matrix33g;
typedef Matrix<double, 3u, 2u> Matrix32g;
typedef Matrix<double, 4u, 4u> Matrix44g;

typedef Matrix<float, 2u, 2u> Matrix22f;
typedef Matrix<float, 2u, 3u> Matrix23f;
typedef Matrix<float, 3u, 3u> Matrix33f;
typedef Matrix<float, 3u, 2u> Matrix32f;
typedef Matrix<float, 4u, 4u> Matrix44f;

// *****************************************************************************
// Overloaded math operators
// *****************************************************************************

#define DEFINE_UNARY_OPERATOR(op)                                     \
    template <typename T, size_t rows, size_t cols>                   \
    Matrix<T, rows, cols> operator op(Matrix<T, rows, cols> const& a) \
    {                                                                 \
        Matrix<T, rows, cols> result;                                 \
        size_t i = rows * cols;                                       \
        while (i--)                                                   \
            result.m_data[i] = op a.m_data[i];                        \
        return result;                                                \
    }

#define DEFINE_BINARY_SCALAR_OPERATORS(op)                            \
    /* Scalar-matrix op */                                            \
    template <typename T, size_t rows, size_t cols>                   \
    Matrix<T, rows, cols> operator op(T const a,                      \
                                      Matrix<T, rows, cols> const& b) \
    {                                                                 \
        Matrix<T, rows, cols> result;                                 \
        size_t i = rows * cols;                                       \
        while (i--)                                                   \
            result.m_data[i] = a op b.m_data[i];                      \
        return result;                                                \
    }                                                                 \
    /* Matrix-scalar op */                                            \
    template <typename T, size_t rows, size_t cols>                   \
    Matrix<T, rows, cols> operator op(Matrix<T, rows, cols> const& a, \
                                      T const b)                      \
    {                                                                 \
        Matrix<T, rows, cols> result;                                 \
        size_t i = rows * cols;                                       \
        while (i--)                                                   \
            result.m_data[i] = a.m_data[i] op b;                      \
        return result;                                                \
    }

#define DEFINE_BINARY_OPERATORS(op)                                   \
    /* Matrix-matrix op */                                            \
    template <typename T, size_t rows, size_t cols>                   \
    Matrix<T, rows, cols> operator op(Matrix<T, rows, cols> const& a, \
                                      Matrix<T, rows, cols> const& b) \
    {                                                                 \
        Matrix<T, rows, cols> result;                                 \
        size_t i = rows * cols;                                       \
        while (i--)                                                   \
            result.m_data[i] = a.m_data[i] op b.m_data[i];            \
        return result;                                                \
    }                                                                 \
    DEFINE_BINARY_SCALAR_OPERATORS(op)

#define DEFINE_INPLACE_SCALAR_OPERATOR(op)                                  \
    /* Matrix-scalar op */                                                  \
    template <typename T, size_t rows, size_t cols>                         \
    Matrix<T, rows, cols>& operator op(Matrix<T, rows, cols>& a, T const b) \
    {                                                                       \
        size_t i = rows * cols;                                             \
        while (i--)                                                         \
            a.m_data[i] op b;                                               \
        return a;                                                           \
    }

#define DEFINE_INPLACE_OPERATORS(op)                                   \
    /* Matrix-matrix op */                                             \
    template <typename T, size_t rows, size_t cols>                    \
    Matrix<T, rows, cols>& operator op(Matrix<T, rows, cols>& a,       \
                                       Matrix<T, rows, cols> const& b) \
    {                                                                  \
        size_t i = rows * cols;                                        \
        while (i--)                                                    \
            a.m_data[i] op b.m_data[i];                                \
        return a;                                                      \
    }                                                                  \
    DEFINE_INPLACE_SCALAR_OPERATOR(op)

#define DEFINE_RELATIONAL_OPERATORS(op)                                  \
    /* Matrix-matrix op */                                               \
    template <typename T, typename U, size_t rows, size_t cols>          \
    Matrix<bool, rows, cols> operator op(Matrix<T, rows, cols> const& a, \
                                         Matrix<U, rows, cols> const& b) \
    {                                                                    \
        Matrix<bool, rows, cols> result;                                 \
        size_t i = rows * cols;                                          \
        while (i--)                                                      \
            result.m_data[i] = a.m_data[i] op b.m_data[i];               \
        return result;                                                   \
    }                                                                    \
    /* Scalar-matrix op */                                               \
    template <typename T, typename U, size_t rows, size_t cols>          \
    Matrix<bool, rows, cols> operator op(T const a,                      \
                                         Matrix<U, rows, cols> const& b) \
    {                                                                    \
        Matrix<bool, rows, cols> result;                                 \
        size_t i = rows * cols;                                          \
        while (i--)                                                      \
            result.m_data[i] = a op b.m_data[i];                         \
        return result;                                                   \
    }                                                                    \
    /* Matrix-scalar op */                                               \
    template <typename T, typename U, size_t rows, size_t cols>          \
    Matrix<bool, rows, cols> operator op(Matrix<T, rows, cols> const& a, \
                                         U const b)                      \
    {                                                                    \
        Matrix<bool, rows, cols> result;                                 \
        size_t i = rows * cols;                                          \
        while (i--)                                                      \
            result.m_data[i] = a.m_data[i] op b;                         \
        return result;                                                   \
    }

DEFINE_BINARY_OPERATORS(+)
DEFINE_BINARY_OPERATORS(-)
DEFINE_UNARY_OPERATOR(-)
DEFINE_BINARY_SCALAR_OPERATORS(*)
DEFINE_BINARY_SCALAR_OPERATORS(/)
DEFINE_BINARY_OPERATORS(&)
DEFINE_BINARY_OPERATORS(|)
DEFINE_BINARY_OPERATORS(^)
DEFINE_UNARY_OPERATOR(!)
DEFINE_UNARY_OPERATOR(~)

DEFINE_INPLACE_OPERATORS(+=)
DEFINE_INPLACE_OPERATORS(-=)
DEFINE_INPLACE_SCALAR_OPERATOR(*=)
DEFINE_INPLACE_SCALAR_OPERATOR(/=)
DEFINE_INPLACE_OPERATORS(&=)
DEFINE_INPLACE_OPERATORS(|=)
DEFINE_INPLACE_OPERATORS(^=)

DEFINE_RELATIONAL_OPERATORS(==)
DEFINE_RELATIONAL_OPERATORS(!=)
DEFINE_RELATIONAL_OPERATORS(<)
DEFINE_RELATIONAL_OPERATORS(>)
DEFINE_RELATIONAL_OPERATORS(<=)
DEFINE_RELATIONAL_OPERATORS(>=)

#undef DEFINE_UNARY_OPERATOR
#undef DEFINE_BINARY_SCALAR_OPERATORS
#undef DEFINE_BINARY_OPERATORS
#undef DEFINE_INPLACE_SCALAR_OPERATOR
#undef DEFINE_INPLACE_OPERATORS
#undef DEFINE_RELATIONAL_OPERATORS

// *****************************************************************************
//! \brief Matrix-Matrix multiplication.
//! \param[in] a the matrix (dimension MxN).
//! \param[in] b the matrix (dimension NxR).
//! \return a MxR matrix.
// *****************************************************************************
template <typename T, size_t rows, size_t inner, size_t cols>
Matrix<T, rows, cols> operator*(Matrix<T, rows, inner> const& a,
                                Matrix<T, inner, cols> const& b)
{
    Matrix<T, rows, cols> result(zero<T>());

    for (size_t i = 0u; i < rows; ++i)
        for (size_t j = 0u; j < cols; ++j)
            for (size_t k = 0; k < inner; ++k)
                result(i, j) += a(i, k) * b(k, j);

    return result;
}

// *****************************************************************************
//! \brief Matrix-Vector multiplication.
//!
//! Example:
//! \code
//!     |1 2|   |6|   |20|
//! B = |3 4| * |7| = |46|
//! \endcode
//! \param[in] a the matrix (dimension MxN).
//! \param[in] b the column vector (dimension \c cols).
//! \return a column vector (dimension \c rows).
// *****************************************************************************
template <typename T, size_t rows, size_t cols>
Vector<T, rows> operator*(Matrix<T, rows, cols> const& a,
                          Vector<T, cols> const& b)
{
    Vector<T, rows> result(zero<T>());
    size_t i = rows;
    while (i--)
    {
        size_t j = cols;
        while (j--)
            result[i] += (a(i, j) * b[j]);
    }
    return result;
}

// *****************************************************************************
// Vector-Matrix multiplication (x^T * M) is disabled: Vector is a column
// vector, so x * M has no meaning and silently applied the transposed
// transform. Write transpose(M) * x when x^T * M is really wanted.
// *****************************************************************************
#if 0
template <typename T, size_t rows, size_t cols>
Vector<T, cols> operator*(Vector<T, rows> const& a,
                          Matrix<T, rows, cols> const& b)
{
    Vector<T, cols> result(zero<T>());
    size_t i = rows;

    while (i--)
    {
        size_t j = cols;
        while (j--)
            result[j] += (a[i] * b(i, j));
    }
    return result;
}

template <typename T, size_t n>
Vector<T, n>& operator*=(Vector<T, n>& a, Matrix<T, n, n> const& b)
{
    a = a * b;
    return a;
}
#endif

// *****************************************************************************
//! \brief Matrix-Vector multiplication: matrix = matrix * vector
//! \param[in] a the matrix (dimension MxN).
//! \param[in] b the matrix (dimension NxN).
//! \return a MxN matrix.
// *****************************************************************************
template <typename T, size_t rows, size_t cols>
Matrix<T, rows, cols>& operator*=(Matrix<T, rows, cols>& a,
                                  Matrix<T, cols, cols> const& b)
{
    a = a * b;
    return a;
}

// *************************************************************************
//! \brief Display the matrix.
// *************************************************************************
template <typename T, size_t rows, size_t cols>
std::ostream& operator<<(std::ostream& os, Matrix<T, rows, cols> const& m)
{
    for (size_t i = 0u; i < rows; ++i)
    {
        os << ((i == 0) ? "[" : "; ") << m[i][0];
        for (size_t j = 1u; j < cols; ++j)
            os << " " << m[i][j];
    }
    os << "]";
    return os;
}

// *************************************************************************
//! \brief Convert a generic vector to a row matrix (shape 1 x N).
//!
//! A previous version returned an N x 1 matrix and wrote result(0, i), which
//! is past the only column. That swaps the two orientations and reads as a
//! transposition.
// *************************************************************************
template <typename T, size_t cols>
Matrix<T, 1u, cols> castToRowVector(Vector<T, cols> const& v)
{
    Matrix<T, 1u, cols> result;

    size_t i = cols;
    while (i--)
        result(0, i) = v[i];

    return result;
}

// *************************************************************************
//! \brief Convert a generic vector to a column matrix (shape N x 1).
// *************************************************************************
template <typename T, size_t rows>
Matrix<T, rows, 1u> castToColumnVector(Vector<T, rows> const& v)
{
    Matrix<T, rows, 1u> result;

    size_t i = rows;
    while (i--)
        result(i, 0) = v[i];

    return result;
}

// *************************************************************************
//! \brief Clear the given square matrix and set it as identity matrix.
//! \note The matrix shall be a squared matrix.
// *************************************************************************
template <typename T, size_t rows, size_t cols>
void identity(Matrix<T, rows, cols>& a)
{
    static_assert(rows == cols,
                  "Can't construct identity for a non-square matrix");
    a = Matrix<T, rows, cols>(matrix::Identity);
}

// *************************************************************************
//! \brief Clear the given square matrix and set it as zero matrix.
// *************************************************************************
template <typename T, size_t rows, size_t cols>
void zero(Matrix<T, rows, cols>& a)
{
    a = Matrix<T, rows, cols>(matrix::Zero);
}

// *************************************************************************
//! \brief Clear the given square matrix and set it as one matrix.
// *************************************************************************
template <typename T, size_t rows, size_t cols>
void one(Matrix<T, rows, cols>& a)
{
    a = Matrix<T, rows, cols>(matrix::One);
}

// *************************************************************************
//! \brief Compare each integer elements of two matrices, check if they have
//! the same value. Return the boolean matrix of the result.
//! \return the boolean matrix containing the result of each element
//! comparaison.
// *************************************************************************
template <typename T, size_t rows, size_t cols>
Matrix<bool, rows, cols>
compare(Matrix<T, rows, cols> const& a,
        Matrix<T, rows, cols> const& b,
        typename std::enable_if<std::is_integral<T>::value>::type* = 0)
{
    Matrix<bool, rows, cols> result;
    size_t i = rows * cols;

    while (i--)
    {
        result.m_data[i] = (a.m_data[i] == b.m_data[i]);
    }

    return result;
}

// *************************************************************************
//! \brief Compare each float elements of two matrices, check if they have
//! the same value +/- epsilon. Return the boolean matrix of the result.
//! \return the boolean matrix containing the result of each element
//! comparaison.
// *************************************************************************
template <typename T, size_t rows, size_t cols>
Matrix<bool, rows, cols>
compare(Matrix<T, rows, cols> const& a,
        Matrix<T, rows, cols> const& b,
        typename std::enable_if<std::is_floating_point<T>::value>::type* = 0)
{
    Matrix<bool, rows, cols> result;
    size_t i = rows * cols;

    while (i--)
    {
        result.m_data[i] = almostEqual(a.m_data[i], b.m_data[i]);
    }

    return result;
}

// *************************************************************************
//! \brief Hadamard product (element by element multiplication).
//! See https://en.wikipedia.org/wiki/Hadamard_product_(matrices)
// *************************************************************************
template <typename T, size_t rows, size_t cols>
Matrix<T, rows, cols> hadamard(Matrix<T, rows, cols> const& a,
                               Matrix<T, rows, cols> const& b)
{
    Matrix<T, rows, cols> result;
    size_t i = rows;
    while (i--)
    {
        size_t j = cols;
        while (j--)
            result(i, j) = a(i, j) * b(i, j);
    }
    return result;
}

// *************************************************************************
//! \brief Transpose the matrix.
// *************************************************************************
template <typename T, size_t rows, size_t cols>
Matrix<T, cols, rows> transpose(Matrix<T, rows, cols> const& a)
{
    Matrix<T, cols, rows> result;
    size_t i = rows;
    while (i--)
    {
        size_t j = cols;
        while (j--)
        {
            result(j, i) = a(i, j);
        }
    }
    return result;
}

// *************************************************************************
//! \brief Compute the matrix trace. The matrix shall be a squared matrix.
// *************************************************************************
template <typename T, size_t rows, size_t cols>
T trace(Matrix<T, rows, cols> const& a)
{
    static_assert(rows == cols,
                  "Can't compute the trace of a non-square matrix");

    T result = zero<T>();
    size_t i = rows;

    while (i--)
        result += a(i, i);

    return result;
}

// *************************************************************************
//! \brief Check if the matrix is diagonal.
//! \note The matrix shall be a squared matrix.
// *************************************************************************
template <typename T, size_t rows, size_t cols>
bool isDiagonal(Matrix<T, rows, cols> const& a)
{
    static_assert(rows == cols,
                  "Can't compute the diagonal of a non-square matrix");

    size_t i = rows;

    while (i--)
    {
        size_t j = cols;
        while (j--)
        {
            if (i != j)
            {
                if (!almostZero(a(i, j)))
                    return false;
            }
        }
    }
    return true;
}

// *************************************************************************
//! \brief Check if the matrix is symetric.
//! \note The matrix shall be a squared matrix.
// *************************************************************************
template <typename T, size_t rows, size_t cols>
bool isSymmetric(Matrix<T, rows, cols> const& a)
{
    static_assert(rows == cols,
                  "Can't compute the diagonal of a non-square matrix");

    size_t i = rows;

    while (i--)
    {
        size_t j = cols;
        while (j--)
        {
            if (i != j)
            {
                if (!almostEqual(a(i, j), a(j, i)))
                    return false;
            }
        }
    }
    return true;
}

// *************************************************************************
//! \brief
// *************************************************************************
template <typename T, size_t rows, size_t cols>
bool swapRows(Matrix<T, rows, cols>& a, size_t const i, size_t const j)
{
    if (i == j)
        return true;

    if ((i >= rows) || (j >= rows))
        return false;

    Vector<T, cols> tmp(a[i]);
    a[i] = a[j];
    a[j] = tmp;
    return true;
}

// *************************************************************************
//! \brief
// *************************************************************************
template <typename T>
T determinant(Matrix<T, 2u, 2u> const& m)
{
    return m[0][0] * m[1][1] - m[1][0] * m[0][1];
}

// *************************************************************************
//! \brief
// *************************************************************************
template <typename T>
T determinant(Matrix<T, 3u, 3u> const& m)
{
    return m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) -
           m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2]) +
           m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]);
}

// *************************************************************************
//! \brief https://www.dcode.fr/determinant-matrice
// *************************************************************************
template <typename T>
T determinant(Matrix<T, 4u, 4u> const& m)
{
    const T* ptr = m.m_data;

    const T afkp = ptr[0] * ptr[5] * ptr[10] * ptr[15];
    const T aflo = ptr[0] * ptr[5] * ptr[11] * ptr[14];
    const T agjp = ptr[0] * ptr[6] * ptr[9] * ptr[15];
    const T agln = ptr[0] * ptr[6] * ptr[11] * ptr[13];
    const T ahjo = ptr[0] * ptr[7] * ptr[9] * ptr[14];
    const T ahkn = ptr[0] * ptr[7] * ptr[10] * ptr[13];

    const T bekp = ptr[1] * ptr[4] * ptr[10] * ptr[15];
    const T belo = ptr[1] * ptr[4] * ptr[11] * ptr[14];
    const T bgip = ptr[1] * ptr[6] * ptr[8] * ptr[15];
    const T bglm = ptr[1] * ptr[6] * ptr[11] * ptr[12];
    const T bhio = ptr[1] * ptr[7] * ptr[8] * ptr[14];
    const T bhkm = ptr[1] * ptr[7] * ptr[10] * ptr[12];

    const T cejp = ptr[2] * ptr[4] * ptr[9] * ptr[15];
    const T celn = ptr[2] * ptr[4] * ptr[11] * ptr[13];
    const T cfip = ptr[2] * ptr[5] * ptr[8] * ptr[15];
    const T cflm = ptr[2] * ptr[5] * ptr[11] * ptr[12];
    const T chin = ptr[2] * ptr[7] * ptr[8] * ptr[13];
    const T chjm = ptr[2] * ptr[7] * ptr[9] * ptr[12];

    const T dejo = ptr[3] * ptr[4] * ptr[9] * ptr[14];
    const T dekn = ptr[3] * ptr[4] * ptr[10] * ptr[13];
    const T dfio = ptr[3] * ptr[5] * ptr[8] * ptr[14];
    const T dfkm = ptr[3] * ptr[5] * ptr[10] * ptr[12];
    const T dgin = ptr[3] * ptr[6] * ptr[8] * ptr[13];
    const T dgjm = ptr[3] * ptr[6] * ptr[9] * ptr[12];

    return afkp - aflo - agjp + agln + ahjo - ahkn - bekp + belo + bgip - bglm -
           bhio + bhkm + cejp - celn - cfip + cflm + chin - chjm - dejo + dekn +
           dfio - dfkm - dgin + dgjm;
}

// *************************************************************************
// 4x4 determinate implemented by blocks ..
//     | A B |
// det | C D | = det (A) * det(D - CA'B)
//
// template <typename T, size_t rows, size_t cols>
// T determinant(Matrix<T, 4u, 4u> const& m)
//{}
// TODO:https://stackoverflow.com/a/2625420/8877076
// *************************************************************************

// *************************************************************************
//! \brief
// *************************************************************************
template <typename T>
Matrix<T, 4u, 4u> inverse(Matrix<T, 4u, 4u> const& m)
{
    T m00 = m[0][0], m01 = m[0][1], m02 = m[0][2], m03 = m[0][3];
    T m10 = m[1][0], m11 = m[1][1], m12 = m[1][2], m13 = m[1][3];
    T m20 = m[2][0], m21 = m[2][1], m22 = m[2][2], m23 = m[2][3];
    T m30 = m[3][0], m31 = m[3][1], m32 = m[3][2], m33 = m[3][3];

    T v0 = m20 * m31 - m21 * m30;
    T v1 = m20 * m32 - m22 * m30;
    T v2 = m20 * m33 - m23 * m30;
    T v3 = m21 * m32 - m22 * m31;
    T v4 = m21 * m33 - m23 * m31;
    T v5 = m22 * m33 - m23 * m32;

    T t00 = +(v5 * m11 - v4 * m12 + v3 * m13);
    T t10 = -(v5 * m10 - v2 * m12 + v1 * m13);
    T t20 = +(v4 * m10 - v2 * m11 + v0 * m13);
    T t30 = -(v3 * m10 - v1 * m11 + v0 * m12);

    T invDet =
        one<T>() / (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);
    // assert(invDet != zero<T>() && "The matrix cannot be
    // inversed");

    Matrix<T, 4u, 4u> inv;
    inv[0][0] = t00 * invDet;
    inv[1][0] = t10 * invDet;
    inv[2][0] = t20 * invDet;
    inv[3][0] = t30 * invDet;

    inv[0][1] = -(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
    inv[1][1] = +(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
    inv[2][1] = -(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
    inv[3][1] = +(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

    v0 = m10 * m31 - m11 * m30;
    v1 = m10 * m32 - m12 * m30;
    v2 = m10 * m33 - m13 * m30;
    v3 = m11 * m32 - m12 * m31;
    v4 = m11 * m33 - m13 * m31;
    v5 = m12 * m33 - m13 * m32;

    inv[0][2] = +(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
    inv[1][2] = -(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
    inv[2][2] = +(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
    inv[3][2] = -(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

    v0 = m21 * m10 - m20 * m11;
    v1 = m22 * m10 - m20 * m12;
    v2 = m23 * m10 - m20 * m13;
    v3 = m22 * m11 - m21 * m12;
    v4 = m23 * m11 - m21 * m13;
    v5 = m23 * m12 - m22 * m13;

    inv[0][3] = -(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
    inv[1][3] = +(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
    inv[2][3] = -(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
    inv[3][3] = +(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

    return inv;
}

// *************************************************************************
//! \brief This function LU decomposes a given matrix A and stores
//! it in two new matricies L and U.  It uses the Gaussian
//! Elimination with partial pivoting algorithm from Golub & Van
//! Loan, Matrix Computations, Algorithm 3.4.1.
// *************************************************************************
template <typename T, size_t rows, size_t cols>
void LUdecomposition(Matrix<T, rows, cols> const& AA,
                     Matrix<T, rows, cols>& L,
                     Matrix<T, rows, cols>& U,
                     Matrix<T, rows, cols>& P)
{
    size_t i, j;

    // Set matrices to 0
    L *= zero<T>();
    U *= zero<T>();

    // FIXME: Copy not necessary
    Matrix<T, rows, cols> A(AA);

    for (i = 0; i < rows - 1; ++i)
    {
        double max = abs(A(i, i));
        size_t pivot = i;

        for (j = i + 1u; j < rows; ++j)
        {
            if (abs(A(j, i)) > max)
            {
                max = abs(A(j, i));
                pivot = j;
            }
        }

        if (pivot != i)
        {
            swapRows(A, i, pivot);
            swapRows(P, i, pivot);
        }

        // ERROR:
        // -- original code:  if (A(i, i) != 0.0)
        // -- new code which seems to give less good results: if (fabs(A(i, i))
        // > 0.00001) we cannot use == with floats or double !!!!
        if (A(i, i) != zero<T>())
        {
            for (j = i + 1u; j < rows; ++j)
            {
                A(j, i) = A(j, i) / A(i, i);
                for (size_t k = i + 1u; k < rows; ++k)
                {
                    A(j, k) = A(j, k) - A(j, i) * A(i, k);
                }
            }
        }
    }
    for (i = 0u; i < rows; ++i)
    {
        L(i, i) = one<T>();
        for (j = 0u; j < rows; ++j)
        {
            if (j < i)
            {
                L(i, j) = A(i, j);
            }
            else
            {
                U(i, j) = A(i, j);
            }
        }
    }
}

// *************************************************************************
//! \brief This function solves an LU decomposed matrix equation
//! LU.x = b.
// *************************************************************************
template <typename T, size_t rows, size_t cols>
Vector<T, rows> LUsolve(Matrix<T, rows, cols> const& L,
                        Matrix<T, rows, cols> const& U,
                        Matrix<T, rows, cols> const& P,
                        Vector<T, rows> const& b1)
{
    Vector<T, rows> solution(0), y;

    // Apply permutation
    Vector<T, rows> b(P * b1);

    // y = U.x, thus Ly = b
    // solve for y by forward substitution
    y[0] = b[0] / L(0, 0);
    for (size_t i = 1u; i < rows; ++i)
    {
        y[i] = b[i] / L(i, i);
        for (size_t j = 0u; j < i; ++j)
        {
            y[i] -= (L(i, j) * y[j] / L(i, i));
        }
    }

    // U.x = y
    // Solve for x by backward substitution
    size_t r = rows - 1u;
    solution[r] = y[r] / U(r, r);

    size_t i = r;
    while (i--)
    {
        solution[i] = y[i] / U(i, i);
        for (size_t j = i + 1u; j < rows; j++)
        {
            solution[i] -= (U(i, j) * solution[j] / U(i, i));
        }
    }
    return solution;
}

// *************************************************************************
//! \brief
// *************************************************************************
template <typename T, size_t rows, size_t cols>
Vector<T, rows> LUsolve(Matrix<T, rows, cols> const& A,
                        Vector<T, rows> const& b)
{
    Matrix<T, rows, cols> L;
    Matrix<T, rows, cols> U;
    Matrix<T, rows, cols> P(matrix::Type::Identity);

    LUdecomposition(A, L, U, P);
    return LUsolve(L, U, P, b);
}

template <typename T>
Matrix<T, 4u, 4u> translationMatrix(Vector<T, 3u> const& trans)
{
    Matrix<T, 4u, 4u> m(matrix::Type::Identity);
    m[0][3] = trans[0];
    m[1][3] = trans[1];
    m[2][3] = trans[2];
    m[3][3] = T(1);
    return m;
}

template <typename T>
Matrix<T, 4u, 4u> scalingMatrix(Vector<T, 3u> const& scal)
{
    Matrix<T, 4u, 4u> m(matrix::Type::Zero);
    m[0][0] = scal[0];
    m[1][1] = scal[1];
    m[2][2] = scal[2];
    m[3][3] = T(1);
    return m;
}
} // namespace compages::core

