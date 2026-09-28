//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
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
//=============================================================================

#pragma once

#include "Compages/GPU/Core/FieldReflection.hpp"
#include "Compages/GPU/Core/Preprocessor.hpp"
#include "Compages/GPU/Core/Handle.hpp"
#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// ****************************************************************************
//! \file
//! \brief Says what one vertex looks like in memory.
//!
//! This is where the previous design went wrong, so it is worth saying what
//! changed. Before, the shader decided: the program was introspected, and for
//! every attribute it declared a separate buffer was created, each holding one
//! attribute tightly packed. Three consequences followed, none of them wanted.
//!
//! Interleaving was impossible. Position, normal and texture coordinates of one
//! vertex sat in three different buffers, so drawing read three streams that were
//! far apart in memory, when they are always used together.
//!
//! The layout could not be described without a shader in hand, so the same
//! geometry could not be handed to a second shader expecting the same vertices in
//! a different order, and a mesh could not exist before a program did.
//!
//! And a mismatch between what the geometry held and what the shader wanted was
//! not detected: whichever the shader asked for, it got.
//!
//! Here the C++ struct is the truth. A vertex is a plain struct, its fields are
//! described once, and a pipeline checks that description against what the shader
//! actually declares, naming both sides when they disagree.
// ****************************************************************************

namespace compages::gpu
{

// ----------------------------------------------------------------------------
//! \brief The kind of number one component of a field is stored as.
//!
//! Storage, not interpretation: a field stored as UInt8 can reach the shader
//! either as an integer or as a float between 0 and 1, and that is what
//! normalized() decides.
// ----------------------------------------------------------------------------
enum class ScalarType
{
    //! \brief 32 bit floating point, what most fields use.
    Float,
    //! \brief 16 bit floating point. Halves the memory of a position or a
    //! normal, at a precision that is usually enough for normals and colours.
    Half,
    //! \brief 64 bit floating point. Rarely worth it: most GPUs run it slowly.
    Double,
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
};

// ----------------------------------------------------------------------------
//! \brief Size in bytes of one component.
// ----------------------------------------------------------------------------
[[nodiscard]] constexpr std::size_t sizeOf(ScalarType p_type)
{
    switch (p_type)
    {
        case ScalarType::Int8:
        case ScalarType::UInt8:
            return 1u;
        case ScalarType::Half:
        case ScalarType::Int16:
        case ScalarType::UInt16:
            return 2u;
        case ScalarType::Float:
        case ScalarType::Int32:
        case ScalarType::UInt32:
            return 4u;
        case ScalarType::Double:
            return 8u;
    }
    return 0u;
}

// ----------------------------------------------------------------------------
//! \brief Is this a whole number kind?
// ----------------------------------------------------------------------------
[[nodiscard]] constexpr bool isInteger(ScalarType p_type)
{
    switch (p_type)
    {
        case ScalarType::Int8:
        case ScalarType::UInt8:
        case ScalarType::Int16:
        case ScalarType::UInt16:
        case ScalarType::Int32:
        case ScalarType::UInt32:
            return true;
        case ScalarType::Float:
        case ScalarType::Half:
        case ScalarType::Double:
            return false;
    }
    return false;
}

// ----------------------------------------------------------------------------
//! \brief Name of a scalar kind, for error messages.
// ----------------------------------------------------------------------------
[[nodiscard]] constexpr const char* toString(ScalarType p_type)
{
    switch (p_type)
    {
        case ScalarType::Float:
            return "float";
        case ScalarType::Half:
            return "half";
        case ScalarType::Double:
            return "double";
        case ScalarType::Int8:
            return "int8";
        case ScalarType::UInt8:
            return "uint8";
        case ScalarType::Int16:
            return "int16";
        case ScalarType::UInt16:
            return "uint16";
        case ScalarType::Int32:
            return "int32";
        case ScalarType::UInt32:
            return "uint32";
    }
    return "unknown";
}

// ****************************************************************************
//! \brief How one field is stored and how the shader is to read it.
// ****************************************************************************
struct AttributeFormat
{
    //! \brief The kind of number each component is stored as.
    ScalarType scalar = ScalarType::Float;

    //! \brief How many components, from 1 to 4. A vec3 has three.
    std::uint8_t components = 0u;

    //! \brief How many attribute slots the field takes. Always 1 except for
    //! matrices, which the hardware reads one column per slot.
    std::uint8_t slots = 1u;

    //! \brief Should a whole number be turned into a fraction between 0 and 1,
    //! or between -1 and 1 when signed? This is how a colour fits in four bytes
    //! and still arrives in the shader as a vec4.
    bool normalized = false;

