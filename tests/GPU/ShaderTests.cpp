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

//! \brief A vertex shader declaring one attribute of each interesting shape.
constexpr const char* VERTEX = R"(#version 450 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in ivec2 aCell;

out vec2 vUV;

uniform mat4 uModel;

void main()
{
    vUV = aUV + vec2(aCell);
    gl_Position = uModel * vec4(aPosition, 1.0);
}
)";

//! \brief A fragment shader with a sampler and a plain uniform.
constexpr const char* FRAGMENT = R"(#version 450 core
in vec2 vUV;
out vec4 oColor;

uniform sampler2D uTexture;
uniform vec4 uTint;
uniform float uGamma;

void main()
{
    oColor = texture(uTexture, vUV) * uTint * uGamma;
}
)";

//! \brief The trivial pair, when the test does not care what is declared.
constexpr const char* PLAIN_VERTEX = R"(#version 450 core
void main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0); }
)";

constexpr const char* PLAIN_FRAGMENT = R"(#version 450 core
out vec4 oColor;
void main() { oColor = vec4(1.0); }
)";

//! \brief A uniform block, whose member offsets are the point of the exercise.
//! Deliberately laid out so that the std140 rules bite: a vec3 followed by a
//! float, and an array of floats whose stride is not four bytes.
constexpr const char* BLOCK_FRAGMENT = R"(#version 450 core
out vec4 oColor;

layout(std140, binding = 2) uniform Frame
{
    mat4 projection;
    vec3 lightDirection;
    float exposure;
    float weights[4];
} frame;

void main()
{
    oColor = frame.projection[0] * frame.exposure * frame.weights[3] +
             vec4(frame.lightDirection, 1.0);
}
)";

//! \brief A compute shader with a storage block, for the compute examples.
constexpr const char* COMPUTE = R"(#version 450 core
layout(local_size_x = 64, local_size_y = 2, local_size_z = 1) in;

struct Star
{
    vec4 position;
    vec4 velocity;
};

layout(std430, binding = 0) buffer Stars
{
    Star stars[];
};

uniform float uDeltaTime;

void main()
{
    uint index = gl_GlobalInvocationID.x;
    stars[index].position += stars[index].velocity * uDeltaTime;
}
)";

} // namespace

// ****************************************************************************
//! \brief A live device, on an invisible context.
// ****************************************************************************
class ShaderTest: public GPUTest
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
TEST_F(ShaderTest, CompilesAStage)
{
    auto vertex = compages::gpu::Shader::fromSource(compages::gpu::ShaderStage::Vertex, VERTEX);
    ASSERT_TRUE(bool(vertex)) << vertex.error();

    auto shader = vertex.take();
    ASSERT_TRUE(shader.valid());
    ASSERT_EQ(shader.stage(), compages::gpu::ShaderStage::Vertex);
    ASSERT_EQ(compages::gpu::liveShaders(), 1u);
}

//------------------------------------------------------------------------------
// The compiler log is the whole value of a failed compilation: its line numbers
// are what the reader goes looking for. It is passed through untouched, with the
// name of the shader added in front since the driver does not know it.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, HandsBackTheCompilerLogOnFailure)
{
    constexpr const char* BROKEN = R"(#version 450 core
void main()
{
    this_function_does_not_exist();
}
)";
    auto vertex = compages::gpu::Shader::fromSource(
        compages::gpu::ShaderStage::Vertex, BROKEN, "broken.vert");

    ASSERT_FALSE(bool(vertex));
    ASSERT_THAT(vertex.error(), HasSubstr("broken.vert"));
    ASSERT_THAT(vertex.error(), HasSubstr("did not compile"));
    ASSERT_THAT(vertex.error(), HasSubstr("this_function_does_not_exist"));
    // Nothing is kept when the compilation fails.
    ASSERT_EQ(compages::gpu::liveShaders(), 0u);
}

//------------------------------------------------------------------------------
TEST_F(ShaderTest, RefusesAnEmptySource)
{
    auto vertex = compages::gpu::Shader::fromSource(compages::gpu::ShaderStage::Vertex, "");
    ASSERT_FALSE(bool(vertex));
    ASSERT_THAT(vertex.error(), HasSubstr("no source"));
}

