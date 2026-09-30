// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/Core/Layout.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// ****************************************************************************
//! \file
//! \brief What a linked shader program turns out to contain.
//!
//! Read from the driver once, when the program is linked, and kept. Two things
//! make it worth having rather than guessing.
//!
//! The offsets of the members of a uniform block are the driver's, not ours. The
//! std140 rules say what they should be, and GPU_STD140 checks a C++ struct
//! against those rules, but the only authority on where the driver really put a
//! member is the driver. Asking it means a block can be filled correctly even
//! when the shader was written by somebody else, or compiled with a layout
//! qualifier nobody expected.
//!
//! And knowing what the program declares is what lets a mismatch be named. A
//! vertex layout offering "aPosition" to a shader that asks for "position" used
//! to draw nothing at all, with no message; now both lists are in hand and the
//! difference can be spelled out.
// ****************************************************************************

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
namespace compages::gpu
{



// ----------------------------------------------------------------------------
//! \brief A type as a shader declares it.
//!
//! Covers what a vertex attribute, a uniform or a member of a block can be. A
//! type the library does not know is reported as Unknown rather than refused:
//! introspection is meant to describe whatever it finds, and a shader using an
//! exotic type should still be usable for everything else it declares.
// ----------------------------------------------------------------------------
enum class DataType
{
    Float,
    Vec2,
    Vec3,
    Vec4,

    Double,
    DVec2,
    DVec3,
    DVec4,

    Int,
    IVec2,
    IVec3,
    IVec4,

    UInt,
    UVec2,
    UVec3,
    UVec4,

    Bool,
    BVec2,
    BVec3,
    BVec4,

    //! \brief Named as GLSL names them: matCxR has C columns and R rows, so a
    //! mat2x4 is two columns of four. mat2, mat3 and mat4 are the square ones.
    Mat2,
    Mat3,
    Mat4,
    Mat2x3,
    Mat2x4,
    Mat3x2,
    Mat3x4,
    Mat4x2,
    Mat4x3,

    Sampler1D,
    Sampler2D,
    Sampler3D,
    SamplerCube,
    Sampler1DArray,
    Sampler2DArray,
    SamplerCubeArray,
    Sampler2DShadow,
    SamplerCubeShadow,
    SamplerBuffer,
    ISampler2D,
    ISampler3D,
    USampler2D,
    USampler3D,

    //! \brief An image, which unlike a sampler can be written to. This is how a
    //! compute pass produces a texture.
    Image1D,
    Image2D,
    Image3D,
    ImageCube,
    Image2DArray,
    IImage2D,
    UImage2D,

