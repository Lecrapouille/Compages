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

#include <vector>

using namespace tests;

namespace
{

constexpr int WIDTH = 32;
constexpr int HEIGHT = 32;

constexpr const char* DOUBLE_SOURCE = R"(#version 450 core
layout(local_size_x = 64) in;

layout(std430, binding = 0) buffer Values
{
    float data[];
};

uniform uint count;

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= count)
    {
        return;
    }
    data[i] = data[i] * 2.0;
}
)";

constexpr const char* PING_SOURCE = R"(#version 450 core
layout(local_size_x = 64) in;

layout(std430, binding = 0) readonly buffer Input
{
    float incoming[];
};

layout(std430, binding = 1) writeonly buffer Output
{
    float outgoing[];
};

uniform uint count;

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= count)
    {
        return;
    }
    outgoing[i] = incoming[i] + 1.0;
}
)";

constexpr const char* POINT_VERTEX = R"(#version 450 core
in vec2 position;
void main()
{
    gl_PointSize = 8.0;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr const char* RED_FRAGMENT = R"(#version 450 core
out vec4 oColor;
void main() { oColor = vec4(1.0, 0.0, 0.0, 1.0); }
)";

constexpr const char* WRITE_POINTS = R"(#version 450 core
layout(local_size_x = 1) in;

struct Particle
{
    vec2 position;
    vec2 velocity;
};

layout(std430, binding = 0) buffer Particles
{
    Particle particles[];
};

void main()
{
    particles[0].position = vec2(0.0, 0.0);
    particles[0].velocity = vec2(0.0, 0.0);
}
)";

struct Particle
{
    Vector2f position;
    Vector2f velocity;
};

} // namespace

class ComputeTest: public GPUTest
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

    [[nodiscard]] int targetWidth() const override
    {
        return WIDTH;
    }

    [[nodiscard]] int targetHeight() const override
    {
        return HEIGHT;
    }
};

//------------------------------------------------------------------------------
TEST_F(ComputeTest, DoublesWhatIsInTheBuffer)
{
    const std::vector<float> start(64u, 3.0f);
    auto buffer = compages::gpu::Buffer<float>::from(
                      std::span<const float>(start),
                      compages::gpu::BufferKind::Storage,
                      compages::gpu::BufferUsage::Storage)
                      .take();

    auto compute = compages::gpu::ComputeProgram::fromSource(DOUBLE_SOURCE);
    ASSERT_TRUE(bool(compute)) << compute.error();
    ASSERT_EQ(compute.value().workGroupSize()[0], 64);

    ASSERT_TRUE(bool(compute.value().bind("Values", buffer)));
    ASSERT_TRUE(bool(compages::gpu::attempt([&] { compute.value().set("count", 64u); })));
    auto ran = compute.value().dispatchItems(64u);
    ASSERT_TRUE(bool(ran)) << ran.error();

    compages::gpu::barrier(compages::gpu::Barrier::Storage);
    auto back = buffer.read();
    ASSERT_TRUE(bool(back)) << back.error();
    ASSERT_EQ(back.value().size(), 64u);
    ASSERT_FLOAT_EQ(back.value()[0], 6.0f);
    ASSERT_FLOAT_EQ(back.value()[63], 6.0f);
}

//------------------------------------------------------------------------------
TEST_F(ComputeTest, ErgonomicDispatchSynchronizesOneMirroredBuffer)
{
    const std::vector<float> start(64u, 3.0f);
    compages::gpu::BufferOptions options;
    options.kind = compages::gpu::BufferKind::Storage;
    options.usage = compages::gpu::BufferUsage::Storage;
    auto buffer = compages::gpu::Buffer<float>::from(start, options).take();
    buffer[0u] = 4.0f;

    auto compute = compages::gpu::ComputeProgram::fromSource(DOUBLE_SOURCE).take();
    ASSERT_TRUE(bool(compages::gpu::attempt([&] { compute.set("count", 64u); })));
    auto ran = compages::gpu::dispatch(compute, buffer);
    ASSERT_TRUE(bool(ran)) << ran.error();
    auto downloaded = buffer.download();
    ASSERT_TRUE(bool(downloaded)) << downloaded.error();

    auto const& values = static_cast<compages::gpu::Buffer<float> const&>(buffer);
    ASSERT_FLOAT_EQ(values[0u], 8.0f);
    ASSERT_FLOAT_EQ(values[63u], 6.0f);
}

//------------------------------------------------------------------------------
TEST_F(ComputeTest, ErgonomicDispatchRequiresExactlyOneStorageBlock)
{
    const std::vector<float> start(64u, 1.0f);
    compages::gpu::BufferOptions options;
    options.kind = compages::gpu::BufferKind::Storage;
    options.usage = compages::gpu::BufferUsage::Storage;
    auto buffer = compages::gpu::Buffer<float>::from(start, options).take();
    auto compute = compages::gpu::ComputeProgram::fromSource(PING_SOURCE).take();

    auto ran = compages::gpu::dispatch(compute, buffer);
    ASSERT_FALSE(bool(ran));
    ASSERT_THAT(ran.error(), HasSubstr("exactly one"));
    ASSERT_THAT(ran.error(), HasSubstr("2"));
}

//------------------------------------------------------------------------------
TEST_F(ComputeTest, DownloadRefusesABufferWithoutCpuMirror)
{
    const std::vector<float> start(4u, 1.0f);
    auto buffer = compages::gpu::Buffer<float>::from(
                      std::span<const float>(start),
                      compages::gpu::BufferKind::Storage,
                      compages::gpu::BufferUsage::Storage)
                      .take();

    auto downloaded = buffer.download();
    ASSERT_FALSE(bool(downloaded));
    ASSERT_THAT(downloaded.error(), HasSubstr("CPU mirror"));
}

