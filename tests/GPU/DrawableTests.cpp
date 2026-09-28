// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPUContext.hpp"

#include "Compages/GPU/GPU.hpp"

#include <vector>

using namespace tests;
using ::testing::HasSubstr;

namespace
{

constexpr int WIDTH = 16;
constexpr int HEIGHT = 16;

//! \brief Position and colour, as two attributes.
constexpr const char* COLORED_VERTEX = R"(#version 450 core
in vec2 position;
in vec3 color;
out vec3 vColor;
void main() { vColor = color; gl_Position = vec4(position, 0.0, 1.0); }
)";

constexpr const char* COLORED_FRAGMENT = R"(#version 450 core
in vec3 vColor;
out vec4 oColor;
void main() { oColor = vec4(vColor, 1.0); }
)";

//! \brief Position only, painted with a uniform colour.
constexpr const char* PLAIN_VERTEX = R"(#version 450 core
in vec2 position;
void main() { gl_Position = vec4(position, 0.0, 1.0); }
)";

constexpr const char* TINTED_FRAGMENT = R"(#version 450 core
uniform vec3 tint;
uniform float strength;
out vec4 oColor;
void main() { oColor = vec4(tint * strength, 1.0); }
)";

//! \brief A triangle covering the target, made from gl_VertexID alone.
constexpr const char* GENERATED_VERTEX = R"(#version 450 core
void main()
{
    vec2 corners[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(corners[gl_VertexID], 0.0, 1.0);
}
)";

constexpr const char* SAMPLING_FRAGMENT = R"(#version 450 core
uniform sampler2D image;
out vec4 oColor;
void main() { oColor = texture(image, vec2(0.5)); }
)";

//! \brief The vertex the colored shader reads, as a struct.
struct Vertex
{
    Vector2f position;
    Vector3f color;
};

//! \brief The red, green, blue of the pixel in the middle of the target.
std::array<int, 3> middle()
{
    auto picture = compages::gpu::readPixels();
    EXPECT_TRUE(bool(picture)) << picture.error();
    if (!picture)
    {
        return { -1, -1, -1 };
    }
    const std::size_t at = ((std::size_t(HEIGHT / 2) * WIDTH) + WIDTH / 2) * 4u;
    auto const& bytes = picture.value();
    return { int(bytes[at]), int(bytes[at + 1u]), int(bytes[at + 2u]) };
}

const std::array<int, 3> RED{ 255, 0, 0 };
const std::array<int, 3> GREEN{ 0, 255, 0 };

} // namespace

// ****************************************************************************
//! \brief A live device, a small target, and a pass opened over it.
// ****************************************************************************
class DrawableTest: public GPUTest
{
protected:

    void SetUp() override
    {
        GPUTest::SetUp();
        auto ready = compages::gpu::init(GPUContext::procAddress());
        ASSERT_TRUE(bool(ready)) << ready.error();
        (void)compages::gpu::takeFrameError();
    }