    //! \brief Something the library has no name for. Its name and location are
    //! still reported.
    Unknown,
};

// ----------------------------------------------------------------------------
//! \brief The type as it is spelled in shader source, for error messages.
// ----------------------------------------------------------------------------
[[nodiscard]] const char* toString(DataType p_type);

// ----------------------------------------------------------------------------
//! \brief Is this a texture the shader reads through a sampler?
// ----------------------------------------------------------------------------
[[nodiscard]] bool isSampler(DataType p_type);

// ----------------------------------------------------------------------------
//! \brief Is this an image the shader may write to?
// ----------------------------------------------------------------------------
[[nodiscard]] bool isImage(DataType p_type);

// ----------------------------------------------------------------------------
//! \brief Is this a matrix?
// ----------------------------------------------------------------------------
[[nodiscard]] bool isMatrix(DataType p_type);

// ----------------------------------------------------------------------------
//! \brief How many columns, which for a vector or a scalar is one.
// ----------------------------------------------------------------------------
[[nodiscard]] std::uint8_t columnsOf(DataType p_type);

// ----------------------------------------------------------------------------
//! \brief How many rows, that is how many components one column holds.
// ----------------------------------------------------------------------------
[[nodiscard]] std::uint8_t rowsOf(DataType p_type);

// ----------------------------------------------------------------------------
//! \brief What kind of number the components are, or Float for a sampler, which
//! has no components of its own.
// ----------------------------------------------------------------------------
[[nodiscard]] ScalarType scalarOf(DataType p_type);

// ----------------------------------------------------------------------------
//! \brief The vertex format an attribute of this type expects.
//!
//! This is what makes a C++ vertex layout comparable to what the shader declares:
//! both sides end up as an AttributeFormat, and a mismatch can be shown in the
//! same words.
// ----------------------------------------------------------------------------
[[nodiscard]] AttributeFormat attributeFormatOf(DataType p_type);

// ****************************************************************************
//! \brief One `in` variable of the vertex stage.
// ****************************************************************************
struct AttributeInfo
{
    //! \brief The name the shader declares.
    std::string name;
    //! \brief Which attribute slot the driver assigned to it.
    int location = -1;
    //! \brief What the shader declares it as.
    DataType type = DataType::Unknown;
    //! \brief The same thing said as a vertex format, to compare with a
    //! VertexLayout.
    AttributeFormat format;
    //! \brief How many of them, for an array attribute. One when not an array.
    int elements = 1;
};

// ****************************************************************************
//! \brief One uniform that does not belong to a block.
//!
//! These are the ones set one at a time, and the ones the driver keeps its own
//! copy of. Samplers appear here too, since a sampler is a uniform holding the
//! number of a texture unit.
// ****************************************************************************
struct UniformInfo
{
    //! \brief The name the shader declares. An array member appears as
    //! "lights[0]", which is how the driver names it.
    std::string name;
    //! \brief Where to write it. Never -1 for a uniform that is really used.
    int location = -1;
    //! \brief What the shader declares it as.
    DataType type = DataType::Unknown;
    //! \brief How many, for an array. One when not an array.
    int elements = 1;
};

// ****************************************************************************
//! \brief One member of a uniform or storage block, at the offset the driver
//! chose.
// ****************************************************************************
struct BlockMember
{
    //! \brief The name the shader declares, without the name of the block.
    std::string name;
    //! \brief What the shader declares it as.
    DataType type = DataType::Unknown;
    //! \brief Distance in bytes from the start of the block. The driver's
    //! answer, not a computed one.
    std::uint32_t offset = 0u;
    //! \brief How many, for an array. One when not an array.
    int elements = 1;
    //! \brief Distance in bytes from one element of the array to the next. In
    //! std140 an array of floats has a stride of 16, not 4, which is the rule
    //! people are most often surprised by.
    std::uint32_t array_stride = 0u;
    //! \brief Distance in bytes from one column of a matrix to the next.
    std::uint32_t matrix_stride = 0u;

    // ------------------------------------------------------------------------
    //! \brief How many bytes this member occupies, arrays and matrix padding
    //! included.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t bytes() const;
};

// ****************************************************************************
//! \brief One uniform block or one shader storage block.
// ****************************************************************************
struct BlockInfo
{
    //! \brief The name of the block as declared, not the name of the instance.
    std::string name;
    //! \brief Which binding point the shader expects it on. This is what a
    //! buffer has to be bound to for the shader to see it.
    int binding = 0;
    //! \brief Size in bytes the driver wants the buffer to be, padding
    //! included.
    std::size_t bytes = 0u;
    //! \brief The members, in the order the driver reports them.
    std::vector<BlockMember> members;

    // ------------------------------------------------------------------------
    //! \brief The member of this name, or nullptr.
    // ------------------------------------------------------------------------
    [[nodiscard]] BlockMember const* find(std::string_view p_name) const;
};

// ****************************************************************************
//! \brief Everything a linked program turns out to declare.
// ****************************************************************************
struct ProgramReflection
{
    //! \brief The `in` variables of the vertex stage.
    std::vector<AttributeInfo> attributes;
    //! \brief The uniforms outside any block, samplers included.
    std::vector<UniformInfo> uniforms;
    //! \brief The uniform blocks, in the order the driver lists them, which is
    //! also how it identifies them.
    std::vector<BlockInfo> uniform_blocks;
    //! \brief The shader storage blocks, which a compute pass writes to.
    std::vector<BlockInfo> storage_blocks;
    //! \brief For a compute program, the work group size the shader declared in
    //! its `layout(local_size_x = ...)`. Zero for a graphics program.
    std::array<int, 3> work_group_size{ 0, 0, 0 };

    // ------------------------------------------------------------------------
    //! \brief The attribute of this name, or nullptr.
    // ------------------------------------------------------------------------
    [[nodiscard]] AttributeInfo const* attribute(std::string_view p_name) const;

