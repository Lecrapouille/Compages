//==============================================================================
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
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//==============================================================================

#include "GPUContext.hpp"

#include "Compages/GPU/GPU.hpp"

using namespace tests;

namespace
{

// ****************************************************************************
//! \brief The vertex of a lit, textured mesh: the case the whole design is for.
// ****************************************************************************
struct Mesh
{
    Vector3f position;
    Vector3f normal;
    Vector2f uv;
};

// ****************************************************************************
//! \brief A vertex whose colour is four bytes rather than four floats, which is
//! what normalized() is for.
// ****************************************************************************
struct Sprite
{
    Vector2f position;
    Vector<std::uint8_t, 4u> color;
};

// ****************************************************************************
//! \brief One instance of a sprite, carrying its own transform.
// ****************************************************************************
struct Instance
{
    Matrix44f model;
};

//! \brief Reads all three fields of a Mesh. The forward pass.
constexpr const char* FORWARD_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;
in vec2 uv;

out vec3 vNormal;
out vec2 vUV;

void main()
{
    vNormal = normal;
    vUV = uv;
    gl_Position = vec4(position, 1.0);
}
)";

//! \brief Reads only the position of a Mesh. The shadow pass, and the proof that
//! one buffer feeds several shaders.
constexpr const char* DEPTH_ONLY_VERTEX = R"(#version 450 core
in vec3 position;
void main() { gl_Position = vec4(position, 1.0); }
)";

//! \brief Asks for a field the Mesh does not have.
constexpr const char* WRONG_NAME_VERTEX = R"(#version 450 core
in vec3 aPosition;
void main() { gl_Position = vec4(aPosition, 1.0); }
)";

//! \brief Reads the position as a vec2, which a Vector3f field cannot feed.
constexpr const char* NARROW_VERTEX = R"(#version 450 core
in vec2 position;
void main() { gl_Position = vec4(position, 0.0, 1.0); }
)";

//! \brief Reads the colour as whole numbers rather than as fractions.
constexpr const char* INTEGER_COLOR_VERTEX = R"(#version 450 core
in vec2 position;
in uvec4 color;

flat out uvec4 vColor;

void main()
{
    vColor = color;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

//! \brief Reads the colour as fractions between 0 and 1.
constexpr const char* FLOAT_COLOR_VERTEX = R"(#version 450 core
in vec2 position;
in vec4 color;

out vec4 vColor;

void main()
{
    vColor = color;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

//! \brief A per instance transform, spread over four attribute slots.
constexpr const char* INSTANCED_VERTEX = R"(#version 450 core
in mat4 model;
void main() { gl_Position = model * vec4(0.0, 0.0, 0.0, 1.0); }
)";

//! \brief Generates its own vertices, so it needs no layout at all. What a full
//! screen pass looks like.
constexpr const char* NO_VERTEX_INPUT = R"(#version 450 core
void main()
{
    vec2 corner = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT = R"(#version 450 core
out vec4 oColor;
void main() { oColor = vec4(1.0); }
)";

//! \brief Uses the normal and the texture coordinates the vertex stage produced.
//!
//! It has to. A linker is free to drop a vertex attribute whose value never
//! reaches the picture, and then the attribute is not declared as far as
//! introspection is concerned. Pairing FORWARD_VERTEX with a fragment shader that
//! ignores its outputs would leave a program reading position alone.
constexpr const char* LIT_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
in vec2 vUV;
out vec4 oColor;
void main() { oColor = vec4(normalize(vNormal) * 0.5 + 0.5, vUV.x + vUV.y); }
)";

constexpr const char* FLAT_FRAGMENT = R"(#version 450 core
flat in uvec4 vColor;
out vec4 oColor;
void main() { oColor = vec4(vColor) / 255.0; }
)";

constexpr const char* COLOR_FRAGMENT = R"(#version 450 core
in vec4 vColor;
out vec4 oColor;
void main() { oColor = vColor; }
)";