    void TearDown() override
    {
        (void)compages::gpu::takeFrameError();
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

    //! \brief Draw once in a pass covering the target and give the colour of
    //! its middle, or fail with the frame error.
    template <typename F>
    std::array<int, 3> drawn(F&& p_draw)
    {
        auto pass = compages::gpu::RenderPass::begin(
            compages::gpu::PassDesc{ .width = WIDTH, .height = HEIGHT, .target = {} });
        EXPECT_TRUE(bool(pass)) << pass.error();
        auto status = compages::gpu::attempt(std::forward<F>(p_draw));
        EXPECT_TRUE(bool(status)) << status.error();
        return middle();
    }
};

// The whole of 01b: values given attribute by attribute, stored interleaved.
TEST_F(DrawableTest, DrawsAttributesGivenByName)
{
    compages::gpu::Drawable triangle;
    ASSERT_TRUE(bool(triangle.load(COLORED_VERTEX, COLORED_FRAGMENT)));
    triangle["position"] = { { -1.0f, -1.0f }, { 3.0f, -1.0f }, { -1.0f, 3.0f } };
    triangle["color"] = { { 0, 1, 0 }, { 0, 1, 0 }, { 0, 1, 0 } };

    ASSERT_EQ(triangle.count(), 3u);
    ASSERT_EQ(triangle.layout().stride(), 5u * sizeof(float));
    ASSERT_EQ(drawn([&] { triangle.draw(); }), GREEN);
}

// The whole of 01c: the same triangle from a struct.
TEST_F(DrawableTest, DrawsVerticesGivenAsAStruct)
{
    compages::gpu::Drawable triangle;
    ASSERT_TRUE(bool(triangle.load(COLORED_VERTEX, COLORED_FRAGMENT)));
    triangle.vertices<Vertex>({ { { -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } },
                                { { 3.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } },
                                { { -1.0f, 3.0f }, { 1.0f, 0.0f, 0.0f } } });
    ASSERT_EQ(drawn([&] { triangle.draw(); }), RED);
}

// A vertex changed in place reaches the GPU at the next draw, unasked.
TEST_F(DrawableTest, SendsWhatChangedAtTheNextDraw)
{
    compages::gpu::Drawable triangle;
    ASSERT_TRUE(bool(triangle.load(COLORED_VERTEX, COLORED_FRAGMENT)));
    triangle.vertices<Vertex>({ { { -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } },
                                { { 3.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } },
                                { { -1.0f, 3.0f }, { 1.0f, 0.0f, 0.0f } } });
    ASSERT_EQ(drawn([&] { triangle.draw(); }), RED);

    for (Vertex& vertex : triangle.vertices<Vertex>())
    {
        vertex.color = Vector3f(0.0f, 1.0f, 0.0f);
    }
    ASSERT_EQ(drawn([&] { triangle.draw(); }), GREEN);

    triangle["color"] = { { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 } };
    ASSERT_EQ(drawn([&] { triangle.draw(); }), RED);
}

// Immutable vertices are sent once; changing one afterwards is a frame error.
TEST_F(DrawableTest, RefusesToChangeImmutableVerticesOnceDrawn)
{
    compages::gpu::Drawable triangle;
    ASSERT_TRUE(bool(triangle.load(COLORED_VERTEX, COLORED_FRAGMENT)));
    triangle.vertices<Vertex>({ { { -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } },
                                { { 3.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } },
                                { { -1.0f, 3.0f }, { 1.0f, 0.0f, 0.0f } } });
    triangle.usage(compages::gpu::BufferUsage::Immutable);
    ASSERT_EQ(drawn([&] { triangle.draw(); }), RED);
    ASSERT_EQ(drawn([&] { triangle.draw(); }), RED);

    triangle.vertex<Vertex>(0u).color = Vector3f(0.0f, 1.0f, 0.0f);
    auto pass = compages::gpu::RenderPass::begin(
        compages::gpu::PassDesc{ .width = WIDTH, .height = HEIGHT, .target = {} });
    ASSERT_TRUE(bool(pass)) << pass.error();
    triangle.draw();
    ASSERT_TRUE(compages::gpu::hasFrameError());
    EXPECT_THAT(compages::gpu::takeFrameError(), HasSubstr("Immutable"));
}

// Uniforms by name, with the same syntax as attributes.
TEST_F(DrawableTest, SetsUniformsByName)
{
    compages::gpu::Drawable triangle;
    ASSERT_TRUE(bool(triangle.load(PLAIN_VERTEX, TINTED_FRAGMENT)));
    triangle["position"] = { { -1.0f, -1.0f }, { 3.0f, -1.0f }, { -1.0f, 3.0f } };
    triangle["tint"] = Vector3f(0.0f, 1.0f, 0.0f);
    triangle["strength"] = 1.0;
    ASSERT_EQ(drawn([&] { triangle.draw(); }), GREEN);

    triangle["tint"] = { 1.0f, 0.0f, 0.0f };
    ASSERT_EQ(drawn([&] { triangle.draw(); }), RED);
}

// Indices name the vertices to draw.
TEST_F(DrawableTest, DrawsThroughIndices)
{
    compages::gpu::Drawable square;
    ASSERT_TRUE(bool(square.load(PLAIN_VERTEX, TINTED_FRAGMENT)));
    square["position"] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
    square["tint"] = Vector3f(1.0f, 0.0f, 0.0f);
    square["strength"] = 1.0f;
    square.indices({ 0, 1, 2, 2, 3, 0 });
    ASSERT_EQ(drawn([&] { square.draw(); }), RED);
}

// A shader making its own vertices, sampling a texture given by name.
TEST_F(DrawableTest, DrawsWithoutVerticesAndSamplesATexture)
{
    compages::gpu::TextureDesc desc;
    desc.width = 1u;
    desc.height = 1u;
    desc.format = compages::gpu::PixelFormat::RGBA8;
    compages::gpu::Texture texture;
    ASSERT_TRUE(bool(texture.allocate(desc)));
    const std::array<std::uint8_t, 4> green{ 0u, 255u, 0u, 255u };
    ASSERT_TRUE(bool(texture.write(std::as_bytes(std::span(green)))));

    compages::gpu::Drawable screen;
    ASSERT_TRUE(bool(screen.load(GENERATED_VERTEX, SAMPLING_FRAGMENT)));
    screen["image"] = texture;
    ASSERT_EQ(drawn([&] { screen.draw(3u); }), GREEN);
}

// Mistakes are frame errors that say what the shader has.
TEST_F(DrawableTest, NamesWhatTheShaderHasWhenANameIsWrong)
{
    compages::gpu::Drawable triangle;
    ASSERT_TRUE(bool(triangle.load(COLORED_VERTEX, COLORED_FRAGMENT)));
    triangle["positon"] = { { 0, 0 }, { 1, 0 }, { 0, 1 } };

    ASSERT_TRUE(compages::gpu::hasFrameError());
    const std::string error = compages::gpu::takeFrameError();
    EXPECT_THAT(error, HasSubstr("'positon'"));
    EXPECT_THAT(error, HasSubstr("position"));
}

TEST_F(DrawableTest, RefusesTheWrongNumberOfComponents)
{
    compages::gpu::Drawable triangle;
    ASSERT_TRUE(bool(triangle.load(COLORED_VERTEX, COLORED_FRAGMENT)));
    triangle["color"] = { { 1, 0 }, { 0, 1 }, { 0, 0 } };
    EXPECT_THAT(compages::gpu::takeFrameError(), HasSubstr("vec3"));
}

TEST_F(DrawableTest, RefusesToDrawAttributesOfDifferentLengths)
{
    compages::gpu::Drawable triangle;
    ASSERT_TRUE(bool(triangle.load(COLORED_VERTEX, COLORED_FRAGMENT)));
    triangle["position"] = { { -1, -1 }, { 1, -1 }, { 0, 1 }, { 0, 0 } };
    triangle["color"] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

    auto prepared = triangle.prepare();
    ASSERT_FALSE(bool(prepared));
    EXPECT_THAT(prepared.error(), HasSubstr("'position' has 4 values"));
}

TEST_F(DrawableTest, SaysWhichAttributeWasNeverGiven)
{
    compages::gpu::Drawable triangle;
    ASSERT_TRUE(bool(triangle.load(COLORED_VERTEX, COLORED_FRAGMENT)));
    triangle["position"] = { { -1, -1 }, { 1, -1 }, { 0, 1 } };

    auto prepared = triangle.prepare();
    ASSERT_FALSE(bool(prepared));
    EXPECT_THAT(prepared.error(), HasSubstr("'color'"));
}

TEST_F(DrawableTest, TellsAnAttributeFromAUniform)
{
    compages::gpu::Drawable triangle;
    ASSERT_TRUE(bool(triangle.load(PLAIN_VERTEX, TINTED_FRAGMENT)));
    triangle["position"] = Vector2f(0.0f, 0.0f);
    EXPECT_THAT(compages::gpu::takeFrameError(), HasSubstr("attribute"));
    triangle["tint"] = { { 1, 0, 0 } };
    EXPECT_THAT(compages::gpu::takeFrameError(), HasSubstr("uniform"));
}

// compages::gpu::clear() fills the target of the open pass, whatever opened it.
TEST_F(DrawableTest, ClearsTheOpenPass)
{
    compages::gpu::RenderPass pass(compages::gpu::PassDesc{ .width = WIDTH, .height = HEIGHT });
    ASSERT_TRUE(pass.open());
    compages::gpu::clear({ 0.0f, 1.0f, 0.0f });
    ASSERT_EQ(middle(), GREEN);
}

// Drawing into a texture from inside the window pass, then showing it.
TEST_F(DrawableTest, DrawsIntoATextureFromInsideAnotherPass)
{
    compages::gpu::TextureDesc desc;
    desc.width = WIDTH;
    desc.height = HEIGHT;
    compages::gpu::Texture color;
    ASSERT_TRUE(bool(color.allocate(desc)));
    compages::gpu::Framebuffer framebuffer;
    ASSERT_TRUE(bool(framebuffer.attach(color)));

    compages::gpu::Drawable screen;
    ASSERT_TRUE(bool(screen.load(GENERATED_VERTEX, SAMPLING_FRAGMENT)));

    compages::gpu::RenderPass window(compages::gpu::PassDesc{ .width = WIDTH, .height = HEIGHT });
    {
        compages::gpu::RenderPass offscreen(framebuffer, { .color = { 1.0f, 0.0f, 0.0f, 1.0f } });
        ASSERT_TRUE(offscreen.open());
    }
    screen["image"] = color;
    screen.draw(3u);
    ASSERT_FALSE(compages::gpu::hasFrameError()) << compages::gpu::takeFrameError();
    ASSERT_EQ(middle(), RED);
}

// A pass naming no target draws where the pass around it draws: a viewer can
// show an example in a texture without the example knowing.
TEST_F(DrawableTest, APassWithoutTargetDrawsIntoThePassAroundIt)
{
    compages::gpu::TextureDesc desc;
    desc.width = WIDTH;
    desc.height = HEIGHT;
    compages::gpu::Texture color;
    ASSERT_TRUE(bool(color.allocate(desc)));
    compages::gpu::Framebuffer framebuffer;
    ASSERT_TRUE(bool(framebuffer.attach(color)));

    compages::gpu::Drawable screen;
    ASSERT_TRUE(bool(screen.load(GENERATED_VERTEX, SAMPLING_FRAGMENT)));

    compages::gpu::RenderPass window(compages::gpu::PassDesc{ .width = WIDTH, .height = HEIGHT });
    compages::gpu::clear({ 0.0f, 0.0f, 1.0f });
    {
        compages::gpu::RenderPass viewer(framebuffer);
        compages::gpu::RenderPass example(compages::gpu::PassDesc{ .width = WIDTH, .height = HEIGHT,
                                               .color = { 0.0f, 1.0f, 0.0f, 1.0f } });
        ASSERT_TRUE(example.open());
        EXPECT_EQ(example.description().target, framebuffer.handle());
    }
    screen["image"] = color;
    screen.draw(3u);
    ASSERT_FALSE(compages::gpu::hasFrameError()) << compages::gpu::takeFrameError();
    ASSERT_EQ(middle(), GREEN);
}
