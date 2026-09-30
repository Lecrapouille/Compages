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

#include <cstring>
#include <vector>




using namespace tests;

// GPU_STD140 specialises a template in compages::gpu::std140, which a type hidden in an
// anonymous namespace cannot do: the specialisation would be declared in the
// unnamed namespace rather than in compages::gpu::std140, and the compiler would refuse it
// and then treat the struct as never described.

// ****************************************************************************
//! \brief The two matrices a scene always has, and the easiest typed block.
// ****************************************************************************
struct Matrices
{
    compages::core::Matrix44f projection;
    compages::core::Matrix44f view;
};
GPU_STD140(Matrices, projection, view);

// ****************************************************************************
//! \brief Same size, same offsets, names the shader does not use.
//!
//! GPU_STD140 is happy because C++ and the rules agree. create() is not, because
//! the driver is asked about a block whose members are called something else.
// ****************************************************************************
struct Renamed
{
    compages::core::Matrix44f a;
    compages::core::Matrix44f b;
};
GPU_STD140(Renamed, a, b);

// ****************************************************************************
//! \brief Half of what the shader wants. Too small, and missing a member.
// ****************************************************************************
struct Half
{
    compages::core::Matrix44f projection;
};
GPU_STD140(Half, projection);

namespace
{

constexpr int WIDTH = 32;
constexpr int HEIGHT = 32;

constexpr const char* VERTEX = R"(#version 450 core
in vec2 position;
void main() { gl_Position = vec4(position, 0.0, 1.0); }
)";

//! \brief Laid out so that the two surprises of std140 are both in it: a vec3
//! followed by a float, which C++ packs and the rules leave a gap for unless the
//! float sits in the gap, and an array of floats whose elements are 16 bytes
//! apart rather than 4.
constexpr const char* FRAME_FRAGMENT = R"(#version 450 core
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

constexpr const char* MATRICES_FRAGMENT = R"(#version 450 core
out vec4 oColor;

layout(std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
} matrices;

void main()
{
    oColor = matrices.projection[0] + matrices.view[0];
}
)";

//! \brief A mat3, which occupies 48 bytes in a block and 36 in C++. The columns
//! have to travel one at a time or the shader reads the padding of one as the
//! start of the next.
constexpr const char* MAT3_FRAGMENT = R"(#version 450 core
out vec4 oColor;

layout(std140, binding = 1) uniform Basis
{
    mat3 axes;
} basis;

void main()
{
    oColor = vec4(basis.axes[0], 1.0);
}
)";

//! \brief The colour of the picture comes from the block, and nothing else. Two
//! programs declare the same block on the same point, which is the whole of what
//! a multi-pass frame is: one write, two reads.
constexpr const char* COLOUR_FRAGMENT = R"(#version 450 core
out vec4 oColor;

layout(std140, binding = 3) uniform Tint
{
    vec4 colour;
} tint;

void main()
{
    oColor = tint.colour;
}
)";

constexpr const char* COLOUR_SQUARED_FRAGMENT = R"(#version 450 core
out vec4 oColor;

layout(std140, binding = 3) uniform Tint
{
    vec4 colour;
} tint;

void main()
{
    oColor = tint.colour * tint.colour;
}
)";

struct Corner
{
    compages::core::Vector2f position;
};

const std::vector<Corner> BIG_TRIANGLE{ { { -1.0f, -1.0f } },
                                        { { 3.0f, -1.0f } },
                                        { { -1.0f, 3.0f } } };

struct Pixel
{
    std::uint8_t red = 0u;
    std::uint8_t green = 0u;
    std::uint8_t blue = 0u;
    std::uint8_t alpha = 0u;

    [[nodiscard]] friend bool operator==(Pixel const& p_left,
                                         Pixel const& p_right)
    {
        return (p_left.red == p_right.red) && (p_left.green == p_right.green) &&
               (p_left.blue == p_right.blue) && (p_left.alpha == p_right.alpha);
    }
};

