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
#include "Compages/Core/File.hpp"

#include <array>
#include <cstring>
#include <string>
#include <vector>

using namespace tests;

namespace
{

//! \brief A run of bytes standing in for pixels, each one different so that a
//! write landing in the wrong place is visible.
std::vector<std::byte> pattern(std::size_t p_bytes)
{
    std::vector<std::byte> pixels(p_bytes);
    for (std::size_t i = 0u; i < p_bytes; ++i)
    {
        pixels[i] = static_cast<std::byte>((i * 7u) & 0xFFu);
    }
    return pixels;
}

//! \brief Where an image of the data repository is, whether the tests are run
//! from the root of the project or from the tests directory.
std::string dataPath(std::string const& p_name)
{
    for (const char* root :
         { "external/Compages-data/", "../external/Compages-data/" })
    {
        if (File::exist(root + p_name))
        {
            return root + p_name;
        }
    }
    return {};
}

//! \brief Four pixels of RGBA8, spelled out.
std::vector<std::byte> fourPixels(std::uint8_t p_first)
{
    std::vector<std::byte> pixels(16u);
    for (std::size_t i = 0u; i < 16u; ++i)
    {
        pixels[i] = static_cast<std::byte>(p_first + i);
    }
    return pixels;
}

} // namespace

// ****************************************************************************
//! \brief A live device, on an invisible context.
// ****************************************************************************
class TextureTest: public GPUTest
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
TEST_F(TextureTest, ReservesAnImageOfTheRightShape)
{
    auto created = compages::gpu::Texture::create({ .kind = compages::gpu::TextureKind::Texture2D,
                                          .format = compages::gpu::PixelFormat::RGBA8,
                                          .width = 64u,
                                          .height = 32u });
    ASSERT_TRUE(bool(created)) << created.error();

    auto texture = created.take();
    ASSERT_TRUE(texture.valid());
    ASSERT_EQ(texture.width(), 64u);
    ASSERT_EQ(texture.height(), 32u);
    ASSERT_EQ(texture.depth(), 1u);
    ASSERT_EQ(texture.levels(), 1u);
    ASSERT_EQ(texture.format(), compages::gpu::PixelFormat::RGBA8);
    ASSERT_EQ(texture.bytes(), 64u * 32u * 4u);
    ASSERT_EQ(compages::gpu::liveTextures(), 1u);
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, RefusesASizeOfZero)
{
    auto created = compages::gpu::Texture::create({ .width = 0u, .height = 32u });
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("zero width"));
}

//------------------------------------------------------------------------------
// A depth given to a 2D texture is a shape confusion, and saying which type was
// meant is more useful than silently ignoring it.
//------------------------------------------------------------------------------
TEST_F(TextureTest, RefusesADepthOnATwoDimensionalTexture)
{
    auto created =
        compages::gpu::Texture::create({ .width = 8u, .height = 8u, .depth = 4u });
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("Texture2DArray"));
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, RefusesACubeMapThatIsNotSquare)
{
    auto created = compages::gpu::Texture::create(
        { .kind = compages::gpu::TextureKind::TextureCube, .width = 64u, .height = 32u });
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("square"));
}

//------------------------------------------------------------------------------
// The driver's ceiling, named alongside the size asked for so that the size can
// be brought under it.
//------------------------------------------------------------------------------
TEST_F(TextureTest, RefusesATextureLargerThanTheDriverAllows)
{
    const auto too_big =
        static_cast<std::uint32_t>(compages::gpu::device().max_texture_size) + 2u;

    auto created =
        compages::gpu::Texture::create({ .width = too_big, .height = 4u, .levels = 1u });
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("allows at most"));
}

