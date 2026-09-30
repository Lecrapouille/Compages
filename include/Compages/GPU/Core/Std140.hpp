// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/Core/Preprocessor.hpp"
#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <string>

// ****************************************************************************
//! \file
//! \brief Refuses to compile a C++ struct that does not match what a shader will
//! read from it.
//!
//! A uniform block is a struct on both sides: written by C++, read by GLSL. The
//! two agree on where each member sits only if the C++ struct happens to follow
//! the packing rules of the shading language, and the rules are not the ones C++
//! uses. The classic trap:
//!
//! \code
//! struct Uniforms          // GLSL, layout(std140)
//! {                        //
//!     compages::core::Vector3f position;   //   vec3 position;   at byte 0, occupying 12
//!     float radius;        //   float radius;    at byte 12
//! };                       //
//! \endcode
//!
//! That one happens to agree. This one does not:
//!
//! \code
//! struct Uniforms
//! {
//!     float radius;        //   float radius;    at byte 0
//!     compages::core::Vector3f position;   //   vec3 position;   std140 puts it at byte 16,
//! };                       //                    C++ puts it at byte 4
//! \endcode
//!
//! Copy that struct into a uniform buffer and the shader reads the position from
//! the wrong place. Nothing fails, no error is reported, and the image is subtly
//! wrong: the hardest kind of bug there is. Arrays are worse still, because
//! std140 pads every element of an array up to 16 bytes, so an array of floats
//! occupies four times what C++ gives it.
//!
//! GPU_STD140 turns that silence into a compile error. It is opt-in because it is
//! only needed by the fast path: a uniform block written member by member, at the
//! offsets the driver reports, is always correct and needs none of this. The check
//! is what buys the right to copy the whole struct in one go.
//!
//! For shader storage blocks, prefer layout(std430) in the shader: it drops the
//! padding of array elements, which is what makes std140 painful in the first
//! place, and it matches a plain C++ array of structs.
// ****************************************************************************

namespace compages::gpu::std140
{

// ----------------------------------------------------------------------------
//! \brief Round a size up to the next multiple of an alignment.
// ----------------------------------------------------------------------------
[[nodiscard]] constexpr std::size_t roundUp(std::size_t p_value,
                                            std::size_t p_alignment)
{
    return ((p_value + p_alignment - 1u) / p_alignment) * p_alignment;
}

// ****************************************************************************
//! \brief The alignment and the size std140 gives to a type.
//!
//! Left undefined for unknown types, so that putting something unexpected in a
//! uniform block is a readable error. Specialize it to teach the library about a
//! type of your own.
// ****************************************************************************
template <typename T>
struct Rules
{
    static_assert(sizeof(T) == 0,
                  "std140 does not know this type. A uniform block may hold "
                  "scalars, vectors, matrices, and arrays of those. Specialize "
                  "compages::gpu::std140::Rules<T> to teach it a type of your own");
};

//! \cond Doxygen_Suppress

//! \brief Scalars sit on 4 bytes and occupy 4, whatever their C++ size. A bool
//! in a block is 4 bytes, not 1, which is why it is not accepted here: use
//! std::uint32_t and say what you mean.
template <>
struct Rules<float>
{
    static constexpr std::size_t alignment = 4u;
    static constexpr std::size_t size = 4u;
    static constexpr const char* name = "float";
};

template <>
struct Rules<std::int32_t>
{
    static constexpr std::size_t alignment = 4u;
    static constexpr std::size_t size = 4u;
    static constexpr const char* name = "int";
};

template <>
struct Rules<std::uint32_t>
{
    static constexpr std::size_t alignment = 4u;
    static constexpr std::size_t size = 4u;
    static constexpr const char* name = "uint";
};

//! \brief A vector of two aligns on 8 bytes, one of three or four on 16. The odd
//! one out is the vector of three: it aligns on 16 but occupies only 12, so the
//! member after it may sit in the gap, and often does.
template <typename T>
struct Rules<compages::core::Vector<T, 2u>>
{
    static constexpr std::size_t alignment = 2u * Rules<T>::size;
    static constexpr std::size_t size = 2u * Rules<T>::size;
    static constexpr const char* name = "vec2";
};

template <typename T>
struct Rules<compages::core::Vector<T, 3u>>
{
    static constexpr std::size_t alignment = 4u * Rules<T>::size;
    static constexpr std::size_t size = 3u * Rules<T>::size;
    static constexpr const char* name = "vec3";
};

template <typename T>
struct Rules<compages::core::Vector<T, 4u>>
{
    static constexpr std::size_t alignment = 4u * Rules<T>::size;
    static constexpr std::size_t size = 4u * Rules<T>::size;
    static constexpr const char* name = "vec4";
};

//! \brief A matrix is an array of its columns, and every element of an array in
//! std140 is padded up to 16 bytes. A mat3 therefore occupies 48 bytes, not 36:
//! three columns of 16, of which only 12 are used. That surprise is exactly what
//! this file exists to catch.
template <typename T, std::size_t Rows, std::size_t Cols>
struct Rules<compages::core::Matrix<T, Rows, Cols>>
{
    static constexpr std::size_t alignment =
        roundUp(Rules<compages::core::Vector<T, Rows>>::alignment, 16u);
    static constexpr std::size_t size = Cols * alignment;
    static constexpr const char* name = "matrix";
};

//! \brief Every element of an array is padded up to 16 bytes. An array of ten
//! floats occupies 160 bytes in std140 and 40 in C++, so an array of scalars
//! never matches and must be written as an array of vectors of four, or the block
//! must be written member by member.
template <typename T, std::size_t N>
struct Rules<T[N]>
{
    static constexpr std::size_t alignment = roundUp(Rules<T>::alignment, 16u);
    static constexpr std::size_t size = N * roundUp(Rules<T>::size, 16u);
    static constexpr const char* name = "array";
};

template <typename T, std::size_t N>
struct Rules<std::array<T, N>>
{
    static constexpr std::size_t alignment = roundUp(Rules<T>::alignment, 16u);
    static constexpr std::size_t size = N * roundUp(Rules<T>::size, 16u);
    static constexpr const char* name = "array";
};

//! \endcond

//! \brief Rewrite, in place, the C++ bytes of a member into the bytes std140
//! expects. Null when both are the same.
using Convert = void (*)(std::byte*);

// ****************************************************************************
//! \brief How a member is converted before being copied into a block.
//!
//! Only matrices differ: \c Matrix is row-major while std140 stores a matrix
//! as an array of its columns.
// ****************************************************************************
template <typename T>
struct Staging
{
    static constexpr Convert convert = nullptr;
};

//! \cond Doxygen_Suppress
template <typename T, std::size_t Rows, std::size_t Cols>
struct Staging<compages::core::Matrix<T, Rows, Cols>>
{
    using M = compages::core::Matrix<T, Rows, Cols>;