//------------------------------------------------------------------------------
TEST_F(ShaderTest, LinksAProgram)
{
    auto program = compages::gpu::Program::fromSources(VERTEX, FRAGMENT);
    ASSERT_TRUE(bool(program)) << program.error();

    auto linked = program.take();
    ASSERT_TRUE(linked.valid());
    ASSERT_EQ(compages::gpu::livePrograms(), 1u);
    // The two stages were compiled to link the program and are gone with it.
    ASSERT_EQ(compages::gpu::liveShaders(), 0u);
}

//------------------------------------------------------------------------------
// A vertex shader whose output does not match what the fragment shader reads is
// the classic link failure, and the linker log is what says which name.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, HandsBackTheLinkerLogOnFailure)
{
    constexpr const char* WANTS_SOMETHING_ELSE = R"(#version 450 core
in vec3 vMissing;
out vec4 oColor;
void main() { oColor = vec4(vMissing, 1.0); }
)";

    auto program = compages::gpu::Program::fromSources(VERTEX, WANTS_SOMETHING_ELSE);
    ASSERT_FALSE(bool(program));
    ASSERT_THAT(program.error(), HasSubstr("did not link"));
    ASSERT_EQ(compages::gpu::livePrograms(), 0u);
}

//------------------------------------------------------------------------------
// Two stages of the same kind cannot both be linked. The driver says so in its
// own words, which are rarely the first thing a reader understands, so this is
// caught before it gets there.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, RefusesTwoStagesOfTheSameKind)
{
    auto first = compages::gpu::Shader::fromSource(compages::gpu::ShaderStage::Vertex, PLAIN_VERTEX);
    ASSERT_TRUE(bool(first)) << first.error();
    auto second = compages::gpu::Shader::fromSource(compages::gpu::ShaderStage::Vertex, PLAIN_VERTEX);
    ASSERT_TRUE(bool(second)) << second.error();

    auto shader_one = first.take();
    auto shader_two = second.take();
    auto program = compages::gpu::Program::link({ &shader_one, &shader_two });

    ASSERT_FALSE(bool(program));
    ASSERT_THAT(program.error(), HasSubstr("two vertex stages"));
}

//------------------------------------------------------------------------------
TEST_F(ShaderTest, RefusesAStageThatHoldsNothing)
{
    compages::gpu::Shader empty;
    auto program = compages::gpu::Program::link({ &empty });

    ASSERT_FALSE(bool(program));
    ASSERT_THAT(program.error(), HasSubstr("no compiled shader"));
}

//------------------------------------------------------------------------------
// A stage compiled once and linked into two programs, which is why Shader is a
// type of its own rather than an argument to Program.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, LinksOneStageIntoSeveralPrograms)
{
    auto vertex =
        compages::gpu::Shader::fromSource(compages::gpu::ShaderStage::Vertex, PLAIN_VERTEX).take();
    auto first =
        compages::gpu::Shader::fromSource(compages::gpu::ShaderStage::Fragment, PLAIN_FRAGMENT)
            .take();
    auto second =
        compages::gpu::Shader::fromSource(compages::gpu::ShaderStage::Fragment, BLOCK_FRAGMENT)
            .take();

    auto one = compages::gpu::Program::link({ &vertex, &first });
    ASSERT_TRUE(bool(one)) << one.error();
    auto two = compages::gpu::Program::link({ &vertex, &second });
    ASSERT_TRUE(bool(two)) << two.error();

    ASSERT_EQ(compages::gpu::livePrograms(), 2u);
    // The shared stage is still there, having been linked twice.
    ASSERT_TRUE(vertex.valid());
}