    // ------------------------------------------------------------------------
    //! \brief The uniform of this name, or nullptr.
    // ------------------------------------------------------------------------
    [[nodiscard]] UniformInfo const* uniform(std::string_view p_name) const;

    // ------------------------------------------------------------------------
    //! \brief The uniform block of this name, or nullptr.
    // ------------------------------------------------------------------------
    [[nodiscard]] BlockInfo const* uniformBlock(std::string_view p_name) const;

    // ------------------------------------------------------------------------
    //! \brief The storage block of this name, or nullptr.
    // ------------------------------------------------------------------------
    [[nodiscard]] BlockInfo const* storageBlock(std::string_view p_name) const;

    // ------------------------------------------------------------------------
    //! \brief The samplers among the uniforms, which is what a pass has to be
    //! given textures for.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::vector<UniformInfo const*> samplers() const;

    // ------------------------------------------------------------------------
    //! \brief Everything the program declares, laid out for reading.
    //!
    //! What to print when a shader is not doing what its author expected: it
    //! shows the names as the driver knows them, which is often the difference
    //! itself.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string toString() const;
};

// ****************************************************************************
//! \brief What a shader would have to declare to be given a value of this C++
//! type.
//!
//! The one place the two type systems are put side by side. Left undefined for
//! anything else, so offering a shader a type it cannot hold is a compile error
//! naming the type rather than a silent conversion.
// ****************************************************************************
template <typename T>
struct TypeOf
{
    static_assert(sizeof(T) == 0,
                  "no shader type corresponds to this C++ type. A uniform may "
                  "be a float, an int, an unsigned int, a bool, a vector or a "
                  "matrix of those");
};

//! \cond Doxygen_Suppress

template <>
struct TypeOf<float>
{
    static constexpr DataType value = DataType::Float;
};

template <>
struct TypeOf<std::int32_t>
{
    static constexpr DataType value = DataType::Int;
};

template <>
struct TypeOf<std::uint32_t>
{
    static constexpr DataType value = DataType::UInt;
};

template <>
struct TypeOf<bool>
{
    static constexpr DataType value = DataType::Bool;
};

//! \brief A vector maps onto the family of its component type and the family
//! onto its size, which is the whole of the naming scheme: vec3, ivec3, uvec3
//! and bvec3 differ only in where they start.
template <typename T, std::size_t N>
struct TypeOf<compages::core::Vector<T, N>>
{
    static_assert((N >= 2u) && (N <= 4u),
                  "a shader knows vectors of two, three and four");

    static constexpr DataType FIRST = TypeOf<T>::value;
    static constexpr DataType value =
        static_cast<DataType>(static_cast<int>(FIRST) + static_cast<int>(N) - 1);
};

//! \brief matCxR in GLSL has C columns of R rows, and the library's Matrix is
//! named the other way round, rows first, which is the one place the two
//! conventions have to be crossed.
template <std::size_t Rows, std::size_t Cols>
struct TypeOf<compages::core::Matrix<float, Rows, Cols>>
{
    static_assert((Rows >= 2u) && (Rows <= 4u) && (Cols >= 2u) && (Cols <= 4u),
                  "a shader knows matrices from two to four in each direction");

    [[nodiscard]] static constexpr DataType pick()
    {
        if constexpr (Rows == Cols)
        {
            return static_cast<DataType>(static_cast<int>(DataType::Mat2) +
                                         static_cast<int>(Rows) - 2);
        }
        else
        {
            // Mat2x3, Mat2x4, Mat3x2, Mat3x4, Mat4x2, Mat4x3 in that order, that
            // is columns first and skipping the square ones already named above.
            constexpr int TABLE[3][3] = {
                { -1, static_cast<int>(DataType::Mat2x3),
                  static_cast<int>(DataType::Mat2x4) },
                { static_cast<int>(DataType::Mat3x2), -1,
                  static_cast<int>(DataType::Mat3x4) },
                { static_cast<int>(DataType::Mat4x2),
                  static_cast<int>(DataType::Mat4x3), -1 }
            };
            return static_cast<DataType>(TABLE[Cols - 2u][Rows - 2u]);
        }
    }

    static constexpr DataType value = pick();
};

//! \endcond

} // namespace compages::gpu