std::ostream& operator<<(std::ostream& p_stream, Pixel const& p_pixel)
{
    return p_stream << "rgba(" << int(p_pixel.red) << ", " << int(p_pixel.green)
                    << ", " << int(p_pixel.blue) << ", " << int(p_pixel.alpha)
                    << ")";
}

Pixel pixelAt(std::vector<std::byte> const& p_picture,
              std::uint32_t p_x,
              std::uint32_t p_y)
{
    const std::size_t at =
        ((static_cast<std::size_t>(p_y) * WIDTH) + p_x) * 4u;
    return Pixel{ static_cast<std::uint8_t>(p_picture[at]),
                  static_cast<std::uint8_t>(p_picture[at + 1u]),
                  static_cast<std::uint8_t>(p_picture[at + 2u]),
                  static_cast<std::uint8_t>(p_picture[at + 3u]) };
}

float floatAt(std::vector<std::byte> const& p_bytes, std::size_t p_offset)
{
    float value = 0.0f;
    std::memcpy(&value, p_bytes.data() + p_offset, sizeof(float));
    return value;
}

compages::gpu::Program linked(char const* p_fragment)
{
    auto program = compages::gpu::Program::fromSources(VERTEX, p_fragment);
    EXPECT_TRUE(bool(program)) << program.error();
    return program ? program.take() : compages::gpu::Program{};
}

//! \brief What the device holds for this block, after an update.
std::vector<std::byte> deviceBytes(compages::gpu::UniformBlock const& p_block)
{
    std::vector<std::byte> bytes(p_block.bytes());
    auto read = compages::gpu::detail::readBuffer(
        p_block.handle(), 0u, bytes.size(), bytes.data());
    EXPECT_TRUE(bool(read)) << read.error();
    return bytes;
}

compages::gpu::PassDesc wholeTarget()
{
    return { .width = static_cast<std::uint32_t>(WIDTH),
             .height = static_cast<std::uint32_t>(HEIGHT),
             .color = { 0.0f, 0.0f, 0.0f, 1.0f },
             .target = {} };
}

} // namespace

// ****************************************************************************
//! \brief A live device, large enough that a drawn colour can be read back.
// ****************************************************************************
class UniformBlockTest: public GPUTest
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
TEST_F(UniformBlockTest, MakesABlockTheSizeTheDriverAskedFor)
{
    auto program = linked(FRAME_FRAGMENT);
    auto made = compages::gpu::UniformBlock::create(program, "Frame");
    ASSERT_TRUE(bool(made)) << made.error();
    auto block = made.take();

    ASSERT_TRUE(block.valid());
    ASSERT_EQ(block.name(), "Frame");
    ASSERT_EQ(block.binding(), 2);
    ASSERT_EQ(block.bytes(), program.reflection().uniformBlock("Frame")->bytes);
    ASSERT_GE(block.bytes(), 144u);

    ASSERT_THAT(block.describe(), HasSubstr("projection"));
    ASSERT_THAT(block.describe(), HasSubstr("weights"));
    ASSERT_THAT(block.describe(), HasSubstr("binding point 2"));
}

//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, RefusesANameTheProgramDoesNotDeclare)
{
    auto program = linked(FRAME_FRAGMENT);
    auto block = compages::gpu::UniformBlock::create(program, "Camera");

    ASSERT_FALSE(bool(block));
    ASSERT_THAT(block.error(), HasSubstr("Camera"));
    ASSERT_THAT(block.error(), HasSubstr("Frame"));
}

//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, RefusesAProgramThatNeverLinked)
{
    compages::gpu::Program empty;
    auto block = compages::gpu::UniformBlock::create(empty, "Frame");

    ASSERT_FALSE(bool(block));
    ASSERT_THAT(block.error(), HasSubstr("did not link"));
}

//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, CanBeMovedToAnotherBinding)
{
    auto program = linked(FRAME_FRAGMENT);
    auto made = compages::gpu::UniformBlock::create(program, "Frame", 7);
    ASSERT_TRUE(bool(made)) << made.error();
    auto block = made.take();

    ASSERT_EQ(block.binding(), 7);
    ASSERT_EQ(program.reflection().uniformBlock("Frame")->binding, 7);
}