//------------------------------------------------------------------------------
// What the vertex layout has to be checked against. Names, locations and shapes,
// all of them the driver's answer rather than a guess.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, ReportsTheAttributesTheShaderDeclares)
{
    auto program = compages::gpu::Program::fromSources(VERTEX, FRAGMENT).take();
    compages::gpu::ProgramReflection const& what = program.reflection();

    ASSERT_EQ(what.attributes.size(), 3u);

    compages::gpu::AttributeInfo const* position = what.attribute("aPosition");
    ASSERT_NE(position, nullptr);
    ASSERT_EQ(position->location, 0);
    ASSERT_EQ(position->type, compages::gpu::DataType::Vec3);
    ASSERT_EQ(position->format.components, 3u);
    ASSERT_EQ(position->format.slots, 1u);
    ASSERT_EQ(position->format.scalar, compages::gpu::ScalarType::Float);
    ASSERT_FALSE(position->format.as_integer);

    compages::gpu::AttributeInfo const* uv = what.attribute("aUV");
    ASSERT_NE(uv, nullptr);
    ASSERT_EQ(uv->location, 1);
    ASSERT_EQ(uv->type, compages::gpu::DataType::Vec2);

    // An integer attribute is read as whole numbers, which is what tells the
    // hardware not to convert it.
    compages::gpu::AttributeInfo const* cell = what.attribute("aCell");
    ASSERT_NE(cell, nullptr);
    ASSERT_EQ(cell->type, compages::gpu::DataType::IVec2);
    ASSERT_TRUE(cell->format.as_integer);
    ASSERT_EQ(cell->format.scalar, compages::gpu::ScalarType::Int32);

    ASSERT_EQ(what.attribute("thereIsNoSuchThing"), nullptr);
}

//------------------------------------------------------------------------------
TEST_F(ShaderTest, ReportsTheUniformsAndTheirTypes)
{
    auto program = compages::gpu::Program::fromSources(VERTEX, FRAGMENT).take();
    compages::gpu::ProgramReflection const& what = program.reflection();

    compages::gpu::UniformInfo const* model = what.uniform("uModel");
    ASSERT_NE(model, nullptr);
    ASSERT_EQ(model->type, compages::gpu::DataType::Mat4);
    ASSERT_GE(model->location, 0);

    ASSERT_NE(what.uniform("uTint"), nullptr);
    ASSERT_EQ(what.uniform("uTint")->type, compages::gpu::DataType::Vec4);
    ASSERT_EQ(what.uniform("uGamma")->type, compages::gpu::DataType::Float);
}

//------------------------------------------------------------------------------
// Which textures a pass has to be given, asked of the program rather than
// remembered by hand.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, SinglesOutTheSamplers)
{
    auto program = compages::gpu::Program::fromSources(VERTEX, FRAGMENT).take();

    auto samplers = program.reflection().samplers();
    ASSERT_EQ(samplers.size(), 1u);
    ASSERT_EQ(samplers[0]->name, "uTexture");
    ASSERT_EQ(samplers[0]->type, compages::gpu::DataType::Sampler2D);
    ASSERT_TRUE(compages::gpu::isSampler(samplers[0]->type));
}

//------------------------------------------------------------------------------
// The reason introspection exists at all. The std140 rules say where a member
// should be; only the driver knows where it is. Here both agree, and the value of
// asking is that we would find out if they ever did not.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, ReportsWhereTheDriverPutEachBlockMember)
{
    auto program = compages::gpu::Program::fromSources(PLAIN_VERTEX, BLOCK_FRAGMENT).take();

    compages::gpu::BlockInfo const* frame = program.reflection().uniformBlock("Frame");
    ASSERT_NE(frame, nullptr) << program.reflection().toString();
    ASSERT_EQ(frame->binding, 2);

    compages::gpu::BlockMember const* projection = frame->find("projection");
    ASSERT_NE(projection, nullptr);
    ASSERT_EQ(projection->type, compages::gpu::DataType::Mat4);
    ASSERT_EQ(projection->offset, 0u);
    // A mat4 in std140 is four columns of sixteen bytes.
    ASSERT_EQ(projection->matrix_stride, 16u);

    // A vec3 takes the space of a vec4, so what follows starts at 64 + 16.
    compages::gpu::BlockMember const* light = frame->find("lightDirection");
    ASSERT_NE(light, nullptr);
    ASSERT_EQ(light->type, compages::gpu::DataType::Vec3);
    ASSERT_EQ(light->offset, 64u);

    // The float that follows a vec3 fits in the gap the vec3 left behind. This is
    // the single most surprising rule of std140, and getting it wrong silently
    // shifts everything after it.
    compages::gpu::BlockMember const* exposure = frame->find("exposure");
    ASSERT_NE(exposure, nullptr);
    ASSERT_EQ(exposure->offset, 76u);

    // And an array of floats has a stride of sixteen bytes, not four.
    compages::gpu::BlockMember const* weights = frame->find("weights");
    ASSERT_NE(weights, nullptr);
    ASSERT_EQ(weights->elements, 4);
    ASSERT_EQ(weights->array_stride, 16u);
    ASSERT_EQ(weights->offset, 80u);

    // Which makes the whole block 80 + 4 * 16 bytes.
    ASSERT_EQ(frame->bytes, 144u);
}