    //! \brief Write the columns one after the other, each padded to the
    //! std140 column stride.
    static void toColumns(std::byte* p_bytes)
    {
        constexpr std::size_t stride =
            Rules<compages::core::Vector<T, Rows>>::alignment < 16u
                ? 16u
                : Rules<compages::core::Vector<T, Rows>>::alignment;
        M matrix;
        std::memcpy(&matrix, p_bytes, sizeof(M));
        std::memset(p_bytes, 0, sizeof(M));
        for (std::size_t c = 0u; c < Cols; ++c)
        {
            for (std::size_t r = 0u; r < Rows; ++r)
            {
                std::memcpy(p_bytes + (c * stride) + (r * sizeof(T)),
                            &matrix(r, c),
                            sizeof(T));
            }
        }
    }

    // A matrix whose std140 size differs from its C++ size is refused by
    // GPU_STD140, so there is nothing to convert in place.
    static constexpr Convert convert =
        (Rules<M>::size == sizeof(M)) ? &toColumns : nullptr;
};

template <typename T, std::size_t N>
struct Staging<T[N]>
{
    static void each(std::byte* p_bytes)
    {
        for (std::size_t i = 0u; i < N; ++i)
        {
            Staging<T>::convert(p_bytes + (i * sizeof(T)));
        }
    }