constexpr const char* COMPUTE = R"(#version 450 core
layout(local_size_x = 8) in;
layout(std430, binding = 0) buffer Values { float values[]; };
void main() { values[gl_GlobalInvocationID.x] *= 2.0; }
)";

//! \brief What a Mesh looks like, using the names the shaders declare.
compages::gpu::VertexLayout meshLayout()
{
    return compages::gpu::VertexLayout::of<Mesh>();
}

//! \brief A Sprite whose colour arrives as fractions between 0 and 1.
compages::gpu::VertexLayout spriteLayoutNormalized()
{
    return compages::gpu::describe<Sprite>(
        compages::gpu::field(&Sprite::position, "position"),
        compages::gpu::field(&Sprite::color, "color").normalized());
}

//! \brief A Sprite whose colour arrives as whole numbers.
compages::gpu::VertexLayout spriteLayoutInteger()
{
    return compages::gpu::VertexLayout::of<Sprite>();
}

//! \brief Link a pair of sources, failing the test loudly when they will not.
compages::gpu::Program linked(const char* p_vertex, const char* p_fragment)
{
    auto program = compages::gpu::Program::fromSources(p_vertex, p_fragment);
    EXPECT_TRUE(bool(program)) << program.error();
    return program ? program.take() : compages::gpu::Program{};
}

} // namespace

// ****************************************************************************
//! \brief A live device, on an invisible context.
// ****************************************************************************
class PipelineTest: public GPUTest
{
protected:

    void SetUp() override
    {
        GPUTest::SetUp();
        auto ready = compages::gpu::init(GPUContext::procAddress());
        ASSERT_TRUE(bool(ready)) << ready.error();
    }

    void TearDown() override
    {
        compages::gpu::shutdown();
        GPUTest::TearDown();
    }
};

//------------------------------------------------------------------------------
TEST_F(PipelineTest, AcceptsAShaderTheVertexCanFeed)
{
    compages::gpu::Program program = linked(FORWARD_VERTEX, LIT_FRAGMENT);

    auto created = compages::gpu::Pipeline::create<Mesh>(program, meshLayout());
    ASSERT_TRUE(bool(created)) << created.error();

    auto pipeline = created.take();
    ASSERT_TRUE(pipeline.valid());
    ASSERT_EQ(pipeline.program(), program.handle());
    ASSERT_EQ(pipeline.stride(), sizeof(Mesh));
    ASSERT_FALSE(pipeline.instanced());
    ASSERT_EQ(compages::gpu::livePipelines(), 1u);

    // All three fields are read, each at the slot the driver chose for it.
    const std::string described = pipeline.describeAttributes();
    ASSERT_THAT(described, HasSubstr("vec3 position at byte 0"));
    ASSERT_THAT(described, HasSubstr("vec3 normal at byte 12"));
    ASSERT_THAT(described, HasSubstr("vec2 uv at byte 24"));
}

//------------------------------------------------------------------------------
// The whole reason the layer was rewritten. One buffer of vertices, three passes
// reading different parts of it, and a VAO that used to belong to one program
// forever.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, LetsOneVertexFeedSeveralShaders)
{
    compages::gpu::Program forward = linked(FORWARD_VERTEX, LIT_FRAGMENT);
    compages::gpu::Program shadow = linked(DEPTH_ONLY_VERTEX, FRAGMENT);

    const compages::gpu::VertexLayout layout = meshLayout();

    auto lit = compages::gpu::Pipeline::create<Mesh>(
        forward, layout, { .depth_test = true, .cull = compages::gpu::CullMode::Back });
    ASSERT_TRUE(bool(lit)) << lit.error();

    auto depth = compages::gpu::Pipeline::create<Mesh>(
        shadow,
        layout,
        { .depth_test = true, .color_mask = compages::gpu::ColorMask::none() });
    ASSERT_TRUE(bool(depth)) << depth.error();

    auto wireframe = compages::gpu::Pipeline::create<Mesh>(
        shadow, layout, { .polygon = compages::gpu::PolygonMode::Line });
    ASSERT_TRUE(bool(wireframe)) << wireframe.error();

    ASSERT_EQ(compages::gpu::livePipelines(), 3u);

    // The two passes reading only the position read a vertex the same way, so
    // they share one description; the one reading all three fields needs its own.
    ASSERT_EQ(compages::gpu::vertexReadersHeld(), 2u);
}