//------------------------------------------------------------------------------
// A block member is asked for by its own name, not by the name it has inside the
// block instance, which is how the driver reports it.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, NamesBlockMembersWithoutTheirBlock)
{
    auto program = compages::gpu::Program::fromSources(PLAIN_VERTEX, BLOCK_FRAGMENT).take();
    compages::gpu::BlockInfo const* frame = program.reflection().uniformBlock("Frame");
    ASSERT_NE(frame, nullptr);

    for (compages::gpu::BlockMember const& member : frame->members)
    {
        ASSERT_THAT(member.name, Not(HasSubstr("Frame.")));
        ASSERT_THAT(member.name, Not(HasSubstr("[0]")));
    }
}

//------------------------------------------------------------------------------
// A uniform inside a block is not settable one value at a time, and saying which
// block it lives in is more useful than saying it does not exist.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, SaysWhenAUniformLivesInABlock)
{
    auto program = compages::gpu::Program::fromSources(PLAIN_VERTEX, BLOCK_FRAGMENT).take();

    auto written = compages::gpu::attempt([&] { program.set("exposure", 1.0f); });
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("uniform block 'Frame'"));
}

//------------------------------------------------------------------------------
// Compute, and its storage block, which the older introspection calls could not
// describe at all.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, ReportsAComputeProgramAndItsStorageBlock)
{
    auto program = compages::gpu::Program::fromComputeSource(COMPUTE);
    ASSERT_TRUE(bool(program)) << program.error();
    compages::gpu::ProgramReflection const& what = program.value().reflection();

    ASSERT_EQ(what.work_group_size[0], 64);
    ASSERT_EQ(what.work_group_size[1], 2);
    ASSERT_EQ(what.work_group_size[2], 1);

    compages::gpu::BlockInfo const* stars = what.storageBlock("Stars");
    ASSERT_NE(stars, nullptr) << what.toString();
    ASSERT_EQ(stars->binding, 0);

    // A member of an array of structs is reported by the driver under the name of
    // the first element, "stars[0].position". The subscript is dropped so that the
    // name is the one somebody would go looking for.
    ASSERT_NE(stars->find("stars.position"), nullptr) << what.toString();
    ASSERT_NE(stars->find("stars.velocity"), nullptr);
    ASSERT_EQ(stars->find("stars.position")->type, compages::gpu::DataType::Vec4);
    // std430 packs a vec4 at sixteen bytes, so the velocity follows the position.
    ASSERT_EQ(stars->find("stars.velocity")->offset, 16u);

    ASSERT_NE(what.uniform("uDeltaTime"), nullptr);
}

//------------------------------------------------------------------------------
// A compute shader reading gl_GlobalInvocationID has it reported among its
// inputs, at no location at all. Leaving it in would mean a vertex layout is
// expected to supply it, so what is reported is only what the caller owes.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, LeavesOutTheVariablesTheDriverProvidesItself)
{
    auto program = compages::gpu::Program::fromComputeSource(COMPUTE).take();

    for (compages::gpu::AttributeInfo const& one : program.reflection().attributes)
    {
        ASSERT_THAT(one.name, Not(StartsWith("gl_")));
        ASSERT_GE(one.location, 0);
    }
    ASSERT_EQ(program.reflection().attribute("gl_GlobalInvocationID"), nullptr);
}

