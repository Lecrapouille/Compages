// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// ****************************************************************************
// The picture of a patch of the map: the sea bed seen through the water, the
// land coloured by height and wetness, the regions tinted, the edges of the
// cells, the rivers and their banks. Its alpha tells the shader where the
// water is (SEA_ALPHA, RIVER_ALPHA).
// ****************************************************************************

#include "10_Complete/Archipelago/Cells.hpp"
#include "10_Complete/Archipelago/Land.hpp"
#include "10_Complete/Archipelago/Noise.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace examples::archipelago
{

using compages::Vector2f;

namespace colours
{

struct Rgb
{
    float r;
    float g;
    float b;
};

static Rgb mix(Rgb p_a, Rgb p_b, float p_t)
{
    return { p_a.r + ((p_b.r - p_a.r) * p_t),
             p_a.g + ((p_b.g - p_a.g) * p_t),
             p_a.b + ((p_b.b - p_a.b) * p_t) };
}

// The colours of the map (sRGB).
static constexpr Rgb DEEP{ 0.07f, 0.24f, 0.42f };
static constexpr Rgb SHALLOW{ 0.16f, 0.52f, 0.68f };
static constexpr Rgb LAGOON{ 0.42f, 0.80f, 0.78f };
static constexpr Rgb FOAM{ 0.92f, 0.97f, 0.96f };
static constexpr Rgb SAND{ 0.90f, 0.84f, 0.64f };
static constexpr Rgb GRASS{ 0.58f, 0.75f, 0.40f };
static constexpr Rgb FOREST{ 0.32f, 0.53f, 0.29f };
static constexpr Rgb UPLAND{ 0.62f, 0.60f, 0.44f };
static constexpr Rgb ROCK{ 0.60f, 0.56f, 0.50f };
static constexpr Rgb SUMMIT{ 0.80f, 0.78f, 0.74f };
static constexpr Rgb BORDER{ 1.00f, 0.96f, 0.86f };
static constexpr Rgb RIVER{ 0.25f, 0.60f, 0.85f };
static constexpr Rgb BANK{ 0.30f, 0.42f, 0.36f };
//! \brief The hue of the regions of each island (they cycle).
static constexpr std::array<Rgb, 5> HUES = { Rgb{ 0.45f, 0.55f, 0.95f },
                                             Rgb{ 0.95f, 0.45f, 0.60f },
                                             Rgb{ 0.95f, 0.70f, 0.35f },
                                             Rgb{ 0.30f, 0.80f, 0.80f },
                                             Rgb{ 0.60f, 0.90f, 0.40f } };

} // namespace colours

using colours::mix;
using colours::Rgb;

//! \brief A line of the map, \p p_half wide on each side, at \p p_distance:
//! its coverage of a pixel. A line thinner than a pixel is fainter, not
//! wider: the far view keeps its lines light.
static float line(float p_distance, float p_half, float p_pixel)
{
    const float shown = std::max(p_half, 0.5f * p_pixel);
    return std::clamp(((shown - p_distance) / p_pixel) + 0.5f, 0.0f, 1.0f) *
           std::min(1.0f, p_half / shown);
}

// ----------------------------------------------------------------------------
//! \brief The sea bed seen through the water: lagoon, shallows, deep; two
//! lines of equal depth, as on a nautical chart.
// ----------------------------------------------------------------------------
static Rgb seaColour(float p_depth, float p_slope, float p_pixel)
{
    Rgb colour = mix(
        colours::LAGOON, colours::SHALLOW, smoothstep(0.0f, 18.0f, p_depth));
    colour = mix(colour, colours::DEEP, smoothstep(15.0f, 85.0f, p_depth));
    for (const float level : { 6.0f, 30.0f })
    {
        const float lines =
            line(std::abs(p_depth - level) / std::max(p_slope, 1.0e-3f),
                 1.5f,
                 p_pixel);
        colour = mix(colour, colours::FOAM, 0.25f * lines);
    }
    return colour;
}

// ----------------------------------------------------------------------------
//! \brief The ground of a land cell: its own wetness (a biome, after
//! Patel; the banks of the rivers are wetter), the height, a mosaic.
// ----------------------------------------------------------------------------
static Rgb groundColour(Land const& p_land,
                        std::size_t p_cell,
                        float p_height,
                        float p_river)
{
    const Vector2f site = p_land.sites[p_cell];
    const float wet =
        noise(site.x / 900.0f, site.y / 900.0f, p_land.seed + 41u) +
        (0.35f * (1.0f - smoothstep(0.0f, 250.0f, p_river)));
    Rgb colour =
        mix(colours::GRASS, colours::FOREST, smoothstep(0.45f, 0.7f, wet));
    colour = mix(colours::SAND, colour, smoothstep(2.0f, 7.0f, p_height));
    colour = mix(colour, colours::UPLAND, smoothstep(110.0f, 250.0f, p_height));
    colour = mix(colour, colours::ROCK, smoothstep(250.0f, 400.0f, p_height));
    colour = mix(colour, colours::SUMMIT, smoothstep(440.0f, 540.0f, p_height));
    const float tone = 0.95f + (0.1f * lattice(int(p_cell), 7, p_land.seed));
    return { colour.r * tone, colour.g * tone, colour.b * tone };
}

// ----------------------------------------------------------------------------
//! \brief The tint of the regions (the hue of the island, lighter or darker
//! per region, the cells of the links stronger), and the edges of the
//! cells: faint inside a region, firmer around a link, bold between two.
// ----------------------------------------------------------------------------
static Rgb regionColour(Land const& p_land,
                        CellHit const& p_hit,
                        Rgb p_colour,
                        float p_pixel)
{
    const auto cell = std::size_t(p_hit.cell);
    const int region = p_land.regions_of[cell];
    const bool link = (p_land.parts_of[cell] >= 0);
    if (region >= 0)
    {
        const Rgb hue =
            colours::HUES[p_land.regions[std::size_t(region)].island %
                          colours::HUES.size()];
        const float shade = 0.75f + (0.12f * float(region % 4));
        p_colour = mix(p_colour,
                       Rgb{ hue.r * shade, hue.g * shade, hue.b * shade },
                       link ? 0.6f : 0.34f);
    }
    if (p_hit.across < 0)
    {
        return p_colour;
    }
    const auto across = std::size_t(p_hit.across);
    const bool other_region = (p_land.regions_of[across] != region);
    const bool other_part =
        other_region || link || (p_land.parts_of[across] >= 0);
    if ((region < 0) && !other_region)
    {
        return p_colour;
    }
    float half = 1.2f;
    float strength = 0.22f;
    if (other_region)
    {
        half = 5.0f;
        strength = 0.75f;
    }
    else if (other_part)
    {
        half = 2.5f;
        strength = 0.55f;
    }
    return mix(
        p_colour, colours::BORDER, strength * line(p_hit.edge, half, p_pixel));
}

// ----------------------------------------------------------------------------
//! \brief The colour and the alpha of a point of the map.
//! \param[in] p_height The ground there.
//! \param[in] p_slope How fast the ground changes there, per metre.
//! \param[in] p_pixel The size of a pixel, in metres.
// ----------------------------------------------------------------------------
static void paint(Land const& p_land,
                  Vector2f p_at,
                  float p_height,
                  float p_slope,
                  float p_pixel,
                  std::byte* p_out)
{
    Rgb colour;
    float alpha = 1.0f;
    if (p_height <= 0.0f)
    {
        colour = seaColour(-p_height, p_slope, p_pixel);
        alpha = SEA_ALPHA * std::min(1.0f, -p_height / SEABED);
    }
    else
    {
        float river = 1.0e9f;
        float half = 0.0f;
        p_land.nearestRiver(p_at.x, p_at.y, river, half);
        const CellHit hit = hitCell(p_land.sites, p_at);
        colour = regionColour(
            p_land,
            hit,
            groundColour(p_land, std::size_t(hit.cell), p_height, river),
            p_pixel);
        // The river: its banks, then its water, which the shader animates.
        // A river stays a line from afar: never fainter than 60 %.
        const float bank = line(river, half + std::max(2.0f, p_pixel), p_pixel);
        const float water = std::min(
            1.0f,
            line(river, std::max(half, 0.5f * p_pixel), p_pixel) *
                std::max(0.6f, std::min(1.0f, half / (0.5f * p_pixel))));
        colour =
            mix(mix(colour, colours::BANK, 0.6f * bank), colours::RIVER, water);
        if (water > 0.5f)
        {
            alpha = RIVER_ALPHA;
        }
    }
    auto byte = [](float p_v)
    { return std::byte(std::clamp(int((p_v * 255.0f) + 0.5f), 0, 255)); };
    p_out[0] = byte(colour.r);
    p_out[1] = byte(colour.g);
    p_out[2] = byte(colour.b);
    p_out[3] = byte(alpha);
}

// ----------------------------------------------------------------------------
float Patch::heightAt(float p_x, float p_z) const
{
    const float step = size / float(cells);
    const float gx = std::clamp((p_x - x) / step, -1.0f, float(cells) + 0.999f);
    const float gz = std::clamp((p_z - z) / step, -1.0f, float(cells) + 0.999f);
    const auto i = int(std::floor(gx));
    const auto j = int(std::floor(gz));
    const float tx = gx - float(i);
    const float tz = gz - float(j);
    return (height(i, j) * (1.0f - tx) * (1.0f - tz)) +
           (height(i + 1, j) * tx * (1.0f - tz)) +
           (height(i, j + 1) * (1.0f - tx) * tz) +
           (height(i + 1, j + 1) * tx * tz);
}

// ----------------------------------------------------------------------------
Patch makePatch(Land const& p_land,
                float p_x,
                float p_z,
                float p_size,
                int p_cells,
                int p_pixels,
                Progress const& p_progress)
{
    auto tell = [&p_progress](float p_done, char const* p_what)
    {
        if (p_progress)
        {
            p_progress(p_done, p_what);
        }
    };
    Patch patch{ .x = p_x,
                 .z = p_z,
                 .size = p_size,
                 .cells = p_cells,
                 .heights = {},
                 .pixels = p_pixels,
                 .picture = {} };
    const float step = p_size / float(p_cells);
    const int side = p_cells + 3;
    patch.heights.resize(std::size_t(side) * std::size_t(side));
    for (int j = -1; j <= p_cells + 1; ++j)
    {
        for (int i = -1; i <= p_cells + 1; ++i)
        {
            patch.heights[std::size_t(((j + 1) * side) + i + 1)] =
                p_land.heightAt(p_x + (float(i) * step),
                                p_z + (float(j) * step));
        }
        tell(0.3f * float(j + 1) / float(p_cells + 2), "Relief");
    }
    // The picture reads the heights between the corners: the mesh shows no
    // more.
    const float pixel = p_size / float(p_pixels);
    patch.picture.resize(std::size_t(p_pixels) * std::size_t(p_pixels) * 4u);
    for (int t = 0; t < p_pixels; ++t)
    {
        for (int s = 0; s < p_pixels; ++s)
        {
            const Vector2f at(p_x + ((float(s) + 0.5f) * pixel),
                              p_z + ((float(t) + 0.5f) * pixel));
            const float h = patch.heightAt(at.x, at.y);
            const float slope =
                (std::abs(patch.heightAt(at.x + pixel, at.y) - h) +
                 std::abs(patch.heightAt(at.x, at.y + pixel) - h)) /
                pixel;
            paint(p_land,
                  at,
                  h,
                  slope,
                  pixel,
                  &patch.picture[((std::size_t(t) * std::size_t(p_pixels)) +
                                  std::size_t(s)) *
                                 4u]);
        }
        if ((t % 16) == 0)
        {
            tell(0.3f + (0.7f * float(t) / float(p_pixels)), "Picture");
        }
    }
    tell(1.0f, "Picture");
    return patch;
}

} // namespace examples::archipelago