    //! \brief Does the shader see whole numbers rather than floating point? True
    //! for an ivec or a uvec attribute.
    bool as_integer = false;

    // ------------------------------------------------------------------------
    //! \brief Size in bytes of the whole field.
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr std::size_t size() const
    {
        return sizeOf(scalar) * components * slots;
    }

    // ------------------------------------------------------------------------
    //! \brief Is this a format the hardware can actually read?
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr bool valid() const
    {
        return (components >= 1u) && (components <= 4u) && (slots >= 1u) &&
               (slots <= 4u) && !(normalized && as_integer);
    }

    // ------------------------------------------------------------------------
    //! \brief The GLSL type a shader must declare for this field, such as
    //! "vec3" or "ivec4". Used to explain a mismatch in words the reader can
    //! look for in the shader source.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string glslType() const;
};

// ****************************************************************************
//! \brief What vertex format a C++ type maps to.
//!
//! Left undefined on purpose, so that using a type nobody taught the library
//! about is a readable compile error rather than a wrong guess. Add a
//! specialization to teach it a type of your own, for instance a vector type
//! coming from another maths library:
//! \code
//! template <>
//! struct compages::gpu::FormatOf<glm::vec3>
//! {
//!     static constexpr AttributeFormat value{ ScalarType::Float, 3u };
//! };
//! \endcode
// ****************************************************************************
template <typename T>
struct FormatOf
{
    static_assert(sizeof(T) == 0,
                  "this type has no known vertex format. Specialize "
                  "compages::gpu::FormatOf<T> to teach the library about it");
};

//! \cond Doxygen_Suppress
template <>
struct FormatOf<float>
{
    static constexpr AttributeFormat value{ ScalarType::Float, 1u };
};

template <>
struct FormatOf<double>
{
    static constexpr AttributeFormat value{ ScalarType::Double, 1u };
};

template <>
struct FormatOf<std::int8_t>
{
    static constexpr AttributeFormat value{
        ScalarType::Int8, 1u, 1u, false, true
    };
};

template <>
struct FormatOf<std::uint8_t>
{
    static constexpr AttributeFormat value{
        ScalarType::UInt8, 1u, 1u, false, true
    };
};

template <>
struct FormatOf<std::int16_t>
{
    static constexpr AttributeFormat value{
        ScalarType::Int16, 1u, 1u, false, true
    };
};

template <>
struct FormatOf<std::uint16_t>
{
    static constexpr AttributeFormat value{
        ScalarType::UInt16, 1u, 1u, false, true
    };
};

template <>
struct FormatOf<std::int32_t>
{
    static constexpr AttributeFormat value{
        ScalarType::Int32, 1u, 1u, false, true
    };
};

template <>
struct FormatOf<std::uint32_t>
{
    static constexpr AttributeFormat value{
        ScalarType::UInt32, 1u, 1u, false, true
    };
};

//! \brief A vector of anything the library already knows becomes that same kind
//! of number, repeated. This is what makes Vector3f work without a
//! specialization of its own.
template <typename T, std::size_t N>
struct FormatOf<Vector<T, N>>
{
    static_assert((N >= 1u) && (N <= 4u),
                  "a vertex field holds at most 4 components");
    static constexpr AttributeFormat value{ FormatOf<T>::value.scalar,
                                            static_cast<std::uint8_t>(N),
                                            1u,
                                            false,
                                            FormatOf<T>::value.as_integer };
};

//! \brief A matrix field, which the hardware reads one slot at a time.
//!
//! Worth having for exactly one reason: a per instance transform. Giving each
//! instance its own model matrix as a field read once per object is what turns a
//! thousand draw calls into one, and it is how 15_SpriteBatch works.
//!
//! On the order of the numbers. The Matrix of src/Math holds its elements row by
//! row, and the hardware reads one slot as one column of the shader's matrix, so
//! the shader sees the transpose of what C++ holds. That is not an oversight and
//! it is not corrected here: the transformation functions of src/Math already
//! build their matrices that way round, which is the same arrangement the uniform
//! path relies on. Reversing it here would make a matrix passed as an attribute
//! disagree with the same matrix passed as a uniform.
template <typename T, std::size_t Rows, std::size_t Cols>
struct FormatOf<Matrix<T, Rows, Cols>>
{
    static_assert((Rows >= 2u) && (Rows <= 4u) && (Cols >= 2u) && (Cols <= 4u),
                  "a matrix vertex field is between 2x2 and 4x4: the hardware "
                  "reads it as up to four slots of up to four components");
    static_assert(!FormatOf<T>::value.as_integer,
                  "a matrix vertex field holds floating point numbers, since "
                  "there is no integer matrix attribute in GLSL");
    static constexpr AttributeFormat value{ FormatOf<T>::value.scalar,
                                            static_cast<std::uint8_t>(Cols),
                                            static_cast<std::uint8_t>(Rows),
                                            false,
                                            false };
};
//! \endcond

// ----------------------------------------------------------------------------
//! \brief The vertex format of a type, ready to be put in a field description.
// ----------------------------------------------------------------------------
template <typename T>
[[nodiscard]] constexpr AttributeFormat formatOf()
{
    return FormatOf<std::remove_cv_t<std::remove_reference_t<T>>>::value;
}

// ****************************************************************************
//! \brief One field of a vertex: where it sits, how it is stored, and what the
//! shader calls it.
// ****************************************************************************
struct FieldDesc
{
    //! \brief The name the shader declares. This is what a pipeline matches
    //! against the attributes it finds in the compiled program.
    std::string name;

