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

namespace
{

constexpr int WIDTH = 32;
constexpr int HEIGHT = 32;

struct Corner
{
    Vector2f position;
};

const std::vector<Corner> BIG_TRIANGLE{ { { -1.0f, -1.0f } },
                                        { { 3.0f, -1.0f } },
                                        { { -1.0f, 3.0f } } };

constexpr const char* VERTEX = R"(#version 450 core
in vec2 position;
void main() { gl_Position = vec4(position, 0.0, 1.0); }
)";

constexpr const char* RED_FRAGMENT = R"(#version 450 core
out vec4 oColor;
void main() { oColor = vec4(1.0, 0.0, 0.0, 1.0); }
)";

compages::gpu::Texture colorTarget(std::uint32_t p_width = 16u,
                         std::uint32_t p_height = 16u)
{
    auto made = compages::gpu::Texture::create({ .kind = compages::gpu::TextureKind::Texture2D,
                                       .format = compages::gpu::PixelFormat::RGBA8,
                                       .width = p_width,
                                       .height = p_height,
                                       .levels = 1u });
    EXPECT_TRUE(bool(made)) << made.error();
    return made ? made.take() : compages::gpu::Texture{};
}

compages::gpu::Texture depthTarget(std::uint32_t p_width = 16u,
                         std::uint32_t p_height = 16u)
{
    auto made = compages::gpu::Texture::create({ .kind = compages::gpu::TextureKind::Texture2D,
                                       .format = compages::gpu::PixelFormat::Depth32F,
                                       .width = p_width,
                                       .height = p_height,
                                       .levels = 1u });
    EXPECT_TRUE(bool(made)) << made.error();
    return made ? made.take() : compages::gpu::Texture{};
}

} // namespace

class FramebufferTest: public GPUTest
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

TEST_F(FramebufferTest, MakesATargetFromAColourTexture)
{
    auto color = colorTarget();
    auto made = compages::gpu::Framebuffer::create(color);
    ASSERT_TRUE(bool(made)) << made.error();
    auto framebuffer = made.take();

    ASSERT_TRUE(framebuffer.valid());
    ASSERT_EQ(framebuffer.width(), 16u);
    ASSERT_EQ(framebuffer.height(), 16u);
    ASSERT_EQ(framebuffer.colorCount(), 1u);
    ASSERT_EQ(framebuffer.color(), color.handle());
    ASSERT_FALSE(framebuffer.hasDepth());
    ASSERT_EQ(compages::gpu::liveFramebuffers(), 1u);
}

TEST_F(FramebufferTest, AcceptsADepthTextureBesideTheColour)
{
    auto color = colorTarget();
    auto depth = depthTarget();
    auto made = compages::gpu::Framebuffer::create(color, depth);
    ASSERT_TRUE(bool(made)) << made.error();
    auto framebuffer = made.take();

    ASSERT_TRUE(framebuffer.hasDepth());
    ASSERT_EQ(framebuffer.depth(), depth.handle());
}

TEST_F(FramebufferTest, AcceptsADepthOnlyTarget)
{
    auto depth = depthTarget();
    auto made =
        compages::gpu::Framebuffer::create(std::span<const compages::gpu::Attachment>(),
                                 compages::gpu::Attachment{ depth.handle(), 0u });
    ASSERT_TRUE(bool(made)) << made.error();
    ASSERT_EQ(made.value().colorCount(), 0u);
    ASSERT_TRUE(made.value().hasDepth());
}

TEST_F(FramebufferTest, RefusesADepthFormatAsColour)
{
    auto depth = depthTarget();
    auto made = compages::gpu::Framebuffer::create(depth);
    ASSERT_FALSE(bool(made));
    ASSERT_THAT(made.error(), HasSubstr("depth format"));
}

TEST_F(FramebufferTest, RefusesAColourFormatAsDepth)
{
    auto color = colorTarget();
    auto made = compages::gpu::Framebuffer::create(
        std::span<const compages::gpu::Attachment>(),
        compages::gpu::Attachment{ color.handle(), 0u });
    ASSERT_FALSE(bool(made));
    ASSERT_THAT(made.error(), HasSubstr("colour format"));
}

TEST_F(FramebufferTest, RefusesAttachmentsOfDifferentSizes)
{
    auto color = colorTarget(16u, 16u);
    auto depth = depthTarget(8u, 8u);
    auto made = compages::gpu::Framebuffer::create(color, depth);
    ASSERT_FALSE(bool(made));
    ASSERT_THAT(made.error(), HasSubstr("8 by 8"));
    ASSERT_THAT(made.error(), HasSubstr("16 by 16"));
}