//------------------------------------------------------------------------------
// Asking for zero levels means the whole chain, down to a single pixel. A 64 by
// 32 texture therefore has seven, since the longest side halves six times.
//------------------------------------------------------------------------------
TEST_F(TextureTest, BuildsTheWholeChainOfDetailWhenAskedForZero)
{
    auto texture = compages::gpu::Texture::create({ .width = 64u,
                                          .height = 32u,
                                          .levels = 0u })
                       .take();

    ASSERT_EQ(texture.levels(), 7u);
    // Every level, all the way down, plus the largest: 64x32 and its halves.
    const std::size_t expected = (64u * 32u + 32u * 16u + 16u * 8u + 8u * 4u +
                                 4u * 2u + 2u * 1u + 1u * 1u) *
                                4u;
    ASSERT_EQ(texture.bytes(), expected);
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, RefusesMoreLevelsThanTheSizeAllows)
{
    auto created =
        compages::gpu::Texture::create({ .width = 4u, .height = 4u, .levels = 8u });
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("at most 3"));
}

//------------------------------------------------------------------------------
// The round trip that shows the pixels really reached the device and came back in
// the same order.
//------------------------------------------------------------------------------
TEST_F(TextureTest, GivesBackThePixelsThatWerePutIn)
{
    auto texture =
        compages::gpu::Texture::create({ .width = 4u, .height = 4u }).take();

    const std::vector<std::byte> sent = pattern(4u * 4u * 4u);
    auto written = texture.write(sent);
    ASSERT_TRUE(bool(written)) << written.error();

    auto read = texture.read();
    ASSERT_TRUE(bool(read)) << read.error();
    ASSERT_EQ(read.value(), sent);
}

//------------------------------------------------------------------------------
// The check the previous layer never made. Handing over fewer pixels than the
// region needs makes the driver read past the end of the caller's memory, with no
// message at all.
//------------------------------------------------------------------------------
TEST_F(TextureTest, RefusesTooFewPixelsForTheRegion)
{
    auto texture = compages::gpu::Texture::create({ .width = 4u, .height = 4u }).take();

    const std::vector<std::byte> not_enough = pattern(4u * 4u * 3u);
    auto written = texture.write(not_enough);
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("needs 64 bytes"));
    ASSERT_THAT(written.error(), HasSubstr("48 were given"));
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, RefusesTooManyPixelsForTheRegion)
{
    auto texture = compages::gpu::Texture::create({ .width = 4u, .height = 4u }).take();

    auto written = texture.write(pattern(4u * 4u * 4u + 4u));
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("needs 64 bytes"));
}

//------------------------------------------------------------------------------
// Writing a part of an image, which is what an animated region or a font atlas
// does, and the rest must be left alone.
//------------------------------------------------------------------------------
TEST_F(TextureTest, WritesPartOfAnImageLeavingTheRestAlone)
{
    auto texture = compages::gpu::Texture::create({ .width = 4u, .height = 4u }).take();
    ASSERT_TRUE(bool(texture.write(std::vector<std::byte>(64u))));

    // Two by two pixels in the middle, so neither corner is touched.
    const std::vector<std::byte> patch = fourPixels(100u);
    auto written = texture.write(1u, 1u, 0u, 2u, 2u, 1u, patch);
    ASSERT_TRUE(bool(written)) << written.error();

    auto read = texture.read();
    ASSERT_TRUE(bool(read)) << read.error();
    std::vector<std::byte> const& got = read.value();

    // The first row is untouched.
    for (std::size_t i = 0u; i < 16u; ++i)
    {
        ASSERT_EQ(got[i], std::byte{ 0 }) << "at byte " << i;
    }
    // The patch landed at the second pixel of the second row, which is byte
    // (1 * 4 + 1) * 4 = 20.
    ASSERT_EQ(got[20], std::byte{ 100 });
    ASSERT_EQ(got[21], std::byte{ 101 });
    // And the pixel before it is still empty.
    ASSERT_EQ(got[16], std::byte{ 0 });
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, RefusesARegionThatRunsOutsideTheImage)
{
    auto texture = compages::gpu::Texture::create({ .width = 4u, .height = 4u }).take();

    auto written =
        texture.write(2u, 2u, 0u, 4u, 4u, 1u, pattern(4u * 4u * 4u));
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("outside the texture"));
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, RefusesALevelThatDoesNotExist)
{
    auto texture = compages::gpu::Texture::create({ .width = 4u, .height = 4u }).take();

    auto written = texture.write(pattern(64u), 3u);
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("level of detail 3 does not exist"));
}