//------------------------------------------------------------------------------
// A shadow pass reads only the position and simply ignores the rest of the
// vertex, which is what the previous design could not express.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, IgnoresTheFieldsAShaderDoesNotRead)
{
    compages::gpu::Program program = linked(DEPTH_ONLY_VERTEX, FRAGMENT);

    auto created = compages::gpu::Pipeline::create<Mesh>(program, meshLayout());
    ASSERT_TRUE(bool(created)) << created.error();

    const std::string described = created.value().describeAttributes();
    ASSERT_THAT(described, HasSubstr("position"));
    ASSERT_THAT(described, HasSubstr("not read by this shader: vec3 normal"));
    ASSERT_THAT(described, HasSubstr("not read by this shader: vec2 uv"));
}

//------------------------------------------------------------------------------
// A vertex attribute whose value never reaches the picture is dropped by the
// linker, and the pipeline then does not send it. Worth a test of its own,
// because it looks exactly like a bug: the shader source plainly reads uv, yet
// nothing is sent for it. The cause is that the fragment shader ignores what the
// vertex shader computed from it.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, DoesNotSendWhatTheLinkerDroppedAlongTheWay)
{
    // FORWARD_VERTEX reads position, normal and uv, but this fragment shader uses
    // none of what it passes on.
    compages::gpu::Program program = linked(FORWARD_VERTEX, FRAGMENT);

    auto created = compages::gpu::Pipeline::create<Mesh>(program, meshLayout());
    ASSERT_TRUE(bool(created)) << created.error();

    ASSERT_THAT(created.value().describeAttributes(),
                HasSubstr("not read by this shader: vec3 normal"));

    // And with a fragment shader that does use them, all three are sent.
    compages::gpu::Program lit = linked(FORWARD_VERTEX, LIT_FRAGMENT);
    auto other = compages::gpu::Pipeline::create<Mesh>(lit, meshLayout());
    ASSERT_TRUE(bool(other)) << other.error();
    ASSERT_THAT(other.value().describeAttributes(),
                Not(HasSubstr("not read by this shader")));
}

//------------------------------------------------------------------------------
// The mismatch that used to draw nothing at all, with no message. Both names have
// to appear, and so does the way to fix it.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, NamesTheAttributeTheVertexDoesNotHave)
{
    compages::gpu::Program program = linked(WRONG_NAME_VERTEX, FRAGMENT);

    auto created = compages::gpu::Pipeline::create<Mesh>(program, meshLayout());
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("'aPosition'"));
    ASSERT_THAT(created.error(), HasSubstr("vec3 position"));
    ASSERT_THAT(created.error(), HasSubstr(".rename("));
    ASSERT_EQ(compages::gpu::livePipelines(), 0u);
    ASSERT_EQ(compages::gpu::vertexReadersHeld(), 0u);
}

//------------------------------------------------------------------------------
// A vec3 field feeding a vec2 attribute would quietly drop the third component.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, RefusesAFieldOfTheWrongWidth)
{
    compages::gpu::Program program = linked(NARROW_VERTEX, FRAGMENT);

    auto created = compages::gpu::Pipeline::create<Mesh>(program, meshLayout());
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("wants 2 components"));
    ASSERT_THAT(created.error(), HasSubstr("field holds 3"));
}

//------------------------------------------------------------------------------
// Four bytes reaching the shader as fractions between 0 and 1, which is how a
// colour costs a quarter of what four floats would.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, AcceptsWholeNumbersTurnedIntoFractions)
{
    compages::gpu::Program program = linked(FLOAT_COLOR_VERTEX, COLOR_FRAGMENT);

    auto created =
        compages::gpu::Pipeline::create<Sprite>(program, spriteLayoutNormalized());
    ASSERT_TRUE(bool(created)) << created.error();
}

