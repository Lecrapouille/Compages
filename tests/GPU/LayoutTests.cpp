// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"


#include "Compages/GPU/Core/Layout.hpp"

#include <cstddef>



namespace
{

//! \brief The everyday case: everything about one vertex, side by side.
struct Vertex
{
    compages::core::Vector3f position;
    compages::core::Vector3f normal;
    compages::core::Vector2f uv;
};

//! \brief A vertex packing its colour in four bytes rather than four floats.
struct SmallVertex
{
    compages::core::Vector3f position;
    compages::core::Vector<std::uint8_t, 4u> color;
};

//! \brief A byte followed by a float, which the compiler pads.
struct Padded
{
    std::uint8_t flag;
    float weight;
};

//! \brief The smallest vertex there is.
struct Single
{
    compages::core::Vector2f position;
};

//! \brief As many fields as reflection accepts.
struct Crowded
{
    float a, b, c, d, e, f, g, h, i, j, k, l, m, n, o;
    float last_one;
};

//! \brief A vertex declared inside a class, as the examples do.
struct Outer
{
    struct Nested
    {
        compages::core::Vector3f where;
    };
};

} // namespace

// Formats are deduced from the C++ type, so a field renamed or retyped in the
// struct cannot go on being described as it was.
TEST(Layout, ReadsTheFormatFromTheCppType)
{
    constexpr auto vec3 = compages::gpu::formatOf<compages::core::Vector3f>();
    ASSERT_EQ(vec3.scalar, compages::gpu::ScalarType::Float);
    ASSERT_EQ(vec3.components, 3u);
    ASSERT_EQ(vec3.slots, 1u);
    ASSERT_FALSE(vec3.normalized);
    ASSERT_FALSE(vec3.as_integer);
    ASSERT_EQ(vec3.size(), 12u);

    constexpr auto scalar = compages::gpu::formatOf<float>();
    ASSERT_EQ(scalar.components, 1u);
    ASSERT_EQ(scalar.size(), 4u);
}

// A field of whole numbers reaches the shader as whole numbers unless asked
// otherwise, which is what an index or an identifier per vertex wants.
TEST(Layout, KeepsWholeNumbersWholeByDefault)
{
    constexpr auto ivec = compages::gpu::formatOf<compages::core::Vector<std::int32_t, 2u>>();
    ASSERT_EQ(ivec.scalar, compages::gpu::ScalarType::Int32);
    ASSERT_TRUE(ivec.as_integer);
    ASSERT_FALSE(ivec.normalized);
    ASSERT_EQ(ivec.glslType(), "ivec2");
}

TEST(Layout, NamesTheGlslTypeAShaderMustDeclare)
{
    ASSERT_EQ(compages::gpu::formatOf<float>().glslType(), "float");
    ASSERT_EQ(compages::gpu::formatOf<compages::core::Vector2f>().glslType(), "vec2");
    ASSERT_EQ(compages::gpu::formatOf<compages::core::Vector3f>().glslType(), "vec3");
    ASSERT_EQ(compages::gpu::formatOf<compages::core::Vector4f>().glslType(), "vec4");
    using Vector3u = compages::core::Vector<std::uint32_t, 3u>;
    ASSERT_EQ(compages::gpu::formatOf<Vector3u>().glslType(), "uvec3");
    ASSERT_EQ(compages::gpu::formatOf<std::int32_t>().glslType(), "int");
}

// The offsets must be the ones the compiler chose, padding included, since those
// are the ones the GPU has to be told about.
TEST(Layout, TakesTheOffsetsFromTheStructItself)
{
    const compages::gpu::VertexLayout layout =
        compages::gpu::describe<Vertex>(compages::gpu::field(&Vertex::position, "aPosition"),
                              compages::gpu::field(&Vertex::normal, "aNormal"),
                              compages::gpu::field(&Vertex::uv, "aUV"));

    ASSERT_TRUE(bool(layout.validate())) << layout.validate().error();
    ASSERT_EQ(layout.fields().size(), 3u);
    ASSERT_EQ(layout.stride(), sizeof(Vertex));

    ASSERT_EQ(layout.fields()[0].offset, offsetof(Vertex, position));
    ASSERT_EQ(layout.fields()[1].offset, offsetof(Vertex, normal));
    ASSERT_EQ(layout.fields()[2].offset, offsetof(Vertex, uv));
}