//------------------------------------------------------------------------------
TEST_F(ComputeTest, PingPongWritesTheOtherBuffer)
{
    const std::vector<float> start(64u, 1.0f);
    auto pair = compages::gpu::PingPong<float>::from(std::span<const float>(start));
    ASSERT_TRUE(bool(pair)) << pair.error();
    auto& stars = pair.value();

    auto compute = compages::gpu::ComputeProgram::fromSource(PING_SOURCE).take();
    ASSERT_TRUE(bool(compute.bind("Input", stars.input())));
    ASSERT_TRUE(bool(compute.bind("Output", stars.output())));
    ASSERT_TRUE(bool(compages::gpu::attempt([&] { compute.set("count", 64u); })));
    ASSERT_TRUE(bool(compute.dispatchItems(64u)));

    compages::gpu::barrier(compages::gpu::Barrier::Storage);
    stars.swap();

    auto out = stars.input().read();
    ASSERT_TRUE(bool(out)) << out.error();
    ASSERT_FLOAT_EQ(out.value()[0], 2.0f);
}

//------------------------------------------------------------------------------
TEST_F(ComputeTest, AStorageBufferCanThenBeDrawnAsVertices)
{
    auto particles = compages::gpu::Buffer<Particle>::create(
                         1u, compages::gpu::BufferKind::Storage, compages::gpu::BufferUsage::Storage)
                         .take();

    auto compute = compages::gpu::ComputeProgram::fromSource(WRITE_POINTS).take();
    ASSERT_TRUE(bool(compute.bind("Particles", particles)));
    ASSERT_TRUE(bool(compute.dispatch(1u)));
    compages::gpu::barrier(compages::gpu::Barrier::VertexAttrib);

    auto program = compages::gpu::Program::fromSources(POINT_VERTEX, RED_FRAGMENT).take();
    const compages::gpu::VertexLayout layout = compages::gpu::VertexLayout::of<Particle>();
    compages::gpu::RenderState state;
    state.primitive = compages::gpu::Primitive::Points;
    auto pipeline = compages::gpu::Pipeline::create<Particle>(program, layout, state).take();

    compages::gpu::PassDesc desc;
    desc.width = WIDTH;
    desc.height = HEIGHT;
    desc.color = Vector4f(0.0f, 0.0f, 1.0f, 1.0f);
    desc.target = {};
    auto pass = compages::gpu::RenderPass::begin(desc);
    ASSERT_TRUE(bool(pass)) << pass.error();
    ASSERT_TRUE(bool(compages::gpu::attempt([&] { compages::gpu::draw(pipeline, particles); })));

    auto picture = compages::gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    const std::size_t at =
        ((static_cast<std::size_t>(HEIGHT / 2) * WIDTH) + (WIDTH / 2)) * 4u;
    ASSERT_EQ(static_cast<int>(picture.value()[at]), 255);
    ASSERT_EQ(static_cast<int>(picture.value()[at + 1u]), 0);
    ASSERT_EQ(static_cast<int>(picture.value()[at + 2u]), 0);
}

//------------------------------------------------------------------------------
TEST_F(ComputeTest, RefusesADispatchOfNothing)
{
    auto compute = compages::gpu::ComputeProgram::fromSource(DOUBLE_SOURCE).take();
    auto ran = compute.dispatch(0u);
    ASSERT_FALSE(bool(ran));
    ASSERT_THAT(ran.error(), HasSubstr("zero"));
}

//------------------------------------------------------------------------------
TEST_F(ComputeTest, RefusesABufferThatIsNotStorage)
{
    auto compute = compages::gpu::ComputeProgram::fromSource(DOUBLE_SOURCE).take();
    const std::vector<float> start(4u, 1.0f);
    auto vertices = compages::gpu::Buffer<float>::from(
                        std::span<const float>(start),
                        compages::gpu::BufferKind::Vertex,
                        compages::gpu::BufferUsage::Dynamic)
                        .take();

    auto bound = compute.bind("Values", vertices);
    ASSERT_FALSE(bool(bound));
    ASSERT_THAT(bound.error(), HasSubstr("vertex"));
}

//------------------------------------------------------------------------------
TEST_F(ComputeTest, RefusesABlockTheShaderDoesNotDeclare)
{
    auto compute = compages::gpu::ComputeProgram::fromSource(DOUBLE_SOURCE).take();
    auto buffer = compages::gpu::Buffer<float>::create(
                      4u, compages::gpu::BufferKind::Storage, compages::gpu::BufferUsage::Storage)
                      .take();
    auto bound = compute.bind("Stars", buffer);
    ASSERT_FALSE(bool(bound));
    ASSERT_THAT(bound.error(), HasSubstr("Stars"));
}

//------------------------------------------------------------------------------
TEST_F(ComputeTest, CountsADispatch)
{
    compages::gpu::resetFrameStatistics();
    auto compute = compages::gpu::ComputeProgram::fromSource(DOUBLE_SOURCE).take();
    auto buffer = compages::gpu::Buffer<float>::create(
                      64u, compages::gpu::BufferKind::Storage, compages::gpu::BufferUsage::Storage)
                      .take();
    ASSERT_TRUE(bool(compute.bind("Values", buffer)));
    ASSERT_TRUE(bool(compages::gpu::attempt([&] { compute.set("count", 64u); })));
    ASSERT_TRUE(bool(compute.dispatchItems(64u)));
    ASSERT_EQ(compages::gpu::frameStatistics().dispatches, 1u);
}