//------------------------------------------------------------------------------
// The offsets are the driver's. The test does not compute them: it writes, reads
// the buffer back, and checks the values sit where the reflection said they
// would. That is the whole of what makes the untyped block safe.
//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, WritesWhereTheDriverSaidTheMembersLive)
{
    auto program = linked(FRAME_FRAGMENT);
    auto made = compages::gpu::UniformBlock::create(program, "Frame");
    ASSERT_TRUE(bool(made)) << made.error();
    auto block = made.take();

    const compages::core::Vector3f light(0.25f, 0.5f, 0.75f);
    ASSERT_TRUE(bool(block.set("lightDirection", light))) << block.describe();
    ASSERT_TRUE(bool(block.set("exposure", 2.5f)));
    ASSERT_TRUE(bool(block.update()));

    compages::gpu::BlockMember const* direction = block.info().find("lightDirection");
    compages::gpu::BlockMember const* exposure = block.info().find("exposure");
    ASSERT_NE(direction, nullptr);
    ASSERT_NE(exposure, nullptr);

    const auto bytes = deviceBytes(block);
    ASSERT_FLOAT_EQ(floatAt(bytes, direction->offset), 0.25f);
    ASSERT_FLOAT_EQ(floatAt(bytes, direction->offset + 4u), 0.5f);
    ASSERT_FLOAT_EQ(floatAt(bytes, direction->offset + 8u), 0.75f);
    ASSERT_FLOAT_EQ(floatAt(bytes, exposure->offset), 2.5f);
}

//------------------------------------------------------------------------------
// An array of floats is the one people copy from a packed C++ array and then
// spend an afternoon on. Each element sits at the stride the driver reported,
// sixteen bytes under std140, and writing through setElement() is how not to
// invent that number.
//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, PutsArrayElementsAtTheDriversStride)
{
    auto program = linked(FRAME_FRAGMENT);
    auto made = compages::gpu::UniformBlock::create(program, "Frame");
    ASSERT_TRUE(bool(made)) << made.error();
    auto block = made.take();

    ASSERT_TRUE(bool(block.setElement("weights", 0u, 1.0f)));
    ASSERT_TRUE(bool(block.setElement("weights", 3u, 4.0f)));
    ASSERT_TRUE(bool(block.update()));

    compages::gpu::BlockMember const* weights = block.info().find("weights");
    ASSERT_NE(weights, nullptr);
    ASSERT_EQ(weights->elements, 4);
    ASSERT_EQ(weights->array_stride, 16u);

    const auto bytes = deviceBytes(block);
    ASSERT_FLOAT_EQ(floatAt(bytes, weights->offset), 1.0f);
    ASSERT_FLOAT_EQ(floatAt(bytes, weights->offset + (3u * weights->array_stride)),
                    4.0f);
    // The three bytes after the first float are padding, not the next element.
    ASSERT_FLOAT_EQ(floatAt(bytes, weights->offset + 4u), 0.0f);
}

//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, RefusesAnElementPastTheEndOfTheArray)
{
    auto program = linked(FRAME_FRAGMENT);
    auto made = compages::gpu::UniformBlock::create(program, "Frame");
    ASSERT_TRUE(bool(made)) << made.error();
    auto block = made.take();

    auto written = block.setElement("weights", 4u, 1.0f);
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("holds 4"));
    ASSERT_THAT(written.error(), HasSubstr("element number 4"));
}

//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, RefusesAMemberTheBlockDoesNotHave)
{
    auto program = linked(FRAME_FRAGMENT);
    auto made = compages::gpu::UniformBlock::create(program, "Frame");
    ASSERT_TRUE(bool(made)) << made.error();
    auto block = made.take();

    auto written = block.set("camera", 1.0f);
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("camera"));
    ASSERT_THAT(written.error(), HasSubstr("projection"));
}

