// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"

#include "DataPath.hpp"
#include "Compages/GPU/Device.hpp"
#include "Renderer/Render/TextDraw.hpp"

using compages::renderer::FontAtlas;
using tests::dataPath;

//--------------------------------------------------------------------------
TEST(TextOnPicture, DecodesUtf8)
{
    using compages::renderer::decodeUtf8;
    // ASCII, Latin-1 (two bytes), the euro (three bytes), an emoji (four).
    EXPECT_EQ(decodeUtf8("Mars"), (std::vector<char32_t>{ U'M', U'a', U'r', U's' }));
    EXPECT_EQ(decodeUtf8("Été"), (std::vector<char32_t>{ 0xC9u, U't', 0xE9u }));
    EXPECT_EQ(decodeUtf8("€"), (std::vector<char32_t>{ 0x20ACu }));
    EXPECT_EQ(decodeUtf8("\xF0\x9F\x9A\x80"), (std::vector<char32_t>{ 0x1F680u }));
    // A cut sequence and a lone continuation byte are replaced.
    EXPECT_EQ(decodeUtf8("a\xC3"), (std::vector<char32_t>{ U'a', 0xFFFDu }));
    EXPECT_EQ(decodeUtf8("\x80"), (std::vector<char32_t>{ 0xFFFDu }));
}

//--------------------------------------------------------------------------
TEST(TextOnPicture, BakesAFontWithoutTheGpu)
{
    const std::filesystem::path font = dataPath("fonts/DejaVuSans.ttf");
    if (font.empty())
    {
        GTEST_SKIP() << "fonts/DejaVuSans.ttf is missing from Compages-data";
    }
    ASSERT_FALSE(compages::gpu::initialized());
    compages::Result<FontAtlas> baked = compages::renderer::bakeFont(font);
    ASSERT_TRUE(baked) << baked.error();
    FontAtlas const& atlas = baked.value();

    EXPECT_EQ(atlas.width, 1024u);
    EXPECT_EQ(atlas.height % 4u, 0u);
    EXPECT_EQ(atlas.pixels.size(), std::size_t(atlas.width) * atlas.height);
    EXPECT_GT(atlas.ascent, 0.0f);
    EXPECT_LT(atlas.descent, 0.0f);

    // A letter has a field, padded around it, inside the atlas.
    FontAtlas::Glyph const& m = atlas.glyph(U'M');
    EXPECT_GT(m.width, std::uint32_t(2 * FontAtlas::PADDING));
    EXPECT_LE(m.x + m.width, atlas.width);
    EXPECT_LE(m.y + m.height, atlas.height);
    EXPECT_GT(m.advance, 0.0f);
    // An accent is baked; a space advances and draws nothing; a letter out
    // of Latin-1 is written as '?'.
    EXPECT_GT(atlas.glyph(0xE9u).width, 0u);
    EXPECT_EQ(atlas.glyph(U' ').width, 0u);
    EXPECT_GT(atlas.glyph(U' ').advance, 0.0f);
    EXPECT_EQ(&atlas.glyph(0x20ACu), &atlas.glyph(U'?'));
    // The dot is a disc centred on the point: inside at its centre, outside
    // in its corner.
    FontAtlas::Glyph const& dot = atlas.glyph(FontAtlas::DOT);
    auto field = [&](std::uint32_t p_x, std::uint32_t p_y)
    { return std::to_integer<int>(atlas.pixels[(p_y * atlas.width) + p_x]); };
    EXPECT_GT(field(dot.x + (dot.width / 2u), dot.y + (dot.height / 2u)), 128);
    EXPECT_LT(field(dot.x, dot.y), 128);
    EXPECT_FLOAT_EQ(dot.left, -0.5f * float(dot.width));
}

//--------------------------------------------------------------------------
TEST(TextOnPicture, RefusesWhatIsNotAFont)
{
    EXPECT_FALSE(compages::renderer::bakeFont("no/such/font.ttf"));
    const std::filesystem::path picture = dataPath("planets/moon.jpg");
    if (!picture.empty())
    {
        EXPECT_FALSE(compages::renderer::bakeFont(picture));
    }
}