//------------------------------------------------------------------------------
// Every level of detail is half the size, so a write to level one wants a quarter
// of the pixels.
//------------------------------------------------------------------------------
TEST_F(TextureTest, WritesASmallerLevelOfDetail)
{
    auto texture =
        compages::gpu::Texture::create({ .width = 8u, .height = 8u, .levels = 0u }).take();
    ASSERT_EQ(texture.levels(), 4u);

    const std::vector<std::byte> half = pattern(4u * 4u * 4u);
    auto written = texture.write(half, 1u);
    ASSERT_TRUE(bool(written)) << written.error();

    auto read = texture.read(1u);
    ASSERT_TRUE(bool(read)) << read.error();
    ASSERT_EQ(read.value(), half);
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, BuildsTheSmallerLevelsFromTheLargest)
{
    auto texture =
        compages::gpu::Texture::create({ .width = 8u, .height = 8u, .levels = 0u }).take();
    ASSERT_TRUE(bool(texture.write(pattern(8u * 8u * 4u))));

    auto built = texture.generateMipmaps();
    ASSERT_TRUE(bool(built)) << built.error();

    // The smallest level is a single pixel, and it now holds something.
    auto read = texture.read(3u);
    ASSERT_TRUE(bool(read)) << read.error();
    ASSERT_EQ(read.value().size(), 4u);
}

//------------------------------------------------------------------------------
// Asking for mipmaps on a texture that has none is a mistake worth naming: the
// memory for them was never set aside, so nothing would happen.
//------------------------------------------------------------------------------
TEST_F(TextureTest, SaysSoWhenThereAreNoSmallerLevelsToBuild)
{
    auto texture = compages::gpu::Texture::create({ .width = 8u, .height = 8u }).take();

    auto built = texture.generateMipmaps();
    ASSERT_FALSE(bool(built));
    ASSERT_THAT(built.error(), HasSubstr("single level of detail"));
}

//------------------------------------------------------------------------------
// A cube map is six square images, and its faces are written one at a time.
//------------------------------------------------------------------------------
TEST_F(TextureTest, HoldsTheSixFacesOfACubeMap)
{
    auto texture = compages::gpu::Texture::create(
                       { .kind = compages::gpu::TextureKind::TextureCube, .width = 4u,
                         .height = 4u })
                       .take();

    ASSERT_EQ(texture.depth(), 6u);
    ASSERT_EQ(texture.bytes(), 4u * 4u * 4u * 6u);

    for (std::uint32_t face = 0u; face < 6u; ++face)
    {
        const std::vector<std::byte> pixels =
            fourPixels(static_cast<std::uint8_t>(face * 16u));
        std::vector<std::byte> whole(4u * 4u * 4u);
        for (std::size_t i = 0u; i < whole.size(); ++i)
        {
            whole[i] = pixels[i % pixels.size()];
        }
        auto written = texture.writeLayer(face, whole);
        ASSERT_TRUE(bool(written)) << "face " << face << ": " << written.error();
    }

    // A read brings back all six faces one after the other.
    auto read = texture.read();
    ASSERT_TRUE(bool(read)) << read.error();
    ASSERT_EQ(read.value().size(), 4u * 4u * 4u * 6u);
    // The first byte of the first face and of the second differ, so the faces
    // really went to different places.
    ASSERT_NE(read.value()[0], read.value()[4u * 4u * 4u]);
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, RefusesAFaceThatDoesNotExist)
{
    auto texture = compages::gpu::Texture::create({ .kind = compages::gpu::TextureKind::TextureCube,
                                          .width = 4u,
                                          .height = 4u })
                       .take();

    auto written = texture.writeLayer(6u, pattern(4u * 4u * 4u));
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("layer 6 does not exist"));
}