// One buffer holding whole vertices is what the previous design could not
// express: it insisted on one buffer per attribute, so the three fields of a
// vertex ended up far apart in memory.
TEST(Layout, DescribesInterleavedVertices)
{
    const compages::gpu::VertexLayout layout =
        compages::gpu::describe<Vertex>(compages::gpu::field(&Vertex::position, "aPosition"),
                              compages::gpu::field(&Vertex::normal, "aNormal"),
                              compages::gpu::field(&Vertex::uv, "aUV"));

    // All three fields step by the size of a whole vertex, which is exactly what
    // interleaved means.
    ASSERT_EQ(layout.stride(), 32u);
    ASSERT_EQ(layout.fields()[0].offset, 0u);
    ASSERT_EQ(layout.fields()[1].offset, 12u);
    ASSERT_EQ(layout.fields()[2].offset, 24u);
}

TEST(Layout, FindsAFieldByTheNameTheShaderUses)
{
    const compages::gpu::VertexLayout layout =
        compages::gpu::describe<Vertex>(compages::gpu::field(&Vertex::position, "aPosition"),
                              compages::gpu::field(&Vertex::uv, "aUV"));

    ASSERT_NE(layout.find("aPosition"), nullptr);
    ASSERT_EQ(layout.find("aPosition")->offset, offsetof(Vertex, position));
    ASSERT_EQ(layout.find("aNormal"), nullptr);
}

// A colour in four bytes instead of four floats saves three quarters of the
// memory and still arrives as a vec4.
TEST(Layout, TurnsBytesIntoFractionsWhenAsked)
{
    const compages::gpu::VertexLayout layout = compages::gpu::describe<SmallVertex>(
        compages::gpu::field(&SmallVertex::position, "aPosition"),
        compages::gpu::field(&SmallVertex::color, "aColor").normalized());

    ASSERT_TRUE(bool(layout.validate())) << layout.validate().error();

    compages::gpu::FieldDesc const* color = layout.find("aColor");
    ASSERT_NE(color, nullptr);
    ASSERT_EQ(color->format.scalar, compages::gpu::ScalarType::UInt8);
    ASSERT_TRUE(color->format.normalized);
    // Normalizing means arriving as floating point, so not as a whole number.
    ASSERT_FALSE(color->format.as_integer);
    ASSERT_EQ(color->format.glslType(), "vec4");
    ASSERT_EQ(color->format.size(), 4u);
}

TEST(Layout, MarksTheFieldsReadOncePerInstance)
{
    const compages::gpu::VertexLayout layout = compages::gpu::describe<Vertex>(
        compages::gpu::field(&Vertex::position, "aPosition"),
        compages::gpu::field(&Vertex::normal, "aOffset").perInstance());

    ASSERT_TRUE(layout.hasPerInstanceFields());
    ASSERT_FALSE(layout.find("aPosition")->per_instance);
    ASSERT_TRUE(layout.find("aOffset")->per_instance);
}

TEST(Layout, CountsTheAttributeSlotsTheVertexTakes)
{
    const compages::gpu::VertexLayout layout =
        compages::gpu::describe<Vertex>(compages::gpu::field(&Vertex::position, "aPosition"),
                              compages::gpu::field(&Vertex::normal, "aNormal"),
                              compages::gpu::field(&Vertex::uv, "aUV"));

    ASSERT_EQ(layout.slots(), 3u);
}

// Reading the layout off the struct must produce exactly what writing the
// fields out by hand produces, with the member names as the shader names.
TEST(Layout, ReadOffTheStructDescribesTheSameThingAsTheExplicitForm)
{
    const compages::gpu::VertexLayout read = compages::gpu::VertexLayout::of<Vertex>();
    const compages::gpu::VertexLayout explicit_form =
        compages::gpu::describe<Vertex>(compages::gpu::field(&Vertex::position, "position"),
                              compages::gpu::field(&Vertex::normal, "normal"),
                              compages::gpu::field(&Vertex::uv, "uv"));

    ASSERT_TRUE(bool(read.validate())) << read.validate().error();
    ASSERT_EQ(read.stride(), explicit_form.stride());
    ASSERT_EQ(read.fields().size(), explicit_form.fields().size());
    for (std::size_t i = 0u; i < read.fields().size(); ++i)
    {
        ASSERT_EQ(read.fields()[i].name, explicit_form.fields()[i].name);
        ASSERT_EQ(read.fields()[i].offset, explicit_form.fields()[i].offset);
        ASSERT_EQ(read.fields()[i].format.components,
                  explicit_form.fields()[i].format.components);
    }
}