TEST_F(FramebufferTest, RefusesAReleasedTexture)
{
    auto color = colorTarget();
    const compages::gpu::TextureHandle stale = color.handle();
    color.release();

    compages::gpu::Attachment attachment{ stale, 0u };
    auto made = compages::gpu::Framebuffer::create(std::span<const compages::gpu::Attachment>(&attachment, 1u));
    ASSERT_FALSE(bool(made));
    ASSERT_THAT(made.error(), HasSubstr("released"));
}

TEST_F(FramebufferTest, RefusesNothingAtAll)
{
    auto made = compages::gpu::Framebuffer::create(std::span<const compages::gpu::Attachment>());
    ASSERT_FALSE(bool(made));
    ASSERT_THAT(made.error(), HasSubstr("no colour and no depth"));
}

// The picture is in the texture, not on the window. Reading the open pass is
// what says the draw went where we asked, rather than into the default
// framebuffer by accident.
TEST_F(FramebufferTest, APassDrawsIntoTheTexture)
{
    auto color = colorTarget(WIDTH, HEIGHT);
    auto made = compages::gpu::Framebuffer::create(color);
    ASSERT_TRUE(bool(made)) << made.error();
    auto framebuffer = made.take();

    auto program = compages::gpu::Program::fromSources(VERTEX, RED_FRAGMENT).take();
    const compages::gpu::VertexLayout layout = compages::gpu::VertexLayout::of<Corner>();
    auto pipeline = compages::gpu::Pipeline::create<Corner>(program, layout).take();
    auto vertices = compages::gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                              compages::gpu::BufferKind::Vertex,
                                              compages::gpu::BufferUsage::Immutable)
                        .take();

    compages::gpu::PassDesc desc;
    desc.width = WIDTH;
    desc.height = HEIGHT;
    desc.target = framebuffer.handle();
    desc.color = Vector4f(0.0f, 0.0f, 1.0f, 1.0f);
    auto pass = compages::gpu::RenderPass::begin(desc);
    ASSERT_TRUE(bool(pass)) << pass.error();
    ASSERT_TRUE(bool(compages::gpu::attempt([&] { compages::gpu::draw(pipeline, vertices); })));

    auto picture = compages::gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    const std::size_t at =
        ((static_cast<std::size_t>(HEIGHT / 2) * WIDTH) + (WIDTH / 2)) * 4u;
    ASSERT_EQ(static_cast<int>(picture.value()[at]), 255);
    ASSERT_EQ(static_cast<int>(picture.value()[at + 1u]), 0);
    ASSERT_EQ(static_cast<int>(picture.value()[at + 2u]), 0);

    // The pass has to close before the texture is read as an image: the picture
    // is in the texture, and that is what the next pass will sample.
    pass.value().end();
    auto stored = color.read();
    ASSERT_TRUE(bool(stored)) << stored.error();
    ASSERT_EQ(static_cast<int>(stored.value()[at]), 255);
    ASSERT_EQ(static_cast<int>(stored.value()[at + 1u]), 0);
    ASSERT_EQ(static_cast<int>(stored.value()[at + 2u]), 0);
}

TEST_F(FramebufferTest, RefusesAPassThatDoesNotFitTheTarget)
{
    auto color = colorTarget(8u, 8u);
    auto made = compages::gpu::Framebuffer::create(color);
    ASSERT_TRUE(bool(made)) << made.error();
    auto framebuffer = made.take();

    auto pass = compages::gpu::RenderPass::begin({ .width = 16u,
                                         .height = 8u,
                                         .target = framebuffer.handle() });
    ASSERT_FALSE(bool(pass));
    ASSERT_THAT(pass.error(), HasSubstr("16 by 8"));
    ASSERT_THAT(pass.error(), HasSubstr("8 by 8"));
}

TEST_F(FramebufferTest, RefusesAPassToAReleasedFramebuffer)
{
    auto color = colorTarget();
    auto made = compages::gpu::Framebuffer::create(color);
    ASSERT_TRUE(bool(made)) << made.error();
    auto framebuffer = made.take();
    const compages::gpu::FramebufferHandle stale = framebuffer.handle();
    framebuffer.release();

    auto pass = compages::gpu::RenderPass::begin(
        { .width = 16u, .height = 16u, .target = stale });
    ASSERT_FALSE(bool(pass));
    ASSERT_THAT(pass.error(), HasSubstr("released"));
}

TEST_F(FramebufferTest, ReleasingItDoesNotReleaseTheTextures)
{
    auto color = colorTarget();
    {
        auto made = compages::gpu::Framebuffer::create(color);
        ASSERT_TRUE(bool(made)) << made.error();
    }
    ASSERT_EQ(compages::gpu::liveFramebuffers(), 0u);
    ASSERT_TRUE(color.valid());
    ASSERT_EQ(compages::gpu::liveTextures(), 1u);
}