//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, RefusesATypeTheShaderDidNotDeclare)
{
    auto program = linked(FRAME_FRAGMENT);
    auto made = compages::gpu::UniformBlock::create(program, "Frame");
    ASSERT_TRUE(bool(made)) << made.error();
    auto block = made.take();

    auto written = block.set("exposure", compages::core::Vector3f(1.0f, 0.0f, 0.0f));
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("float"));
    ASSERT_THAT(written.error(), HasSubstr("vec3"));
}

//------------------------------------------------------------------------------
// A mat3 is 36 bytes in C++ and 48 in the block: three columns of twelve, each
// padded to sixteen. Copying the 36 bytes whole would put the second column where
// the shader expects the padding of the first. The columns travel separately,
// at the stride the driver reported.
//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, CopiesAMat3OneColumnAtATime)
{
    auto program = linked(MAT3_FRAGMENT);
    auto made = compages::gpu::UniformBlock::create(program, "Basis");
    ASSERT_TRUE(bool(made)) << made.error();
    auto block = made.take();

    compages::core::Matrix33f axes(compages::core::matrix::Identity);
    ASSERT_TRUE(bool(block.set("axes", axes))) << block.describe();
    ASSERT_TRUE(bool(block.update()));

    compages::gpu::BlockMember const* member = block.info().find("axes");
    ASSERT_NE(member, nullptr);
    ASSERT_EQ(member->matrix_stride, 16u);

    const auto bytes = deviceBytes(block);
    ASSERT_FLOAT_EQ(floatAt(bytes, member->offset), 1.0f);
    ASSERT_FLOAT_EQ(floatAt(bytes, member->offset + 4u), 0.0f);
    ASSERT_FLOAT_EQ(floatAt(bytes, member->offset + 8u), 0.0f);
    // Padding of the first column, then the first element of the second.
    ASSERT_FLOAT_EQ(floatAt(bytes, member->offset + 12u), 0.0f);
    ASSERT_FLOAT_EQ(floatAt(bytes, member->offset + member->matrix_stride), 0.0f);
    ASSERT_FLOAT_EQ(
        floatAt(bytes, member->offset + member->matrix_stride + 4u), 1.0f);
}

//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, CopiesAMatchingStructWhole)
{
    auto program = linked(MATRICES_FRAGMENT);
    auto made =
        compages::gpu::TypedUniformBlock<Matrices>::create(program, "Matrices");
    ASSERT_TRUE(bool(made)) << made.error() << "\n" << program.reflection().toString();
    auto block = made.take();

    Matrices value;
    value.projection = compages::core::Matrix44f(compages::core::matrix::Identity);
    value.view = compages::core::Matrix44f(compages::core::matrix::Identity);
    value.view[0][0] = 7.0f;
    block.assign(value);
    ASSERT_TRUE(bool(block.bind()));

    compages::gpu::BlockMember const* view = block.untyped().info().find("view");
    ASSERT_NE(view, nullptr);
    const auto bytes = deviceBytes(block.untyped());
    ASSERT_FLOAT_EQ(floatAt(bytes, view->offset), 7.0f);
}

//------------------------------------------------------------------------------
// The translation of a transform sits in column 3 of the C++ matrix, and std140
// stores the columns one after the other: the typed copy and set() shall both
// put it in the last 16 bytes of the matrix.
//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, TypedCopyAndSetAgreeOnMatrixLayout)
{
    auto program = linked(MATRICES_FRAGMENT);
    auto made =
        compages::gpu::TypedUniformBlock<Matrices>::create(program, "Matrices");
    ASSERT_TRUE(bool(made)) << made.error();
    auto typed = made.take();
    auto plain_made = compages::gpu::UniformBlock::create(program, "Matrices");
    ASSERT_TRUE(bool(plain_made)) << plain_made.error();
    auto plain = plain_made.take();

    const compages::core::Matrix44f identity(compages::core::matrix::Identity);
    const compages::core::Matrix44f moved =
        compages::core::translate(identity, compages::core::Vector3f(1.0f, 2.0f, 3.0f));

    typed.assign(Matrices{ identity, moved });
    ASSERT_TRUE(bool(typed.bind()));
    ASSERT_TRUE(bool(plain.set("projection", identity)));
    ASSERT_TRUE(bool(plain.set("view", moved)));
    ASSERT_TRUE(bool(plain.update()));

    compages::gpu::BlockMember const* view = plain.info().find("view");
    ASSERT_NE(view, nullptr);
    const auto typed_bytes = deviceBytes(typed.untyped());
    const auto plain_bytes = deviceBytes(plain);
    ASSERT_EQ(typed_bytes, plain_bytes);
    ASSERT_FLOAT_EQ(floatAt(typed_bytes, view->offset + 48u), 1.0f);
    ASSERT_FLOAT_EQ(floatAt(typed_bytes, view->offset + 52u), 2.0f);
    ASSERT_FLOAT_EQ(floatAt(typed_bytes, view->offset + 56u), 3.0f);
}