//------------------------------------------------------------------------------
// A volume, whose depth halves along with its width and height.
//------------------------------------------------------------------------------
TEST_F(TextureTest, HoldsAVolumeWhoseDepthAlsoShrinks)
{
    auto texture = compages::gpu::Texture::create({ .kind = compages::gpu::TextureKind::Texture3D,
                                          .format = compages::gpu::PixelFormat::R8,
                                          .width = 8u,
                                          .height = 8u,
                                          .depth = 8u,
                                          .levels = 0u })
                       .take();

    ASSERT_EQ(texture.levels(), 4u);
    ASSERT_TRUE(bool(texture.write(pattern(8u * 8u * 8u))));

    // Level one is four by four by four, not four by four by eight.
    auto read = texture.read(1u);
    ASSERT_TRUE(bool(read)) << read.error();
    ASSERT_EQ(read.value().size(), 4u * 4u * 4u);
}

//------------------------------------------------------------------------------
// An array, whose layers are separate images and therefore do not shrink with the
// levels of detail. Getting this wrong is the difference between a write refused
// for the wrong reason and one silently accepted.
//------------------------------------------------------------------------------
TEST_F(TextureTest, KeepsEveryLayerOfAnArrayAtEveryLevel)
{
    auto texture =
        compages::gpu::Texture::create({ .kind = compages::gpu::TextureKind::Texture2DArray,
                               .format = compages::gpu::PixelFormat::R8,
                               .width = 8u,
                               .height = 8u,
                               .depth = 3u,
                               .levels = 0u })
            .take();

    ASSERT_EQ(texture.levels(), 4u);
    // Level one is four by four, still with three layers.
    auto read = texture.read(1u);
    ASSERT_TRUE(bool(read)) << read.error();
    ASSERT_EQ(read.value().size(), 4u * 4u * 3u);

    auto written = texture.writeLayer(2u, pattern(8u * 8u));
    ASSERT_TRUE(bool(written)) << written.error();
}

//------------------------------------------------------------------------------
// A one dimensional texture, used for colour ramps and lookup tables.
//------------------------------------------------------------------------------
TEST_F(TextureTest, HoldsASingleRowOfPixels)
{
    auto texture = compages::gpu::Texture::create({ .kind = compages::gpu::TextureKind::Texture1D,
                                          .format = compages::gpu::PixelFormat::RGBA8,
                                          .width = 256u })
                       .take();

    ASSERT_EQ(texture.bytes(), 256u * 4u);
    ASSERT_TRUE(bool(texture.write(pattern(256u * 4u))));
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, RefusesAHeightOnAOneDimensionalTexture)
{
    auto created = compages::gpu::Texture::create(
        { .kind = compages::gpu::TextureKind::Texture1D, .width = 16u, .height = 4u });
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("single row"));
}

//------------------------------------------------------------------------------
// The floating point formats a simulation keeps its state in. Precision here is
// the difference between a pattern that survives and one that decays.
//------------------------------------------------------------------------------
TEST_F(TextureTest, HoldsFloatingPointPixelsWithoutLosingThem)
{
    auto texture = compages::gpu::Texture::create({ .format = compages::gpu::PixelFormat::RGBA32F,
                                          .width = 2u,
                                          .height = 2u,
                                          .magnify = compages::gpu::Filter::Nearest,
                                          .minify = compages::gpu::Filter::Nearest })
                       .take();

    ASSERT_EQ(texture.bytes(), 2u * 2u * 16u);

    const std::vector<float> values{ 0.25f,   -1.5f,   1e6f,  0.0f,
                                     3.14159f, 2.5f,   -0.75f, 1.0f,
                                     0.1f,     0.2f,   0.3f,   0.4f,
                                     -1e-6f,   100.0f, 0.5f,   0.5f };
    std::vector<std::byte> bytes(values.size() * sizeof(float));
    std::memcpy(bytes.data(), values.data(), bytes.size());

    ASSERT_TRUE(bool(texture.write(bytes)));

    auto read = texture.read();
    ASSERT_TRUE(bool(read)) << read.error();
    std::vector<float> back(values.size());
    std::memcpy(back.data(), read.value().data(), read.value().size());
    ASSERT_EQ(back, values);
}