//------------------------------------------------------------------------------
// The same bytes read as whole numbers, which is a different thing entirely.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, AcceptsWholeNumbersLeftAsWholeNumbers)
{
    compages::gpu::Program program = linked(INTEGER_COLOR_VERTEX, FLAT_FRAGMENT);

    auto created = compages::gpu::Pipeline::create<Sprite>(program, spriteLayoutInteger());
    ASSERT_TRUE(bool(created)) << created.error();
}

//------------------------------------------------------------------------------
// Whole numbers arriving at a float attribute do not become 0.0 to 1.0 by
// themselves: they arrive as 0.0 to 255.0, which looks like a colour gone white.
// This is the mistake normalized() exists for, so the message has to say its name.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, SaysWhenWholeNumbersNeedTurningIntoFractions)
{
    compages::gpu::Program program = linked(FLOAT_COLOR_VERTEX, COLOR_FRAGMENT);

    auto created = compages::gpu::Pipeline::create<Sprite>(program, spriteLayoutInteger());
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("normalized()"));
}

//------------------------------------------------------------------------------
// And the other way round: fractions arriving at an integer attribute are read as
// unrelated values, since the bytes of a float mean nothing as an integer.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, SaysWhenFractionsCannotFeedWholeNumbers)
{
    compages::gpu::Program program = linked(INTEGER_COLOR_VERTEX, FLAT_FRAGMENT);

    auto created =
        compages::gpu::Pipeline::create<Sprite>(program, spriteLayoutNormalized());
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("Drop normalized()"));
}

//------------------------------------------------------------------------------
// A per instance matrix, which is what turns a thousand draw calls into one. It
// spans four attribute slots, and the pipeline knows it is instanced.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, CarriesAPerInstanceTransform)
{
    compages::gpu::Program program = linked(INSTANCED_VERTEX, FRAGMENT);

    const compages::gpu::VertexLayout layout = compages::gpu::describe<Instance>(
        compages::gpu::field(&Instance::model, "model").perInstance());

    auto created = compages::gpu::Pipeline::create<Instance>(program, layout);
    ASSERT_TRUE(bool(created)) << created.error();
    ASSERT_TRUE(created.value().instanced());
    ASSERT_THAT(created.value().describeAttributes(),
                HasSubstr("once per instance"));
}

//------------------------------------------------------------------------------
// A pass that builds its own vertices from the vertex index needs no layout, and
// giving it one is a misunderstanding worth naming.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, DrawsWithoutAnyVertexData)
{
    compages::gpu::Program program = linked(NO_VERTEX_INPUT, FRAGMENT);

    auto created = compages::gpu::Pipeline::create(program, compages::gpu::VertexLayout{});
    ASSERT_TRUE(bool(created)) << created.error();
    ASSERT_EQ(created.value().stride(), 0u);
    ASSERT_THAT(created.value().describeAttributes(),
                HasSubstr("generates its own vertices"));
}

//------------------------------------------------------------------------------
TEST_F(PipelineTest, SaysSoWhenAShaderReadsNoVertexAtAll)
{
    compages::gpu::Program program = linked(NO_VERTEX_INPUT, FRAGMENT);

    auto created = compages::gpu::Pipeline::create<Mesh>(program, meshLayout());
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("reads no vertex attributes"));
    ASSERT_THAT(created.error(), HasSubstr("default constructed"));
}

//------------------------------------------------------------------------------
// Passing the layout of one struct while drawing from a buffer of another reads
// the right number of bytes from the wrong places, so it draws something. Caught
// by the size, which is the one part of it that shows.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, RefusesALayoutBelongingToAnotherVertex)
{
    compages::gpu::Program program = linked(DEPTH_ONLY_VERTEX, FRAGMENT);

    auto created = compages::gpu::Pipeline::create<Sprite>(program, meshLayout());
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("belongs to another vertex struct"));
}