//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, RefusesAStructWhoseNamesDoNotMatchTheShader)
{
    auto program = linked(MATRICES_FRAGMENT);
    auto made = compages::gpu::TypedUniformBlock<Renamed>::create(program, "Matrices");

    ASSERT_FALSE(bool(made));
    ASSERT_THAT(made.error(), HasSubstr("projection"));
    ASSERT_THAT(made.error(), HasSubstr("no member of that name"));
}

//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, RefusesAStructSmallerThanTheBlock)
{
    auto program = linked(MATRICES_FRAGMENT);
    auto made = compages::gpu::TypedUniformBlock<Half>::create(program, "Matrices");

    ASSERT_FALSE(bool(made));
    ASSERT_THAT(made.error(), HasSubstr("64"));
    ASSERT_THAT(made.error(), HasSubstr("128"));
}

//------------------------------------------------------------------------------
// The colour on the screen comes from the block. That is the test that the
// offsets, the upload and the binding point all agree with what the shader reads,
// rather than only with each other.
//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, TheShaderReadsWhatWasWritten)
{
    auto program = linked(COLOUR_FRAGMENT);
    auto made = compages::gpu::UniformBlock::create(program, "Tint");
    ASSERT_TRUE(bool(made)) << made.error();
    auto block = made.take();

    ASSERT_TRUE(bool(block.set("colour", compages::core::Vector4f(0.0f, 1.0f, 0.0f, 1.0f))));
    ASSERT_TRUE(bool(block.bind()));

    auto vertices = compages::gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                              compages::gpu::BufferKind::Vertex,
                                              compages::gpu::BufferUsage::Immutable)
                        .take();
    const compages::gpu::VertexLayout layout = compages::gpu::VertexLayout::of<Corner>();
    auto pipeline = compages::gpu::Pipeline::create<Corner>(program, layout).take();

    auto pass = compages::gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();
    ASSERT_TRUE(bool(compages::gpu::attempt([&] { compages::gpu::draw(pipeline, vertices); })));

    auto picture = compages::gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    ASSERT_EQ(pixelAt(picture.value(), WIDTH / 2u, HEIGHT / 2u),
              (Pixel{ 0u, 255u, 0u, 255u }));
}