//------------------------------------------------------------------------------
// An integer format is handed whole numbers, not fractions, and the two need
// different words when talking to the driver. Getting it wrong yields a texture
// full of zeroes, which is why the round trip is worth checking.
//------------------------------------------------------------------------------
TEST_F(TextureTest, HoldsWholeNumbersAsWholeNumbers)
{
    auto texture = compages::gpu::Texture::create({ .format = compages::gpu::PixelFormat::R32UI,
                                          .width = 2u,
                                          .height = 2u,
                                          .magnify = compages::gpu::Filter::Nearest,
                                          .minify = compages::gpu::Filter::Nearest })
                       .take();

    const std::vector<std::uint32_t> values{ 1u, 70000u, 4000000000u, 42u };
    std::vector<std::byte> bytes(values.size() * sizeof(std::uint32_t));
    std::memcpy(bytes.data(), values.data(), bytes.size());
    ASSERT_TRUE(bool(texture.write(bytes)));

    auto read = texture.read();
    ASSERT_TRUE(bool(read)) << read.error();
    std::vector<std::uint32_t> back(values.size());
    std::memcpy(back.data(), read.value().data(), read.value().size());
    ASSERT_EQ(back, values);
    ASSERT_TRUE(compages::gpu::isIntegerFormat(texture.format()));
}

//------------------------------------------------------------------------------
// An image is how a compute pass writes into a texture, and only some formats may
// be bound that way.
//------------------------------------------------------------------------------
TEST_F(TextureTest, BindsAsAnImageForAComputePass)
{
    auto texture = compages::gpu::Texture::create({ .format = compages::gpu::PixelFormat::RGBA32F,
                                          .width = 8u,
                                          .height = 8u })
                       .take();

    auto bound = texture.bindAsImage(0u, compages::gpu::ImageAccess::ReadWrite);
    ASSERT_TRUE(bool(bound)) << bound.error();
}

//------------------------------------------------------------------------------
// Three channel formats cannot be written by a shader, and the symptom is a
// compute pass that produces nothing at all.
//------------------------------------------------------------------------------
TEST_F(TextureTest, RefusesToBindAThreeChannelTextureAsAnImage)
{
    auto texture = compages::gpu::Texture::create({ .format = compages::gpu::PixelFormat::RGB8,
                                          .width = 8u,
                                          .height = 8u })
                       .take();

    auto bound = texture.bindAsImage(0u, compages::gpu::ImageAccess::Write);
    ASSERT_FALSE(bool(bound));
    ASSERT_THAT(bound.error(), HasSubstr("Use RGBA8 instead"));
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, BindsToATextureUnit)
{
    auto texture = compages::gpu::Texture::create({ .width = 4u, .height = 4u }).take();

    auto bound = compages::gpu::attempt([&] { texture.bind(0u); });
    ASSERT_TRUE(bool(bound)) << bound.error();

    const auto units = static_cast<std::uint32_t>(compages::gpu::device().max_texture_units);
    auto too_far = compages::gpu::attempt([&] { texture.bind(units); });
    ASSERT_FALSE(bool(too_far));
    ASSERT_THAT(too_far.error(), HasSubstr("does not exist"));
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, ChangesHowItIsSampled)
{
    auto texture = compages::gpu::Texture::create({ .width = 4u, .height = 4u }).take();

    ASSERT_TRUE(bool(texture.setFilter(compages::gpu::Filter::Nearest, compages::gpu::Filter::Nearest)));
    ASSERT_EQ(texture.description().magnify, compages::gpu::Filter::Nearest);

    ASSERT_TRUE(bool(texture.setWrap(compages::gpu::Wrap::Repeat, compages::gpu::Wrap::MirroredRepeat)));
    ASSERT_EQ(texture.description().wrap_x, compages::gpu::Wrap::Repeat);
    ASSERT_EQ(texture.description().wrap_y, compages::gpu::Wrap::MirroredRepeat);
}