    //! \brief How the field is stored and read.
    AttributeFormat format;

    //! \brief Distance in bytes from the start of the vertex. Taken from the
    //! C++ struct itself, never guessed.
    std::uint32_t offset = 0u;

    //! \brief Does this field change per instance rather than per vertex? A per
    //! instance colour is read once per object drawn, not once per corner.
    bool per_instance = false;
};

// ****************************************************************************
//! \brief One field being described, still knowing its C++ type.
//!
//! Exists only so that the modifiers can refuse what does not make sense at
//! compile time rather than at run time: asking for a normalized float is a
//! mistake the compiler can catch.
//!
//! \tparam Member the type of the struct member being described.
// ****************************************************************************
template <typename Member>
class Field
{
public:

    // ------------------------------------------------------------------------
    //! \brief Describe a field sitting at a given offset.
    // ------------------------------------------------------------------------
    Field(std::string p_name, std::uint32_t p_offset)
        : m_desc{ std::move(p_name), formatOf<Member>(), p_offset, false }
    {
    }

    // ------------------------------------------------------------------------
    //! \brief Turn the whole numbers into a fraction between 0 and 1, or -1 and
    //! 1 when signed.
    //!
    //! This is how a colour stored as four bytes reaches the shader as a vec4,
    //! for a quarter of the memory of four floats.
    // ------------------------------------------------------------------------
    [[nodiscard]] Field normalized() &&
    {
        static_assert(isInteger(FormatOf<Member>::value.scalar),
                      "normalized() takes whole numbers and turns them into "
                      "fractions, so it only applies to an integer field. A "
                      "float field is already the fraction");
        m_desc.format.normalized = true;
        // Normalizing means converting to floating point, which is the opposite
        // of being read as a whole number.
        m_desc.format.as_integer = false;
        return std::move(*this);
    }

    // ------------------------------------------------------------------------
    //! \brief Read this field once per instance instead of once per vertex.
    // ------------------------------------------------------------------------
    [[nodiscard]] Field perInstance() &&
    {
        m_desc.per_instance = true;
        return std::move(*this);
    }

    // ------------------------------------------------------------------------
    //! \brief Hand the description over to describe().
    // ------------------------------------------------------------------------
    [[nodiscard]] operator FieldDesc() &&
    {
        return std::move(m_desc);
    }

private:

    FieldDesc m_desc;
};

namespace detail
{

// ----------------------------------------------------------------------------
//! \brief Distance in bytes between the start of a struct and one of its
//! members, obtained from a pointer to that member.
//!
//! Measured on a real object rather than computed, because C++20 offers no way
//! to turn a pointer to member into an offset at compile time. Layouts are built
//! once when a program starts, so measuring costs nothing worth counting.
//!
//! \note The library never guesses an offset. Whatever padding the compiler
//! inserted, this reports where the member truly is, which is what the GPU has
//! to be told.
// ----------------------------------------------------------------------------
template <typename Class, typename Member>
[[nodiscard]] std::uint32_t offsetOf(Member Class::* p_member)
{
    static_assert(std::is_standard_layout_v<Class>,
                  "a vertex struct must be standard layout, otherwise where its "
                  "members sit is not something the GPU can be told. Avoid "
                  "virtual methods and mixed access levels");
    static_assert(std::is_default_constructible_v<Class>,
                  "a vertex struct must be default constructible so that its "
                  "field offsets can be measured");

    const Class probe{};
    const char* const base = reinterpret_cast<const char*>(&probe);
    const char* const field = reinterpret_cast<const char*>(&(probe.*p_member));
    return static_cast<std::uint32_t>(field - base);
}

} // namespace detail

// ----------------------------------------------------------------------------
//! \brief Describe one field of a vertex struct.
//!
//! \code
//! compages::gpu::field(&Vertex::position, "aPosition")
//! compages::gpu::field(&Vertex::color, "aColor").normalized()
//! \endcode
//!
//! The pointer to member is what makes this safe: the field must exist, its type
//! decides the format, and its offset is measured rather than written down. A
//! field renamed in the struct stops compiling instead of silently reading the
//! wrong bytes.
//!
//! \param[in] p_member which member of the struct.
//! \param[in] p_name the name the shader declares for it.
// ----------------------------------------------------------------------------
template <typename Class, typename Member>
[[nodiscard]] Field<Member> field(Member Class::* p_member, std::string p_name)
{
    return Field<Member>(std::move(p_name), detail::offsetOf(p_member));
}

// ****************************************************************************
//! \brief The complete description of one vertex: which fields, where they
//! sit in the struct, and what the shader should call each one.
//!
//! Usually read off the struct itself, with nothing written by hand:
//! \code
//! struct Vertex
//! {
//!     Vector3f position;   // feeds "in vec3 position;"
//!     Vector2f uv;         // feeds "in vec2 uv;"
//! };
//! const compages::gpu::VertexLayout layout = compages::gpu::VertexLayout::of<Vertex>();
//! \endcode
//!
//! The name of a field is the name of the shader attribute it feeds. When the
//! two differ, say so rather than renaming either:
//! \code
//! auto layout = compages::gpu::VertexLayout::of<Sprite>()
//!                   .rename("pos", "aPosition")
//!                   .perInstance();             // one Sprite per instance
//! \endcode
//!
//! A pipeline takes one of these and checks it against the attributes of the
//! compiled shader before agreeing to exist. A field the shader does not read
//! is simply not sent; an attribute the shader reads and no field provides is
//! refused, with both lists in the message.
// ****************************************************************************
class VertexLayout
{
public:

    // ------------------------------------------------------------------------
    //! \brief An empty layout, for the passes that draw without vertex data,
    //! such as a full screen quad generated in the vertex shader.
    // ------------------------------------------------------------------------
    VertexLayout() = default;

    // ------------------------------------------------------------------------
    //! \brief Build from the fields and the size of the C++ struct.
    //!
    //! Prefer describe(), which takes the size from the type itself.
    // ------------------------------------------------------------------------
    VertexLayout(std::vector<FieldDesc> p_fields, std::uint32_t p_stride);

    // ------------------------------------------------------------------------
    //! \brief Read the layout off a plain struct: one field per member, named
    //! as the member is, at the offset the compiler put it.
    //!
    //! \tparam Vertex an aggregate with at most sixteen fields, each of a type
    //! a vertex attribute can hold (float, int, a Vector or a Matrix of those).
    //! What else it must be is listed in FieldReflection.hpp, and anything it
    //! is not stops the compilation with a sentence saying which.
    // ------------------------------------------------------------------------
    template <typename Vertex>
    [[nodiscard]] static VertexLayout of()
    {
        static_assert(std::is_trivially_copyable_v<Vertex>,
                      "a vertex struct is sent to the device as the bytes it "
                      "already is, so it must be trivially copyable");
        std::vector<FieldDesc> fields;
        fields.reserve(reflect::fieldCount<Vertex>());
        const Vertex probe{};
        reflect::forEachField<Vertex>([&](auto p_index) {
            constexpr std::size_t I = decltype(p_index)::value;
            fields.emplace_back(FieldDesc{
                std::string(reflect::fieldName<Vertex, I>()),
                formatOf<reflect::FieldType<Vertex, I>>(),
                static_cast<std::uint32_t>(reflect::fieldOffset<I>(probe)),
                false });
        });
        return VertexLayout(std::move(fields),
                            static_cast<std::uint32_t>(sizeof(Vertex)));
    }

    // ------------------------------------------------------------------------
    //! \brief Feed the shader attribute \c p_attribute from the field called
    //! \c p_field.
    //!
    //! A name that is not a field is reported by validate(), so the mistake
    //! surfaces when the pipeline is built rather than as a missing attribute.
    // ------------------------------------------------------------------------
    VertexLayout& rename(std::string_view p_field, std::string p_attribute) &;
    //! \brief Same, on a temporary, for chaining after of().
    [[nodiscard]] VertexLayout&& rename(std::string_view p_field,
                                        std::string p_attribute) &&
    {
        return std::move(rename(p_field, std::move(p_attribute)));
    }