// The compiler finds the count, the names and the types of the fields, at
// compile time: these are static_asserts, not run time checks.
TEST(Layout, ReflectionFindsNamesAndTypesAtCompileTime)
{
    static_assert(compages::gpu::reflect::fieldCount<Vertex>() == 3u);
    static_assert(compages::gpu::reflect::fieldName<Vertex, 0u>() == "position");
    static_assert(compages::gpu::reflect::fieldName<Vertex, 2u>() == "uv");
    static_assert(std::is_same_v<compages::gpu::reflect::FieldType<Vertex, 1u>, compages::core::Vector3f>);
    static_assert(compages::gpu::reflect::fieldCount<Crowded>() == 16u);
    static_assert(compages::gpu::reflect::fieldName<Crowded, 15u>() == "last_one");
    static_assert(compages::gpu::reflect::fieldName<Outer::Nested, 0u>() == "where");
    SUCCEED();
}

// Offsets come from the compiler too, padding included.
TEST(Layout, ReadOffTheStructFindsPaddedOffsets)
{
    const compages::gpu::VertexLayout layout = compages::gpu::VertexLayout::of<Padded>();

    ASSERT_EQ(layout.stride(), sizeof(Padded));
    ASSERT_EQ(layout.fields()[0].name, "flag");
    ASSERT_EQ(layout.fields()[1].name, "weight");
    ASSERT_EQ(layout.fields()[1].offset, offsetof(Padded, weight));
    ASSERT_EQ(layout.fields()[1].format.scalar, compages::gpu::ScalarType::Float);
    ASSERT_TRUE(layout.fields()[0].format.as_integer);
}

TEST(Layout, ReadOffTheStructWorksWithASingleField)
{
    const compages::gpu::VertexLayout layout = compages::gpu::VertexLayout::of<Single>();

    ASSERT_EQ(layout.fields().size(), 1u);
    ASSERT_EQ(layout.fields()[0].name, "position");
    ASSERT_EQ(layout.stride(), sizeof(Single));
}

// When the shader calls it something else, the field is renamed, not the
// struct.
TEST(Layout, RenamesAFieldToTheNameTheShaderUses)
{
    const compages::gpu::VertexLayout layout =
        compages::gpu::VertexLayout::of<Vertex>().rename("position", "aPosition");

    ASSERT_TRUE(bool(layout.validate())) << layout.validate().error();
    ASSERT_NE(layout.find("aPosition"), nullptr);
    ASSERT_EQ(layout.find("position"), nullptr);
}

// A misspelled field is reported when the layout is checked, with the fields
// it could have meant.
TEST(Layout, SaysWhichFieldsExistWhenRenamingOneThatDoesNot)
{
    const compages::gpu::VertexLayout layout =
        compages::gpu::VertexLayout::of<Vertex>().rename("pos", "aPosition");

    auto checked = layout.validate();
    ASSERT_FALSE(checked);
    EXPECT_THAT(checked.error(), HasSubstr("'pos'"));
    EXPECT_THAT(checked.error(), HasSubstr("position, normal, uv"));
}

// Renaming a field onto the name of another one is two fields of one name.
TEST(Layout, RefusesARenameThatCollidesWithAnotherField)
{
    const compages::gpu::VertexLayout layout =
        compages::gpu::VertexLayout::of<Vertex>().rename("normal", "position");

    auto checked = layout.validate();
    ASSERT_FALSE(checked);
    EXPECT_THAT(checked.error(), HasSubstr("two fields are both named"));
}

TEST(Layout, MarksFieldsPerInstanceByNameOrAllAtOnce)
{
    const compages::gpu::VertexLayout one =
        compages::gpu::VertexLayout::of<Vertex>().perInstance("uv");
    ASSERT_FALSE(one.find("position")->per_instance);
    ASSERT_TRUE(one.find("uv")->per_instance);

    const compages::gpu::VertexLayout all = compages::gpu::VertexLayout::of<Vertex>().perInstance();
    ASSERT_TRUE(all.find("position")->per_instance);
    ASSERT_TRUE(all.find("normal")->per_instance);
    ASSERT_TRUE(all.hasPerInstanceFields());
}

TEST(Layout, NormalizesAnIntegerFieldByName)
{
    const compages::gpu::VertexLayout layout =
        compages::gpu::VertexLayout::of<SmallVertex>().normalized("color");

    ASSERT_TRUE(bool(layout.validate())) << layout.validate().error();
    ASSERT_TRUE(layout.find("color")->format.normalized);
    ASSERT_FALSE(layout.find("color")->format.as_integer);
}

TEST(Layout, RefusesToNormalizeAFloatField)
{
    const compages::gpu::VertexLayout layout =
        compages::gpu::VertexLayout::of<SmallVertex>().normalized("position");

    auto checked = layout.validate();
    ASSERT_FALSE(checked);
    EXPECT_THAT(checked.error(), HasSubstr("whole numbers"));
}