//------------------------------------------------------------------------------
// A layout whose fields overlap is refused before any shader is consulted, since
// it cannot be right for any shader.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, RefusesALayoutThatIsWrongOnItsOwn)
{
    compages::gpu::Program program = linked(FORWARD_VERTEX, LIT_FRAGMENT);

    // Two fields claiming the same bytes: normal described where position sits.
    const compages::gpu::VertexLayout broken = compages::gpu::describe<Mesh>(
        compages::gpu::field(&Mesh::position, "position"),
        compages::gpu::field(&Mesh::position, "normal"),
        compages::gpu::field(&Mesh::uv, "uv"));

    auto created = compages::gpu::Pipeline::create<Mesh>(program, broken);
    ASSERT_FALSE(bool(created));
}

//------------------------------------------------------------------------------
// A compute program is dispatched, never fed vertices, so pairing one with a
// layout is a mistake about what kind of program it is.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, RefusesAComputeProgram)
{
    auto program = compages::gpu::Program::fromComputeSource(COMPUTE);
    ASSERT_TRUE(bool(program)) << program.error();

    auto created =
        compages::gpu::Pipeline::create(program.value(), compages::gpu::VertexLayout{});
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("compute program"));
}

//------------------------------------------------------------------------------
TEST_F(PipelineTest, RefusesAProgramThatWasNeverLinked)
{
    auto created = compages::gpu::Pipeline::create(compages::gpu::Program{}, compages::gpu::VertexLayout{});
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("has not been linked"));
}

//------------------------------------------------------------------------------
// Outlining a point means nothing, and drawing it anyway would look like the
// polygon mode had no effect.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, RefusesToOutlineAPoint)
{
    compages::gpu::Program program = linked(DEPTH_ONLY_VERTEX, FRAGMENT);

    auto created = compages::gpu::Pipeline::create<Mesh>(
        program,
        meshLayout(),
        { .primitive = compages::gpu::Primitive::Points,
          .polygon = compages::gpu::PolygonMode::Line });
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("no edges"));
}

//------------------------------------------------------------------------------
// The state is kept whole and given back as it was asked for, since that is what
// makes a pipeline a complete description of one way of drawing.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, KeepsTheStateItWasGiven)
{
    compages::gpu::Program program = linked(DEPTH_ONLY_VERTEX, FRAGMENT);

    const compages::gpu::RenderState asked{ .primitive = compages::gpu::Primitive::TriangleStrip,
                                  .depth_test = true,
                                  .depth_write = false,
                                  .depth_func = compages::gpu::CompareFunc::LessEqual,
                                  .blend = compages::gpu::Blend::additive(),
                                  .cull = compages::gpu::CullMode::Front };

    auto created = compages::gpu::Pipeline::create<Mesh>(program, meshLayout(), asked);
    ASSERT_TRUE(bool(created)) << created.error();
    ASSERT_TRUE(created.value().state() == asked);
}

//------------------------------------------------------------------------------
TEST_F(PipelineTest, AppliesItsStateWhenBound)
{
    compages::gpu::Program program = linked(FORWARD_VERTEX, LIT_FRAGMENT);

    auto opaque = compages::gpu::Pipeline::create<Mesh>(
        program,
        meshLayout(),
        { .depth_test = true, .cull = compages::gpu::CullMode::Back });
    ASSERT_TRUE(bool(opaque)) << opaque.error();

    auto transparent = compages::gpu::Pipeline::create<Mesh>(
        program,
        meshLayout(),
        { .depth_test = true,
          .depth_write = false,
          .blend = compages::gpu::Blend::alpha() });
    ASSERT_TRUE(bool(transparent)) << transparent.error();

    // Bound one after the other, which is what a frame does, and twice over to
    // exercise the path that skips what already holds.
    for (int pass = 0; pass < 2; ++pass)
    {
        auto first = opaque.value().bind();
        ASSERT_TRUE(bool(first)) << first.error();
        auto second = transparent.value().bind();
        ASSERT_TRUE(bool(second)) << second.error();
    }
}