//------------------------------------------------------------------------------
// What an overlay watches to say how much memory a scene is using, and what a
// leak shows up in.
//------------------------------------------------------------------------------
TEST_F(TextureTest, AddsUpTheMemoryItUses)
{
    ASSERT_EQ(compages::gpu::textureMemory(), 0u);
    {
        auto first = compages::gpu::Texture::create({ .width = 16u, .height = 16u }).take();
        auto second =
            compages::gpu::Texture::create({ .format = compages::gpu::PixelFormat::R8,
                                   .width = 8u,
                                   .height = 8u })
                .take();

        ASSERT_EQ(compages::gpu::liveTextures(), 2u);
        ASSERT_EQ(compages::gpu::textureMemory(), (16u * 16u * 4u) + (8u * 8u));
    }
    ASSERT_EQ(compages::gpu::liveTextures(), 0u);
    ASSERT_EQ(compages::gpu::textureMemory(), 0u);
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, HandsTheImageOverWhenMoved)
{
    auto first = compages::gpu::Texture::create({ .width = 4u, .height = 4u }).take();
    const compages::gpu::TextureHandle handle = first.handle();

    compages::gpu::Texture second = std::move(first);
    ASSERT_EQ(compages::gpu::liveTextures(), 1u);
    ASSERT_FALSE(first.valid());
    ASSERT_TRUE(second.valid());
    ASSERT_EQ(second.handle(), handle);
    ASSERT_EQ(second.width(), 4u);
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, RefusesToUseAnImageThatIsGone)
{
    auto texture = compages::gpu::Texture::create({ .width = 4u, .height = 4u }).take();
    texture.release();

    ASSERT_FALSE(texture.valid());
    ASSERT_EQ(texture.width(), 0u);
    ASSERT_EQ(texture.bytes(), 0u);

    auto written = texture.write(pattern(64u));
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("no longer exists"));
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, RefusesToCreateWithoutADevice)
{
    compages::gpu::shutdown();

    auto created = compages::gpu::Texture::create({ .width = 4u, .height = 4u });
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("compages::gpu::init()"));
}

//------------------------------------------------------------------------------
// A missing file is reported in the words of the loader, since those words are
// what says whether the file is absent or merely not an image.
//------------------------------------------------------------------------------
TEST_F(TextureTest, SaysSoWhenAnImageFileCannotBeRead)
{
    auto created = compages::gpu::Texture::fromFile("there/is/no/such.png");
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("there/is/no/such.png"));
    ASSERT_EQ(compages::gpu::liveTextures(), 0u);
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, SaysSoWhenAFileIsNotAnImage)
{
    // A file that certainly exists and certainly is not an image.
    auto created = compages::gpu::Texture::fromFile("Makefile");
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("cannot read the image"));
    ASSERT_EQ(compages::gpu::liveTextures(), 0u);
}

//------------------------------------------------------------------------------
// Reading a real image file, which is the whole point of bringing in stb_image.
// Skipped rather than failed when the data repository is not checked out.
//------------------------------------------------------------------------------
TEST_F(TextureTest, ReadsAnImageFile)
{
    const std::string path = dataPath("wooden-crate.jpg");
    if (path.empty())
    {
        GTEST_SKIP() << "the data repository is not checked out";
    }

    auto created = compages::gpu::Texture::fromFile(path);
    ASSERT_TRUE(bool(created)) << created.error();

    auto texture = created.take();
    ASSERT_GT(texture.width(), 0u);
    ASSERT_GT(texture.height(), 0u);
    // A photograph has three or four channels, and mipmaps by default.
    ASSERT_GE(texture.levels(), 2u);
    ASSERT_GT(texture.bytes(), texture.width() * texture.height());
    ASSERT_EQ(texture.kind(), compages::gpu::TextureKind::Texture2D);
}