    static constexpr Convert convert =
        (Staging<T>::convert != nullptr) ? &each : nullptr;
};

template <typename T, std::size_t N>
struct Staging<std::array<T, N>>
{
    static constexpr Convert convert = Staging<T[N]>::convert;
};
//! \endcond

// ****************************************************************************
//! \brief One member of a block, as the check sees it.
// ****************************************************************************
struct Member
{
    //! \brief Name of the member, so a mismatch can be explained.
    const char* name = "";
    //! \brief Where C++ actually put it.
    std::size_t offset = 0u;
    //! \brief Where std140 requires it to start.
    std::size_t alignment = 0u;
    //! \brief How many bytes std140 gives it.
    std::size_t size = 0u;
    //! \brief Conversion applied to its bytes before upload, or null.
    Convert convert = nullptr;
};

// ----------------------------------------------------------------------------
//! \brief Where std140 would put a member, given where the previous one ended.
// ----------------------------------------------------------------------------
[[nodiscard]] constexpr std::size_t expectedOffset(std::size_t p_cursor,
                                                   Member const& p_member)
{
    return roundUp(p_cursor, p_member.alignment);
}

// ----------------------------------------------------------------------------
//! \brief Index of the first member C++ did not put where std140 wants it, or
//! the number of members when they all agree.
// ----------------------------------------------------------------------------
template <std::size_t N>
[[nodiscard]] constexpr std::size_t
firstMismatch(std::array<Member, N> const& p_members)
{
    std::size_t cursor = 0u;
    for (std::size_t i = 0u; i < N; ++i)
    {
        const std::size_t expected = expectedOffset(cursor, p_members[i]);
        if (p_members[i].offset != expected)
        {
            return i;
        }
        cursor = expected + p_members[i].size;
    }
    return N;
}

// ----------------------------------------------------------------------------
//! \brief Where std140 has got to just before a given member, that is the end of
//! the previous one.
// ----------------------------------------------------------------------------
template <std::size_t N>
[[nodiscard]] constexpr std::size_t
cursorBefore(std::array<Member, N> const& p_members, std::size_t p_index)
{
    std::size_t cursor = 0u;
    for (std::size_t i = 0u; i < p_index; ++i)
    {
        cursor = expectedOffset(cursor, p_members[i]) + p_members[i].size;
    }
    return cursor;
}

// ----------------------------------------------------------------------------
//! \brief Size std140 gives the whole block: everything the members occupy,
//! rounded up to 16 bytes.
// ----------------------------------------------------------------------------
template <std::size_t N>
[[nodiscard]] constexpr std::size_t
blockSize(std::array<Member, N> const& p_members)
{
    std::size_t cursor = 0u;
    for (std::size_t i = 0u; i < N; ++i)
    {
        cursor = expectedOffset(cursor, p_members[i]) + p_members[i].size;
    }
    return roundUp(cursor, 16u);
}

// ****************************************************************************
//! \brief What GPU_STD140 records about a struct.
//!
//! Left undefined until GPU_STD140 is used on a type. A typed uniform block
//! requires it, which is how the compiler refuses a struct that was never
//! checked.
// ****************************************************************************
template <typename T>
struct Description
{
    static constexpr bool declared = false;
};

// ----------------------------------------------------------------------------
//! \brief Has this struct been checked against the std140 rules?
// ----------------------------------------------------------------------------
template <typename T>
[[nodiscard]] constexpr bool described()
{
    return Description<T>::declared;
}

// ----------------------------------------------------------------------------
//! \brief Spell out, member by member, where C++ put things and where std140
//! wants them.
//!
//! A struct that does not conform never gets as far as running, since GPU_STD140
//! refuses to compile it. This is therefore for reading the layout of a struct
//! that does conform, which is worth doing when a shader reads sensible numbers
//! from the wrong fields, and for the library's own tests.
// ----------------------------------------------------------------------------
template <typename T>
[[nodiscard]] std::string explain()
{
    static_assert(Description<T>::declared,
                  "this struct was never described with GPU_STD140");

    auto const& members = Description<T>::members;
    const std::size_t bad = firstMismatch(members);

    std::string text = "std140 layout of a struct of " +
                       std::to_string(sizeof(T)) + " bytes:";
    std::size_t cursor = 0u;
    for (std::size_t i = 0u; i < members.size(); ++i)
    {
        const std::size_t expected = expectedOffset(cursor, members[i]);
        text += "\n  " + std::string(members[i].name) + ": C++ puts it at byte " +
                std::to_string(members[i].offset) + ", std140 wants byte " +
                std::to_string(expected);
        if (members[i].offset != expected)
        {
            text += "  <-- mismatch";
        }
        cursor = expected + members[i].size;
    }

    const std::size_t expected_size = roundUp(cursor, 16u);
    text += "\nwhole block: C++ says " + std::to_string(sizeof(T)) +
            " bytes, std140 says " + std::to_string(expected_size) + " bytes";

    if (bad < members.size())
    {
        text += "\n\nMove '" + std::string(members[bad].name) +
                "' earlier in the struct, or put the widest members first: "
                "std140 aligns vectors of three or four, and matrices, on 16 "
                "bytes. Otherwise write the block member by member instead of "
                "copying it whole.";
    }
    else if (sizeof(T) != expected_size)
    {
        text += "\n\nThe members are all where std140 wants them, but the struct "
                "is not the size std140 gives the block. Add padding at the end "
                "to reach a multiple of 16 bytes.";
    }

    return text;
}

// ----------------------------------------------------------------------------
//! \brief Does the struct match std140 in every respect?
// ----------------------------------------------------------------------------
template <typename T>
[[nodiscard]] constexpr bool conforms()
{
    if constexpr (!Description<T>::declared)
    {
        return false;
    }
    else
    {
        return (firstMismatch(Description<T>::members) ==
                Description<T>::members.size()) &&
               (sizeof(T) == blockSize(Description<T>::members));
    }
}

// ****************************************************************************
//! \brief Never defined, and named only in a compile error.
//!
//! A static_assert message has to be a string literal, so it cannot say which
//! member is misplaced. Asking for the size of an incomplete type can: the
//! compiler prints the type with its arguments filled in, so the error reads
//!
//!     invalid application of 'sizeof' to incomplete type
//!     'compages::gpu::std140::MismatchAtMember<1, 4, 16>'
//!
//! which says member number 1, C++ put it at byte 4, std140 wants byte 16.
//!
//! \tparam MemberIndex which member, counting from zero in the order they were
//! listed. When it equals the number of members, the members are all in place and
//! it is the size of the whole block that disagrees.
//! \tparam CppOffset where C++ put it, or the size of the struct.
//! \tparam Std140Offset where std140 wants it, or the size std140 gives the
//! block.
// ****************************************************************************
template <std::size_t MemberIndex,
          std::size_t CppOffset,
          std::size_t Std140Offset>
struct MismatchAtMember;

// ****************************************************************************
//! \brief Empty when the struct conforms, and holds a member of an incomplete
//! type when it does not, so that asking for its size names the offsets.
// ****************************************************************************
template <typename T, bool Conforms>
struct Verify
{
};

//! \cond Doxygen_Suppress
template <typename T>
struct Verify<T, false>
{
    static constexpr std::size_t COUNT = Description<T>::members.size();
    static constexpr std::size_t BAD = firstMismatch(Description<T>::members);