//------------------------------------------------------------------------------
// A pipeline names its program rather than owning it, so the program going away
// first has to be noticed rather than crashed on.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, NoticesWhenItsProgramHasGone)
{
    compages::gpu::Program program = linked(DEPTH_ONLY_VERTEX, FRAGMENT);

    auto created = compages::gpu::Pipeline::create<Mesh>(program, meshLayout());
    ASSERT_TRUE(bool(created)) << created.error();
    auto pipeline = created.take();

    ASSERT_TRUE(bool(pipeline.bind()));

    program.release();

    auto bound = pipeline.bind();
    ASSERT_FALSE(bool(bound));
    ASSERT_THAT(bound.error(), HasSubstr("has been released"));
}

//------------------------------------------------------------------------------
// The counter an overlay watches: pipelines sharing a way of reading a vertex
// must not each hold one, and the last one leaving must let it go.
//------------------------------------------------------------------------------
TEST_F(PipelineTest, SharesOneWayOfReadingAVertex)
{
    compages::gpu::Program program = linked(FORWARD_VERTEX, LIT_FRAGMENT);
    const compages::gpu::VertexLayout layout = meshLayout();

    ASSERT_EQ(compages::gpu::vertexReadersHeld(), 0u);
    {
        auto first = compages::gpu::Pipeline::create<Mesh>(program, layout).take();
        ASSERT_EQ(compages::gpu::vertexReadersHeld(), 1u);
        {
            auto second =
                compages::gpu::Pipeline::create<Mesh>(program,
                                            layout,
                                            { .depth_test = true })
                    .take();
            // A different state, the same way of reading a vertex.
            ASSERT_EQ(compages::gpu::vertexReadersHeld(), 1u);
            ASSERT_EQ(compages::gpu::livePipelines(), 2u);
        }
        // One of the two gone, but the other still needs it.
        ASSERT_EQ(compages::gpu::vertexReadersHeld(), 1u);
        ASSERT_EQ(compages::gpu::livePipelines(), 1u);
    }
    ASSERT_EQ(compages::gpu::vertexReadersHeld(), 0u);
    ASSERT_EQ(compages::gpu::livePipelines(), 0u);
}

//------------------------------------------------------------------------------
TEST_F(PipelineTest, HandsThePipelineOverWhenMoved)
{
    compages::gpu::Program program = linked(DEPTH_ONLY_VERTEX, FRAGMENT);

    auto first = compages::gpu::Pipeline::create<Mesh>(program, meshLayout()).take();
    const compages::gpu::PipelineHandle handle = first.handle();

    compages::gpu::Pipeline second = std::move(first);
    ASSERT_EQ(compages::gpu::livePipelines(), 1u);
    ASSERT_FALSE(first.valid());
    ASSERT_TRUE(second.valid());
    ASSERT_EQ(second.handle(), handle);
    ASSERT_EQ(second.stride(), sizeof(Mesh));
}

//------------------------------------------------------------------------------
TEST_F(PipelineTest, RefusesToUseAPipelineThatIsGone)
{
    compages::gpu::Program program = linked(DEPTH_ONLY_VERTEX, FRAGMENT);

    auto pipeline = compages::gpu::Pipeline::create<Mesh>(program, meshLayout()).take();
    pipeline.release();

    ASSERT_FALSE(pipeline.valid());
    ASSERT_EQ(pipeline.stride(), 0u);
    ASSERT_EQ(compages::gpu::vertexReadersHeld(), 0u);

    auto bound = pipeline.bind();
    ASSERT_FALSE(bool(bound));
    ASSERT_THAT(bound.error(), HasSubstr("no longer exists"));
}

//------------------------------------------------------------------------------
TEST_F(PipelineTest, RefusesToCreateWithoutADevice)
{
    compages::gpu::shutdown();

    auto created = compages::gpu::Pipeline::create(compages::gpu::Program{}, compages::gpu::VertexLayout{});
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("compages::gpu::init()"));
}