    // ------------------------------------------------------------------------
    //! \brief Read one field once per instance rather than once per vertex.
    // ------------------------------------------------------------------------
    VertexLayout& perInstance(std::string_view p_field) &;
    //! \brief Same, on a temporary.
    [[nodiscard]] VertexLayout&& perInstance(std::string_view p_field) &&
    {
        return std::move(perInstance(p_field));
    }

    // ------------------------------------------------------------------------
    //! \brief Read every field once per instance: the struct describes one
    //! object, not one corner, as for a sprite drawn a hundred thousand times.
    // ------------------------------------------------------------------------
    VertexLayout& perInstance() &;
    //! \brief Same, on a temporary.
    [[nodiscard]] VertexLayout&& perInstance() &&
    {
        return std::move(perInstance());
    }

    // ------------------------------------------------------------------------
    //! \brief Hand an integer field to the shader as a fraction between 0 and
    //! 1 (or -1 and 1 when signed), as a colour stored in four bytes wants.
    // ------------------------------------------------------------------------
    VertexLayout& normalized(std::string_view p_field) &;
    //! \brief Same, on a temporary.
    [[nodiscard]] VertexLayout&& normalized(std::string_view p_field) &&
    {
        return std::move(normalized(p_field));
    }

    // ------------------------------------------------------------------------
    //! \brief The fields, in the order they were described.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::vector<FieldDesc> const& fields() const
    {
        return m_fields;
    }

    // ------------------------------------------------------------------------
    //! \brief Distance in bytes from one vertex to the next, that is the size of
    //! the C++ struct including whatever padding the compiler added.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::uint32_t stride() const
    {
        return m_stride;
    }

    // ------------------------------------------------------------------------
    //! \brief The field a shader attribute of this name would read, or nullptr.
    // ------------------------------------------------------------------------
    [[nodiscard]] FieldDesc const* find(std::string_view p_name) const;

    // ------------------------------------------------------------------------
    //! \brief How many attribute slots the whole vertex takes. Not the same as
    //! the number of fields, since a matrix field takes several.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t slots() const;

    // ------------------------------------------------------------------------
    //! \brief Is any field read per instance?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool hasPerInstanceFields() const;

    // ------------------------------------------------------------------------
    //! \brief Does this layout describe something the hardware can read?
    //!
    //! Checks what can be known without a shader: that no field runs past the
    //! end of the vertex, that no two fields overlap, that names are not
    //! repeated, and that every format is one the hardware accepts. Whether the
    //! names match a particular shader is checked when a pipeline is created,
    //! since only then is there a shader to compare against.
    //!
    //! Computed once when the layout is built, so asking is free.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status validate() const;

    // ------------------------------------------------------------------------
    //! \brief Has anything been described at all?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool empty() const
    {
        return m_fields.empty();
    }

    // ------------------------------------------------------------------------
    //! \brief A readable dump of the fields, their offsets and their formats.
    //! What to print when a shader and a layout disagree.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string toString() const;

private:

    void check();

    //! \brief The field called p_name, or nullptr after recording why not.
    FieldDesc* edit(std::string_view p_name, const char* p_what);

    std::vector<FieldDesc> m_fields;
    std::uint32_t m_stride = 0u;
    //! \brief Kept from construction so that asking twice costs nothing.
    std::string m_error;
    //! \brief What rename(), perInstance() or normalized() could not do.
    std::string m_edit_error;
};

// ----------------------------------------------------------------------------
//! \brief Describe a vertex struct field by field.
//!
//! \code
//! struct Vertex
//! {
//!     Vector3f position;
//!     Vector3f normal;
//!     Vector2f uv;
//! };
//!
//! static const compages::gpu::VertexLayout LAYOUT = compages::gpu::describe<Vertex>(
//!     compages::gpu::field(&Vertex::position, "aPosition"),
//!     compages::gpu::field(&Vertex::normal, "aNormal"),
//!     compages::gpu::field(&Vertex::uv, "aUV"));
//! \endcode
//!
//! The stride comes from sizeof(Vertex), so the vertices can be interleaved in a
//! single buffer exactly as the struct lays them out, which is what the GPU reads
//! fastest and what the previous design could not express at all.
//!
//! \tparam Vertex the struct describing one vertex.
// ----------------------------------------------------------------------------
template <typename Vertex, typename... Fields>
[[nodiscard]] VertexLayout describe(Fields&&... p_fields)
{
    static_assert(sizeof...(Fields) > 0u,
                  "describe() needs at least one field. Use a default "
                  "constructed VertexLayout to draw without vertex data");
    return VertexLayout(
        std::vector<FieldDesc>{ FieldDesc(std::move(p_fields))... },
        static_cast<std::uint32_t>(sizeof(Vertex)));
}

} // namespace compages::gpu