//------------------------------------------------------------------------------
// A graphics program has no work group size, and asking the driver for one is an
// error, so the answer is zero rather than a message from the driver.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, ReportsNoWorkGroupSizeForAGraphicsProgram)
{
    auto program = compages::gpu::Program::fromSources(PLAIN_VERTEX, PLAIN_FRAGMENT).take();
    ASSERT_EQ(program.reflection().work_group_size[0], 0);
}

//------------------------------------------------------------------------------
TEST_F(ShaderTest, SetsAUniform)
{
    auto program = compages::gpu::Program::fromSources(VERTEX, FRAGMENT).take();

    const compages::core::Matrix44f model(compages::core::matrix::Identity);
    auto written = compages::gpu::attempt([&] { program.set("uModel", model); });
    ASSERT_TRUE(bool(written)) << written.error();

    ASSERT_TRUE(bool(compages::gpu::attempt([&] { program.set("uTint", compages::core::Vector4f(1.0f, 0.5f, 0.0f, 1.0f)); })));
    ASSERT_TRUE(bool(compages::gpu::attempt([&] { program.set("uGamma", 2.2f); })));
    // A sampler is set by giving it the number of a texture unit.
    ASSERT_TRUE(bool(compages::gpu::attempt([&] { program.set("uTexture", 0); })));
}

//------------------------------------------------------------------------------
// A name that does not match the shader used to do nothing at all, and the
// symptom was a black screen. Now it is a sentence, and it lists what the program
// does declare, since the answer is usually right there.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, RefusesAUniformTheShaderDoesNotDeclare)
{
    auto program = compages::gpu::Program::fromSources(VERTEX, FRAGMENT).take();

    auto written = compages::gpu::attempt([&] { program.set("uModelMatrix", 1.0f); });
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("uModelMatrix"));
    // The list of what does exist, so the real name can be spotted.
    ASSERT_THAT(written.error(), HasSubstr("uModel"));
}

//------------------------------------------------------------------------------
// Writing four floats into a uniform the shader declares as a matrix used to be
// undefined behaviour with no warning. The declared type is known, so the two can
// be named against each other.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, RefusesAUniformSetWithTheWrongType)
{
    auto program = compages::gpu::Program::fromSources(VERTEX, FRAGMENT).take();

    auto written = compages::gpu::attempt([&] { program.set("uModel", compages::core::Vector4f(0.0f, 0.0f, 0.0f, 1.0f)); });
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("declares 'uModel' as mat4"));
    ASSERT_THAT(written.error(), HasSubstr("set as vec4"));
}

//------------------------------------------------------------------------------
// A uniform the shader declares but never reads is removed by the compiler, and
// that is by far the most common reason a name cannot be found.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, ReportsOnlyTheUniformsTheShaderActuallyUses)
{
    constexpr const char* UNUSED = R"(#version 450 core
out vec4 oColor;
uniform vec4 uNeverRead;
void main() { oColor = vec4(1.0); }
)";
    auto program = compages::gpu::Program::fromSources(PLAIN_VERTEX, UNUSED).take();

    ASSERT_EQ(program.reflection().uniform("uNeverRead"), nullptr);

    auto written = compages::gpu::attempt([&] { program.set("uNeverRead", compages::core::Vector4f(1.0f, 1.0f, 1.0f, 1.0f)); });
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("never reads it"));
}

//------------------------------------------------------------------------------
TEST_F(ShaderTest, MovesAProgramWithoutCopyingIt)
{
    auto first = compages::gpu::Program::fromSources(VERTEX, FRAGMENT).take();
    const compages::gpu::ProgramHandle handle = first.handle();

    compages::gpu::Program second = std::move(first);
    ASSERT_EQ(compages::gpu::livePrograms(), 1u);
    ASSERT_FALSE(first.valid());
    ASSERT_TRUE(second.valid());
    ASSERT_EQ(second.handle(), handle);
    // The reflection travelled with it.
    ASSERT_NE(second.reflection().uniform("uModel"), nullptr);
}