//------------------------------------------------------------------------------
// A photograph holds colours that were already adjusted for a screen, so reading
// it as sRGB is what lets the hardware undo that adjustment before any lighting
// is computed.
//------------------------------------------------------------------------------
TEST_F(TextureTest, ReadsAPhotographAsAdjustedForAScreen)
{
    const std::string path = dataPath("wooden-crate.jpg");
    if (path.empty())
    {
        GTEST_SKIP() << "the data repository is not checked out";
    }

    auto created = compages::gpu::Texture::fromFile(path, { .srgb = true });
    ASSERT_TRUE(bool(created)) << created.error();
    ASSERT_TRUE(compages::gpu::isSrgb(created.value().format()));
}

//------------------------------------------------------------------------------
// The rows of an image file run the other way round from the texture coordinates
// a shader uses, so turning them over is the default. Reading the same file both
// ways shows the two really differ, which is the check that the flag does
// something.
//------------------------------------------------------------------------------
TEST_F(TextureTest, TurnsTheRowsOfAnImageOverByDefault)
{
    const std::string path = dataPath("hazard.png");
    if (path.empty())
    {
        GTEST_SKIP() << "the data repository is not checked out";
    }

    auto flipped = compages::gpu::Texture::fromFile(path, { .mipmaps = false });
    ASSERT_TRUE(bool(flipped)) << flipped.error();
    auto as_stored =
        compages::gpu::Texture::fromFile(path, { .flip_vertically = false, .mipmaps = false });
    ASSERT_TRUE(bool(as_stored)) << as_stored.error();

    auto one = flipped.value().read();
    auto other = as_stored.value().read();
    ASSERT_TRUE(bool(one)) << one.error();
    ASSERT_TRUE(bool(other)) << other.error();
    ASSERT_EQ(one.value().size(), other.value().size());
    ASSERT_NE(one.value(), other.value());
}

//------------------------------------------------------------------------------
// The six images of a sky box, read in one go.
//------------------------------------------------------------------------------
TEST_F(TextureTest, ReadsTheSixFacesOfASkyBox)
{
    std::array<std::string, 6u> paths{ dataPath("right.jpg"),
                                       dataPath("left.jpg"),
                                       dataPath("top.jpg"),
                                       dataPath("bottom.jpg"),
                                       dataPath("front.jpg"),
                                       dataPath("back.jpg") };
    for (std::string const& path : paths)
    {
        if (path.empty())
        {
            GTEST_SKIP() << "the data repository is not checked out";
        }
    }

    auto created = compages::gpu::Texture::cubeFromFiles(paths);
    ASSERT_TRUE(bool(created)) << created.error();

    auto texture = created.take();
    ASSERT_EQ(texture.kind(), compages::gpu::TextureKind::TextureCube);
    ASSERT_EQ(texture.depth(), 6u);
    ASSERT_EQ(texture.width(), texture.height());
    ASSERT_GT(texture.width(), 0u);
}

//------------------------------------------------------------------------------
// Faces of different sizes are the mistake a sky box made of images from
// different sources runs into, and it must name which file is the odd one.
//------------------------------------------------------------------------------
TEST_F(TextureTest, RefusesFacesOfDifferentSizes)
{
    const std::string square = dataPath("right.jpg");
    const std::string other = dataPath("hazard.png");
    if (square.empty() || other.empty())
    {
        GTEST_SKIP() << "the data repository is not checked out";
    }

    std::array<std::string, 6u> paths{ square, other, square,
                                       square, square, square };
    auto created = compages::gpu::Texture::cubeFromFiles(paths);
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr(other));
    ASSERT_EQ(compages::gpu::liveTextures(), 0u);
}

//------------------------------------------------------------------------------
TEST_F(TextureTest, SaysSoWhenAFaceOfACubeMapIsMissing)
{
    const std::string square = dataPath("right.jpg");
    if (square.empty())
    {
        GTEST_SKIP() << "the data repository is not checked out";
    }

    std::array<std::string, 6u> paths{ square, square, square,
                                       square, square, "there/is/no/such.jpg" };
    auto created = compages::gpu::Texture::cubeFromFiles(paths);
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("there/is/no/such.jpg"));
    // Nothing was set aside on the device before the files were all read.
    ASSERT_EQ(compages::gpu::liveTextures(), 0u);
}