//------------------------------------------------------------------------------
// One block, two programs, the same binding point. Writing once is enough for
// both, which is why a frame of several passes is cheap and why 06_MultiPassMesh
// can share its matrices.
//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, OneBlockFeedsTwoPrograms)
{
    auto first = linked(COLOUR_FRAGMENT);
    auto second = linked(COLOUR_SQUARED_FRAGMENT);

    auto made = compages::gpu::UniformBlock::create(first, "Tint");
    ASSERT_TRUE(bool(made)) << made.error();
    auto block = made.take();
    ASSERT_TRUE(bool(second.bindUniformBlock("Tint", block.binding())));

    // Half intensity, so the second program, which squares the colour, paints a
    // darker green that cannot be mistaken for the first.
    ASSERT_TRUE(bool(block.set("colour", compages::core::Vector4f(0.0f, 0.5f, 0.0f, 1.0f))));
    ASSERT_TRUE(bool(block.bind()));

    auto vertices = compages::gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                              compages::gpu::BufferKind::Vertex,
                                              compages::gpu::BufferUsage::Immutable)
                        .take();
    const compages::gpu::VertexLayout layout = compages::gpu::VertexLayout::of<Corner>();
    auto left = compages::gpu::Pipeline::create<Corner>(first, layout).take();
    auto right = compages::gpu::Pipeline::create<Corner>(second, layout).take();

    {
        auto pass = compages::gpu::RenderPass::begin(
            { .width = WIDTH / 2u,
              .height = HEIGHT,
              .color = { 0.0f, 0.0f, 0.0f, 1.0f },
              .target = {} });
        ASSERT_TRUE(bool(pass)) << pass.error();
        ASSERT_TRUE(bool(compages::gpu::attempt([&] { compages::gpu::draw(left, vertices); })));
    }
    {
        auto pass = compages::gpu::RenderPass::begin(
            { .x = WIDTH / 2u,
              .width = WIDTH / 2u,
              .height = HEIGHT,
              .color = { 0.0f, 0.0f, 0.0f, 1.0f },
              .target = {} });
        ASSERT_TRUE(bool(pass)) << pass.error();
        ASSERT_TRUE(bool(compages::gpu::attempt([&] { compages::gpu::draw(right, vertices); })));
    }

    auto whole = compages::gpu::RenderPass::begin({ .width = WIDTH,
                                          .height = HEIGHT,
                                          .clear_color = false,
                                          .clear_depth = false,
                                          .target = {} });
    ASSERT_TRUE(bool(whole)) << whole.error();
    auto picture = compages::gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();

    const Pixel bright = pixelAt(picture.value(), (WIDTH / 2u) - 1u, HEIGHT / 2u);
    const Pixel dark = pixelAt(picture.value(), WIDTH / 2u, HEIGHT / 2u);
    ASSERT_EQ(bright.red, 0u);
    ASSERT_EQ(bright.blue, 0u);
    ASSERT_GT(bright.green, dark.green);
    ASSERT_GT(dark.green, 0u);
}

//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, BindSendsWhatWasWritten)
{
    auto program = linked(COLOUR_FRAGMENT);
    auto made = compages::gpu::UniformBlock::create(program, "Tint");
    ASSERT_TRUE(bool(made)) << made.error();
    auto block = made.take();

    // bind() is the call a frame makes. Forgetting the update first is the
    // one-frame lag that looks like the values of the previous frame, so the two
    // are one function.
    ASSERT_TRUE(bool(block.set("colour", compages::core::Vector4f(1.0f, 0.0f, 0.0f, 1.0f))));
    ASSERT_TRUE(bool(block.bind()));

    auto vertices = compages::gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                              compages::gpu::BufferKind::Vertex,
                                              compages::gpu::BufferUsage::Immutable)
                        .take();
    const compages::gpu::VertexLayout layout = compages::gpu::VertexLayout::of<Corner>();
    auto pipeline = compages::gpu::Pipeline::create<Corner>(program, layout).take();

    auto pass = compages::gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();
    ASSERT_TRUE(bool(compages::gpu::attempt([&] { compages::gpu::draw(pipeline, vertices); })));

    auto picture = compages::gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    ASSERT_EQ(pixelAt(picture.value(), WIDTH / 2u, HEIGHT / 2u),
              (Pixel{ 255u, 0u, 0u, 255u }));
}

//------------------------------------------------------------------------------
TEST_F(UniformBlockTest, ReleasingItGivesTheBufferBack)
{
    const std::size_t before = compages::gpu::liveBuffers();
    {
        auto program = linked(FRAME_FRAGMENT);
        auto made = compages::gpu::UniformBlock::create(program, "Frame");
        ASSERT_TRUE(bool(made)) << made.error();
        auto block = made.take();
        ASSERT_EQ(compages::gpu::liveBuffers(), before + 1u);
    }
    ASSERT_EQ(compages::gpu::liveBuffers(), before);
}