//------------------------------------------------------------------------------
TEST_F(ShaderTest, ReleasesEverythingWhenItGoesOutOfScope)
{
    {
        auto vertex =
            compages::gpu::Shader::fromSource(compages::gpu::ShaderStage::Vertex, PLAIN_VERTEX)
                .take();
        auto program = compages::gpu::Program::fromSources(VERTEX, FRAGMENT).take();
        ASSERT_EQ(compages::gpu::liveShaders(), 1u);
        ASSERT_EQ(compages::gpu::livePrograms(), 1u);
    }
    ASSERT_EQ(compages::gpu::liveShaders(), 0u);
    ASSERT_EQ(compages::gpu::livePrograms(), 0u);
}

//------------------------------------------------------------------------------
// Asking a program that no longer exists must be a message, not a crash.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, RefusesToUseAProgramThatIsGone)
{
    auto program = compages::gpu::Program::fromSources(VERTEX, FRAGMENT).take();
    program.release();

    ASSERT_FALSE(program.valid());
    ASSERT_TRUE(program.reflection().attributes.empty());

    auto written = compages::gpu::attempt([&] { program.set("uGamma", 1.0f); });
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("no longer exists"));
}

//------------------------------------------------------------------------------
TEST_F(ShaderTest, RefusesToCompileWithoutADevice)
{
    compages::gpu::shutdown();

    auto vertex = compages::gpu::Shader::fromSource(compages::gpu::ShaderStage::Vertex, VERTEX);
    ASSERT_FALSE(bool(vertex));
    ASSERT_THAT(vertex.error(), HasSubstr("compages::gpu::init()"));
}

//------------------------------------------------------------------------------
// Which stage a file holds is taken from its extension, so that the common case
// needs no argument at all.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, SaysSoWhenItCannotTellTheStageOfAFile)
{
    auto vertex = compages::gpu::Shader::fromFile("shaders/mesh.txt");
    ASSERT_FALSE(bool(vertex));
    ASSERT_THAT(vertex.error(), HasSubstr("cannot tell which pipeline stage"));
    ASSERT_THAT(vertex.error(), HasSubstr(".vert"));
}

//------------------------------------------------------------------------------
TEST_F(ShaderTest, SaysSoWhenAShaderFileIsMissing)
{
    auto vertex = compages::gpu::Shader::fromFile("there/is/no/such.vert");
    ASSERT_FALSE(bool(vertex));
    ASSERT_THAT(vertex.error(), HasSubstr("cannot read"));
}

//------------------------------------------------------------------------------
// The binding of a uniform block belongs in the shader, but it can also be said
// from here, and what is reported must then agree with what the driver was told.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, MovesAUniformBlockToAnotherBinding)
{
    auto program = compages::gpu::Program::fromSources(PLAIN_VERTEX, BLOCK_FRAGMENT).take();
    ASSERT_EQ(program.reflection().uniformBlock("Frame")->binding, 2);

    auto bound = program.bindUniformBlock("Frame", 5);
    ASSERT_TRUE(bool(bound)) << bound.error();
    ASSERT_EQ(program.reflection().uniformBlock("Frame")->binding, 5);
}

//------------------------------------------------------------------------------
TEST_F(ShaderTest, RefusesToBindABlockTheShaderDoesNotDeclare)
{
    auto program = compages::gpu::Program::fromSources(PLAIN_VERTEX, BLOCK_FRAGMENT).take();

    auto bound = program.bindUniformBlock("Camera", 1);
    ASSERT_FALSE(bool(bound));
    ASSERT_THAT(bound.error(), HasSubstr("no uniform block called 'Camera'"));
}

//------------------------------------------------------------------------------
// The readable dump is what somebody prints when a shader is not doing what they
// expected, so it has to mention everything.
//------------------------------------------------------------------------------
TEST_F(ShaderTest, DescribesItselfInWords)
{
    auto program = compages::gpu::Program::fromSources(VERTEX, BLOCK_FRAGMENT).take();
    const std::string text = program.reflection().toString();

    ASSERT_THAT(text, HasSubstr("aPosition"));
    ASSERT_THAT(text, HasSubstr("vec3"));
    ASSERT_THAT(text, HasSubstr("uModel"));
    ASSERT_THAT(text, HasSubstr("mat4"));
    ASSERT_THAT(text, HasSubstr("Frame"));
    ASSERT_THAT(text, HasSubstr("lightDirection"));
    ASSERT_THAT(text, HasSubstr("offset 76"));
}