// Nothing forces the fields to be described in the order they appear, so the
// checks must not assume it either.
TEST(Layout, AcceptsFieldsDescribedOutOfOrder)
{
    const compages::gpu::VertexLayout layout =
        compages::gpu::describe<Vertex>(compages::gpu::field(&Vertex::uv, "aUV"),
                              compages::gpu::field(&Vertex::position, "aPosition"));

    ASSERT_TRUE(bool(layout.validate())) << layout.validate().error();
    ASSERT_EQ(layout.fields()[0].name, "aUV");
    ASSERT_EQ(layout.fields()[0].offset, offsetof(Vertex, uv));
}

// Describing the same member twice would have two shader attributes reading the
// same bytes, which is never what was meant.
TEST(Layout, RefusesTwoFieldsSharingTheSameBytes)
{
    const compages::gpu::VertexLayout layout =
        compages::gpu::describe<Vertex>(compages::gpu::field(&Vertex::position, "aPosition"),
                              compages::gpu::field(&Vertex::position, "aSomethingElse"));

    auto status = layout.validate();
    ASSERT_FALSE(bool(status));
    ASSERT_THAT(status.error(), HasSubstr("overlap"));
    ASSERT_THAT(status.error(), HasSubstr("aPosition"));
    ASSERT_THAT(status.error(), HasSubstr("aSomethingElse"));
}

TEST(Layout, RefusesTwoFieldsWithTheSameName)
{
    const compages::gpu::VertexLayout layout =
        compages::gpu::describe<Vertex>(compages::gpu::field(&Vertex::position, "aThing"),
                              compages::gpu::field(&Vertex::uv, "aThing"));

    auto status = layout.validate();
    ASSERT_FALSE(bool(status));
    ASSERT_THAT(status.error(), HasSubstr("aThing"));
}

// Describing a member of one struct as belonging to another is a copy and paste
// mistake worth catching, and it shows up as a field running off the end.
TEST(Layout, RefusesAFieldRunningPastTheEndOfTheVertex)
{
    // A field the size of a compages::core::Vector3f, placed where only 4 bytes remain.
    std::vector<compages::gpu::FieldDesc> fields{
        compages::gpu::FieldDesc{ "aThing", compages::gpu::formatOf<compages::core::Vector3f>(), 4u, false }
    };
    const compages::gpu::VertexLayout layout(std::move(fields), 8u);

    auto status = layout.validate();
    ASSERT_FALSE(bool(status));
    ASSERT_THAT(status.error(), HasSubstr("only 8 bytes"));
}

TEST(Layout, RefusesAFieldWithoutAName)
{
    std::vector<compages::gpu::FieldDesc> fields{
        compages::gpu::FieldDesc{ "", compages::gpu::formatOf<compages::core::Vector3f>(), 0u, false }
    };
    const compages::gpu::VertexLayout layout(std::move(fields), 12u);

    auto status = layout.validate();
    ASSERT_FALSE(bool(status));
    ASSERT_THAT(status.error(), HasSubstr("no name"));
}

TEST(Layout, RefusesAFormatWithTooManyComponents)
{
    compages::gpu::AttributeFormat format = compages::gpu::formatOf<compages::core::Vector4f>();
    format.components = 5u;
    ASSERT_FALSE(format.valid());

    std::vector<compages::gpu::FieldDesc> fields{
        compages::gpu::FieldDesc{ "aThing", format, 0u, false }
    };
    const compages::gpu::VertexLayout layout(std::move(fields), 64u);

    ASSERT_FALSE(bool(layout.validate()));
    ASSERT_THAT(layout.validate().error(), HasSubstr("cannot read"));
}

// Drawing without any vertex data is legitimate: a full screen effect generates
// its corners in the vertex shader.
TEST(Layout, AcceptsHavingNoFieldsAtAll)
{
    const compages::gpu::VertexLayout layout;

    ASSERT_TRUE(layout.empty());
    ASSERT_TRUE(bool(layout.validate()));
    ASSERT_EQ(layout.stride(), 0u);
    ASSERT_EQ(layout.slots(), 0u);
}

// This text is what a reader sees when a shader and a layout disagree, so it must
// contain the things they need to compare.
TEST(Layout, PrintsSomethingWorthReading)
{
    const compages::gpu::VertexLayout layout = compages::gpu::describe<SmallVertex>(
        compages::gpu::field(&SmallVertex::position, "aPosition"),
        compages::gpu::field(&SmallVertex::color, "aColor").normalized());

    const std::string text = layout.toString();

    ASSERT_THAT(text, HasSubstr("aPosition"));
    ASSERT_THAT(text, HasSubstr("vec3"));
    ASSERT_THAT(text, HasSubstr("aColor"));
    ASSERT_THAT(text, HasSubstr("normalized"));
    ASSERT_THAT(text, HasSubstr(std::to_string(sizeof(SmallVertex))));
}