    //! \brief When a member is misplaced, report its two offsets. When they are
    //! all in place, the size of the whole block is what disagrees, so report
    //! that instead.
    static constexpr std::size_t CPP_VALUE =
        (BAD < COUNT) ? Description<T>::members[BAD].offset : sizeof(T);
    static constexpr std::size_t STD140_VALUE =
        (BAD < COUNT)
            ? expectedOffset(cursorBefore(Description<T>::members, BAD),
                             Description<T>::members[BAD])
            : blockSize(Description<T>::members);

    //! \brief Declaring a member of an incomplete type is the error, and the
    //! compiler has to print the type to report it.
    MismatchAtMember<BAD, CPP_VALUE, STD140_VALUE> where;
};
//! \endcond

} // namespace compages::gpu::std140

//! \cond Doxygen_Suppress
#define GPU_STD140_MEMBER(Type, member)                                       \
    compages::gpu::std140::Member                                                       \
    {                                                                         \
        #member, offsetof(Type, member),                                      \
            compages::gpu::std140::Rules<decltype(Type::member)>::alignment,            \
            compages::gpu::std140::Rules<decltype(Type::member)>::size,                 \
            compages::gpu::std140::Staging<decltype(Type::member)>::convert             \
    }
//! \endcond

// ****************************************************************************
//! \brief Check at compile time that a struct can be copied straight into a
//! uniform block.
//!
//! Write it once, at file scope, after the struct:
//! \code
//! struct Uniforms
//! {
//!     compages::core::Matrix44f projection;
//!     compages::core::Matrix44f view;
//!     compages::core::Vector3f light_direction;
//!     float ambient;
//! };
//! GPU_STD140(Uniforms, projection, view, light_direction, ambient);
//! \endcode
//!
//! Every member must be listed, in order: the check walks them the way std140
//! does, and a member left out would move everything after it.
//!
//! When it fails, the error comes in two parts. The sentence below says what is
//! wrong in general, and just above it the compiler complains about an incomplete
//! MismatchAtMember<index, cpp, wanted>, whose three numbers say which member and
//! which two offsets disagree.
// ****************************************************************************
#define GPU_STD140(Type, ...)                                                 \
    template <>                                                               \
    struct compages::gpu::std140::Description<Type>                                     \
    {                                                                         \
        static constexpr bool declared = true;                                \
        static constexpr std::array<compages::gpu::std140::Member,                      \
                                    GPU_PP_COUNT(__VA_ARGS__)>                \
            members{ GPU_PP_FOR_EACH(GPU_STD140_MEMBER, Type, __VA_ARGS__) }; \
    };                                                                        \
    static_assert(                                                            \
        compages::gpu::std140::conforms<Type>(),                                        \
        "this struct cannot be copied into a uniform block as it stands: C++ " \
        "and std140 do not put its members in the same places. The "           \
        "MismatchAtMember<index, cpp, wanted> named in the error just below "  \
        "says which member and which two offsets disagree. Usually the fix is " \
        "to put the vectors of three or four and the matrices first, and the " \
        "lone floats last");                                                  \
    static_assert(                                                            \
        sizeof(compages::gpu::std140::Verify<Type, compages::gpu::std140::conforms<Type>()>) > 0u, \
        "see the MismatchAtMember named just above")