// ****************************************************************************
//! \brief The render state on its own, which needs no device.
// ****************************************************************************
TEST(RenderStateTest, DefaultsToTheSimplestThingThatDraws)
{
    const compages::gpu::RenderState state;

    ASSERT_EQ(state.primitive, compages::gpu::Primitive::Triangles);
    ASSERT_FALSE(state.depth_test);
    ASSERT_TRUE(state.depth_write);
    ASSERT_FALSE(state.blend.enabled);
    ASSERT_EQ(state.cull, compages::gpu::CullMode::None);
    ASSERT_EQ(state.polygon, compages::gpu::PolygonMode::Fill);
}

//------------------------------------------------------------------------------
// Two blends that are both off behave the same whatever else they say, which is
// what decides whether two pipelines want the same state.
//------------------------------------------------------------------------------
TEST(RenderStateTest, TreatsTwoBlendsThatAreOffAsTheSame)
{
    compages::gpu::Blend one = compages::gpu::Blend::alpha();
    one.enabled = false;

    ASSERT_TRUE(one == compages::gpu::Blend::none());
    ASSERT_FALSE(compages::gpu::Blend::alpha() == compages::gpu::Blend::none());
    ASSERT_FALSE(compages::gpu::Blend::alpha() == compages::gpu::Blend::additive());
}

//------------------------------------------------------------------------------
// The presets read back by name, since a reader who wrote Blend::alpha() should
// not have to decode six factors.
//------------------------------------------------------------------------------
TEST(RenderStateTest, NamesTheBlendItWasGiven)
{
    ASSERT_EQ(compages::gpu::Blend::none().toString(), "off");
    ASSERT_EQ(compages::gpu::Blend::alpha().toString(), "alpha");
    ASSERT_EQ(compages::gpu::Blend::additive().toString(), "additive");
    ASSERT_EQ(compages::gpu::Blend::multiply().toString(), "multiply");
    ASSERT_EQ(compages::gpu::Blend::premultiplied().toString(), "premultiplied alpha");
}

//------------------------------------------------------------------------------
TEST(RenderStateTest, DescribesItselfForReading)
{
    const compages::gpu::RenderState state{ .depth_test = true,
                                  .blend = compages::gpu::Blend::alpha(),
                                  .cull = compages::gpu::CullMode::Back,
                                  .polygon = compages::gpu::PolygonMode::Line };

    const std::string text = state.toString();
    ASSERT_THAT(text, HasSubstr("triangles"));
    ASSERT_THAT(text, HasSubstr("depth test: on, less, writing"));
    ASSERT_THAT(text, HasSubstr("blend: alpha"));
    ASSERT_THAT(text, HasSubstr("cull: back faces"));
    ASSERT_THAT(text, HasSubstr("polygon: lines"));
}

//------------------------------------------------------------------------------
// A pass writing no colour at all is a depth only pass, and saying so is the
// difference between a state dump and one that explains itself.
//------------------------------------------------------------------------------
TEST(RenderStateTest, SaysWhenAPassWritesDepthOnly)
{
    const compages::gpu::RenderState state{ .depth_test = true,
                                  .color_mask = compages::gpu::ColorMask::none() };

    ASSERT_THAT(state.toString(), HasSubstr("writes depth only"));
}

//------------------------------------------------------------------------------
// A matrix field spans one attribute slot per row of the C++ matrix, because the
// hardware reads one slot as one column of the shader's matrix and src/Math holds
// its matrices the other way round. Getting this backwards would send a transform
// nobody asked for.
//------------------------------------------------------------------------------
TEST(RenderStateTest, ReadsAMatrixFieldOneSlotAtATime)
{
    constexpr compages::gpu::AttributeFormat format = compages::gpu::formatOf<Matrix44f>();

    ASSERT_EQ(format.slots, 4u);
    ASSERT_EQ(format.components, 4u);
    ASSERT_EQ(format.size(), sizeof(Matrix44f));
    ASSERT_EQ(format.glslType(), "mat4");
    ASSERT_FALSE(format.as_integer);
}
