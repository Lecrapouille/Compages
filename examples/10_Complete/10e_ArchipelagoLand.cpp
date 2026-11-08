// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/10e_ArchipelagoLand.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <queue>
#include <random>

namespace examples::archipelago
{

namespace
{

using compages::Vector2f;
using compages::Vector3f;

// --- The geology --------------------------------------------------------------
//
// The cells do not get a height by hand: it grows. The plate rises under the
// islands (tectonic uplift U, in metres a year) and the rivers cut it down.
// The stream power law says how fast a river erodes its bed: faster where it
// drains more land (area A) and where it is steeper (slope S),
//
//     dh/dt = U - K A^m S^n,      m = 1/2, n = 1,
//
// plus a slow creep of the slopes (hillslope diffusion). With n = 1, a step
// is solved exactly from the sea upward (Braun and Willett 2013): a cell
// only needs the new height of the cell it drains to. The pits are filled
// first (priority flood: a lake overflows at its lowest pass), so that the
// water of every cell reaches the sea. Cordonnier et al. (2016) run this on
// the graph of a Voronoi diagram, as here.

//! \brief Uplift at the heart of an island (1 mm a year).
constexpr float UPLIFT = 1.0e-3f;
//! \brief Erodibility K (per year, for m = 1/2) and hillslope creep D (m2 a
//! year).
constexpr float ERODIBILITY = 2.0e-5f;
constexpr float CREEP = 0.05f;
//! \brief One step: 40 000 years.
constexpr float YEARS = 4.0e4f;

//! \brief The main river goes up while it drains at least this (five
//! cells); a tributary while it drains two, at most TRIBUTARY_CELLS long. Its width follows
//! the square root of its flow (the hydraulic geometry of Leopold and
//! Maddock), so of the area it drains.
constexpr float STRAND_AREA = 5.0f * CELL_SIZE * CELL_SIZE;
constexpr std::size_t TRIBUTARY_CELLS = 12u;
constexpr float RIVER_WIDTH = 5.0f;
constexpr float RIVER_GROWTH = 8.0f;   // metres more per sqrt(km2)
//! \brief A river meanders with a wavelength of about eleven times its
//! width (Leopold and Wolman), about its width to each side.
constexpr float MEANDER_WAVELENGTH = 11.0f;
constexpr float MEANDER_SWING = 0.9f;

//! \brief The noise that bends the coasts.
constexpr float COAST_BEND = 1100.0f;
//! \brief How far the sites move from the centres of their grid squares, in
//! squares: under 0.3, the nearest site of a point is always among the 3 x 3
//! squares around it.
constexpr float JITTER = 0.3f;

// --- The colours of the map (sRGB) -------------------------------------------

struct Rgb
{
    float r;
    float g;
    float b;
};

constexpr Rgb DEEP{ 0.07f, 0.24f, 0.42f };
constexpr Rgb SHALLOW{ 0.16f, 0.52f, 0.68f };
constexpr Rgb LAGOON{ 0.42f, 0.80f, 0.78f };
constexpr Rgb FOAM{ 0.92f, 0.97f, 0.96f };
constexpr Rgb SAND{ 0.90f, 0.84f, 0.64f };
constexpr Rgb GRASS{ 0.58f, 0.75f, 0.40f };
constexpr Rgb FOREST{ 0.32f, 0.53f, 0.29f };
constexpr Rgb UPLAND{ 0.62f, 0.60f, 0.44f };
constexpr Rgb ROCK{ 0.60f, 0.56f, 0.50f };
constexpr Rgb SUMMIT{ 0.80f, 0.78f, 0.74f };
constexpr Rgb BORDER{ 1.00f, 0.96f, 0.86f };
constexpr Rgb RIVER{ 0.25f, 0.60f, 0.85f };
constexpr Rgb BANK{ 0.30f, 0.42f, 0.36f };
//! \brief The hue of the regions of each island (they cycle).
constexpr std::array<Rgb, 5> HUES = { Rgb{ 0.45f, 0.55f, 0.95f }, Rgb{ 0.95f, 0.45f, 0.60f },
                                      Rgb{ 0.95f, 0.70f, 0.35f }, Rgb{ 0.30f, 0.80f, 0.80f },
                                      Rgb{ 0.60f, 0.90f, 0.40f } };

Rgb mix(Rgb p_a, Rgb p_b, float p_t)
{
    return { p_a.r + ((p_b.r - p_a.r) * p_t), p_a.g + ((p_b.g - p_a.g) * p_t),
             p_a.b + ((p_b.b - p_a.b) * p_t) };
}

float smoothstep(float p_from, float p_to, float p_x)
{
    const float t = std::clamp((p_x - p_from) / (p_to - p_from), 0.0f, 1.0f);
    return t * t * (3.0f - (2.0f * t));
}

// --- Noise ------------------------------------------------------------------

//! \brief A number from 0 to 1 for a corner of an integer lattice.
float lattice(int p_x, int p_y, std::uint32_t p_seed)
{
    std::uint32_t h = (std::uint32_t(p_x) * 0x8da6b343u) ^ (std::uint32_t(p_y) * 0xd8163841u) ^
                      (p_seed * 0xcb1ab31fu);
    h ^= h >> 13u;
    h *= 0x5bd1e995u;
    h ^= h >> 15u;
    return float(h & 0xffffffu) / float(0xffffffu);
}

//! \brief Value noise: the lattice, smoothly interpolated. From 0 to 1.
float noise(float p_x, float p_y, std::uint32_t p_seed)
{
    const float fx = std::floor(p_x);
    const float fy = std::floor(p_y);
    const int x = int(fx);
    const int y = int(fy);
    const float tx = p_x - fx;
    const float ty = p_y - fy;
    const float sx = tx * tx * tx * ((tx * ((tx * 6.0f) - 15.0f)) + 10.0f);
    const float sy = ty * ty * ty * ((ty * ((ty * 6.0f) - 15.0f)) + 10.0f);
    const float a = lattice(x, y, p_seed);
    const float b = lattice(x + 1, y, p_seed);
    const float c = lattice(x, y + 1, p_seed);
    const float d = lattice(x + 1, y + 1, p_seed);
    return a + ((b - a) * sx) + ((c - a) * sy) + ((a - b - c + d) * sx * sy);
}

//! \brief Octaves of noise, each twice as fine and half as strong. 0 to 1.
float fbm(float p_x, float p_y, std::uint32_t p_seed, int p_octaves)
{
    float sum = 0.0f;
    float amplitude = 0.5f;
    float total = 0.0f;
    for (int i = 0; i < p_octaves; ++i)
    {
        sum += amplitude * noise(p_x, p_y, p_seed + std::uint32_t(i));
        total += amplitude;
        p_x *= 2.03f;
        p_y *= 2.03f;
        amplitude *= 0.5f;
    }
    return sum / total;
}

// --- The cells ----------------------------------------------------------------

//! \brief The cell of grid square (i, j).
int cellAt(int p_i, int p_j)
{
    return (p_j * CELLS) + p_i;
}

//! \brief The grid square a coordinate falls in.
int squareOf(float p_coordinate)
{
    return std::clamp(int(std::floor((p_coordinate + (0.5f * MAP)) / CELL_SIZE)), 0, CELLS - 1);
}

//! \brief The distance from a point to the bisector of the sites of two
//! cells, positive on the side of the first.
float bisector(Vector2f p_own, Vector2f p_other, Vector2f p_point)
{
    const Vector2f across = p_other - p_own;
    const Vector2f middle = (p_own + p_other) * 0.5f;
    return (((middle.x - p_point.x) * across.x) + ((middle.y - p_point.y) * across.y)) /
           std::sqrt((across.x * across.x) + (across.y * across.y));
}

//! \brief The cell a point is in (the nearest site), and the nearest border
//! of that cell: the cell across it and how far it is.
struct CellHit
{
    int cell = 0;
    int across = -1;
    float edge = 1.0e9f;
};

CellHit hitCell(std::vector<Vector2f> const& p_sites, Vector2f p_point)
{
    const int gi = squareOf(p_point.x);
    const int gj = squareOf(p_point.y);
    CellHit hit;
    float best = 1.0e30f;
    for (int j = std::max(0, gj - 1); j <= std::min(CELLS - 1, gj + 1); ++j)
    {
        for (int i = std::max(0, gi - 1); i <= std::min(CELLS - 1, gi + 1); ++i)
        {
            const Vector2f d = p_point - p_sites[std::size_t(cellAt(i, j))];
            const float d2 = (d.x * d.x) + (d.y * d.y);
            if (d2 < best)
            {
                best = d2;
                hit.cell = cellAt(i, j);
            }
        }
    }
    const Vector2f own = p_sites[std::size_t(hit.cell)];
    const int ci = hit.cell % CELLS;
    const int cj = hit.cell / CELLS;
    for (int j = std::max(0, cj - 1); j <= std::min(CELLS - 1, cj + 1); ++j)
    {
        for (int i = std::max(0, ci - 1); i <= std::min(CELLS - 1, ci + 1); ++i)
        {
            const int other = cellAt(i, j);
            if (other == hit.cell)
            {
                continue;
            }
            const float edge = bisector(own, p_sites[std::size_t(other)], p_point);
            if (edge < hit.edge)
            {
                hit.edge = edge;
                hit.across = other;
            }
        }
    }
    return hit;
}

//! \brief The heights of the cells, read smoothly between their sites (a
//! Gaussian of the distance, over the 3 x 3 cells around).
float cellHeight(Land const& p_land, float p_x, float p_z)
{
    const int gi = squareOf(p_x);
    const int gj = squareOf(p_z);
    float sum = 0.0f;
    float weights = 0.0f;
    constexpr float SPREAD = 1.0f / (0.62f * CELL_SIZE * 0.62f * CELL_SIZE);
    for (int j = std::max(0, gj - 1); j <= std::min(CELLS - 1, gj + 1); ++j)
    {
        for (int i = std::max(0, gi - 1); i <= std::min(CELLS - 1, gi + 1); ++i)
        {
            const std::size_t c = std::size_t(cellAt(i, j));
            const Vector2f d = Vector2f(p_x, p_z) - p_land.sites[c];
            const float w = std::exp(-((d.x * d.x) + (d.y * d.y)) * SPREAD);
            sum += w * p_land.elevations[c];
            weights += w;
        }
    }
    return sum / std::max(weights, 1.0e-12f);
}

//! \brief The distance from the border of a group of cells, positive inside:
//! where the outline of the group is zero.
template <typename Member>
float groupField(std::vector<Vector2f> const& p_sites, Vector2f p_point, Member&& p_member)
{
    const CellHit hit = hitCell(p_sites, p_point);
    const bool inside = p_member(hit.cell);
    const Vector2f own = p_sites[std::size_t(hit.cell)];
    float nearest = CELL_SIZE;
    const int ci = hit.cell % CELLS;
    const int cj = hit.cell / CELLS;
    for (int j = std::max(0, cj - 1); j <= std::min(CELLS - 1, cj + 1); ++j)
    {
        for (int i = std::max(0, ci - 1); i <= std::min(CELLS - 1, ci + 1); ++i)
        {
            const int other = cellAt(i, j);
            if ((other != hit.cell) && (p_member(other) != inside))
            {
                nearest = std::min(nearest, bisector(own, p_sites[std::size_t(other)], p_point));
            }
        }
    }
    return inside ? nearest : -nearest;
}

// --- The drainage of the cells -----------------------------------------------

//! \brief How the cells drain: each one to its receiver; the order goes
//! from the sea upward; the area each one drains (m2).
struct Drainage
{
    std::vector<int> receivers;
    std::vector<int> order;
    std::vector<float> areas;
};

//! \brief The drainage of the cells by priority flood: the sea is reached
//! first, then always the lowest cell not yet reached, from the neighbour
//! it was reached from, which becomes its receiver.
Drainage drain(std::vector<float> const& p_heights, std::vector<char> const& p_outlet)
{
    const std::size_t count = p_heights.size();
    Drainage result;
    result.receivers.assign(count, -1);
    result.order.reserve(count);
    std::vector<float> level(p_heights);
    std::vector<char> reached(count, 0);
    using Entry = std::pair<float, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> open;
    for (std::size_t c = 0u; c < count; ++c)
    {
        if (p_outlet[c] != 0)
        {
            reached[c] = 1;
            open.push({ p_heights[c], int(c) });
        }
    }
    while (!open.empty())
    {
        const int c = open.top().second;
        open.pop();
        result.order.push_back(c);
        const int ci = c % CELLS;
        const int cj = c / CELLS;
        for (int j = std::max(0, cj - 1); j <= std::min(CELLS - 1, cj + 1); ++j)
        {
            for (int i = std::max(0, ci - 1); i <= std::min(CELLS - 1, ci + 1); ++i)
            {
                const int n = cellAt(i, j);
                if (reached[std::size_t(n)] != 0)
                {
                    continue;
                }
                reached[std::size_t(n)] = 1;
                level[std::size_t(n)] = std::max(p_heights[std::size_t(n)], level[std::size_t(c)]);
                result.receivers[std::size_t(n)] = c;
                open.push({ level[std::size_t(n)], n });
            }
        }
    }
    result.areas.assign(count, CELL_SIZE * CELL_SIZE);
    for (auto it = result.order.rbegin(); it != result.order.rend(); ++it)
    {
        const int receiver = result.receivers[std::size_t(*it)];
        if (receiver >= 0)
        {
            result.areas[std::size_t(receiver)] += result.areas[std::size_t(*it)];
        }
    }
    return result;
}

// ----------------------------------------------------------------------------
//! \brief Grow the relief of the cells: uplift, river erosion, creep.
// ----------------------------------------------------------------------------
Drainage erode(std::vector<Vector2f> const& p_sites,
               std::vector<float>& p_heights,
               std::vector<float> const& p_uplift,
               std::vector<char> const& p_outlet,
               int p_steps,
               Progress const& p_progress)
{
    std::vector<float> creep(p_heights.size(), 0.0f);
    for (int step = 0; step < p_steps; ++step)
    {
        const Drainage drainage = drain(p_heights, p_outlet);
        // From the sea upward: the receiver already has its new height,
        //   h = (h + U dt + F h_receiver) / (1 + F),  F = K dt sqrt(A) / distance.
        for (const int c : drainage.order)
        {
            const int receiver = drainage.receivers[std::size_t(c)];
            if ((receiver < 0) || (p_outlet[std::size_t(c)] != 0))
            {
                continue;
            }
            const Vector2f d = p_sites[std::size_t(c)] - p_sites[std::size_t(receiver)];
            const float distance = std::sqrt((d.x * d.x) + (d.y * d.y));
            const float factor = ERODIBILITY * YEARS * std::sqrt(drainage.areas[std::size_t(c)]) / distance;
            const float raised = p_heights[std::size_t(c)] + (p_uplift[std::size_t(c)] * YEARS);
            const float bed = p_heights[std::size_t(receiver)];
            // A lake (the receiver is higher) is not eroded: it fills.
            p_heights[std::size_t(c)] = (bed >= raised) ? raised : ((raised + (factor * bed)) / (1.0f + factor));
        }
        // The slopes creep: D dt times the Laplacian of the 4 neighbours.
        const float rate = CREEP * YEARS / (CELL_SIZE * CELL_SIZE);
        for (int j = 1; j < CELLS - 1; ++j)
        {
            for (int i = 1; i < CELLS - 1; ++i)
            {
                const std::size_t c = std::size_t(cellAt(i, j));
                creep[c] = (p_outlet[c] != 0)
                               ? 0.0f
                               : rate * (p_heights[c - 1u] + p_heights[c + 1u] + p_heights[c - std::size_t(CELLS)] +
                                         p_heights[c + std::size_t(CELLS)] - (4.0f * p_heights[c]));
            }
        }
        for (std::size_t c = 0u; c < p_heights.size(); ++c)
        {
            p_heights[c] += creep[c];
        }
        p_progress(float(step + 1) / float(std::max(p_steps, 1)), "Uplift and erosion");
    }
    return drain(p_heights, p_outlet);
}

// --- Rivers -----------------------------------------------------------------

//! \brief A Catmull-Rom spline through the points: it passes through each
//! one (two rivers still meet at a cell), smooth between them.
std::vector<Vector2f> spline(std::vector<Vector2f> const& p_points, std::vector<float> const& p_widths,
                             std::vector<float>& p_out_widths)
{
    std::vector<Vector2f> curve;
    constexpr int STEPS = 6;
    const std::size_t last = p_points.size() - 1u;
    for (std::size_t k = 0u; k < last; ++k)
    {
        const Vector2f p0 = p_points[(k == 0u) ? 0u : (k - 1u)];
        const Vector2f p1 = p_points[k];
        const Vector2f p2 = p_points[k + 1u];
        const Vector2f p3 = p_points[std::min(k + 2u, last)];
        for (int s = 0; s < STEPS; ++s)
        {
            const float t = float(s) / float(STEPS);
            const float t2 = t * t;
            const float t3 = t2 * t;
            curve.push_back(((p1 * 2.0f) + ((p2 - p0) * t) + (((p0 * 2.0f) - (p1 * 5.0f) + (p2 * 4.0f) - p3) * t2) +
                             (((p1 * 3.0f) - p0 - (p2 * 3.0f) + p3) * t3)) *
                            0.5f);
            p_out_widths.push_back(p_widths[k] + ((p_widths[k + 1u] - p_widths[k]) * t));
        }
    }
    curve.push_back(p_points.back());
    p_out_widths.push_back(p_widths.back());
    return curve;
}

// ----------------------------------------------------------------------------
//! \brief A river that swings to each side of its path: points every few
//! metres, pushed across the path by a sine whose wavelength follows the
//! width. No two bends alike: the wavelength and the swing vary with the
//! noise, and a second harmonic makes the loops lean. The swing fades at
//! both ends, so that a river still meets the one it joins.
// ----------------------------------------------------------------------------
River meander(std::vector<Vector2f> const& p_points, std::vector<float> const& p_widths, std::uint32_t p_seed)
{
    std::vector<float> along(p_points.size(), 0.0f);
    for (std::size_t k = 1u; k < p_points.size(); ++k)
    {
        const Vector2f d = p_points[k] - p_points[k - 1u];
        along[k] = along[k - 1u] + std::sqrt((d.x * d.x) + (d.y * d.y));
    }
    const float total = along.back();
    River result;
    float phase = 0.0f;
    std::size_t k = 0u;
    constexpr float STEP = 6.0f;
    for (float s = 0.0f; s <= total; s += STEP)
    {
        while (((k + 2u) < along.size()) && (along[k + 1u] < s))
        {
            ++k;
        }
        const float span = std::max(along[k + 1u] - along[k], 1.0e-3f);
        const float t = std::clamp((s - along[k]) / span, 0.0f, 1.0f);
        const Vector2f a = p_points[k];
        const Vector2f b = p_points[k + 1u];
        const float width = p_widths[k] + ((p_widths[k + 1u] - p_widths[k]) * t);
        const Vector2f tangent = (b - a) * (1.0f / span);
        const Vector2f across(-tangent.y, tangent.x);
        const Vector2f at = a + ((b - a) * t);
        const float stretch = 0.7f + (0.8f * noise(at.x / 300.0f, at.y / 300.0f, p_seed + 93u));
        phase += 6.2831853f * STEP / (MEANDER_WAVELENGTH * width * stretch);
        const float ends = std::min(1.0f, std::min(s, total - s) / (2.0f * MEANDER_WAVELENGTH * width));
        const float swing = MEANDER_SWING * width * ends *
                            (0.25f + (1.75f * noise(at.x / 500.0f, at.y / 500.0f, p_seed + 91u)));
        const float bend = std::sin(phase) + (0.3f * std::sin((2.0f * phase) + 0.8f));
        result.points.push_back(at + (across * (swing * bend)));
        result.widths.push_back(width);
    }
    result.points.push_back(p_points.back());
    result.widths.push_back(p_widths.back());
    return result;
}

//! \brief The 64 x 64 squares that index the river segments.
constexpr int BUCKETS = 64;
constexpr float BUCKET_SIZE = MAP / float(BUCKETS);

//! \brief How far from its middle a river digs: its bed, then its valley.
float valleyReach(float p_half)
{
    return (6.0f * p_half) + 90.0f;
}

// --- Painting ---------------------------------------------------------------

//! \brief A line of the map, \p p_half wide on each side, at \p p_distance:
//! its coverage of a pixel. A line thinner than a pixel is fainter, not
//! wider: the far view keeps its lines light.
float line(float p_distance, float p_half, float p_pixel)
{
    const float shown = std::max(p_half, 0.5f * p_pixel);
    return std::clamp(((shown - p_distance) / p_pixel) + 0.5f, 0.0f, 1.0f) * std::min(1.0f, p_half / shown);
}

//! \brief The colour and the alpha of a point of the map.
//! \param[in] p_height The ground there.
//! \param[in] p_slope How fast the ground changes there, per metre.
//! \param[in] p_pixel The size of a pixel, in metres.
void paint(Land const& p_land, float p_x, float p_z, float p_height, float p_slope, float p_pixel, std::byte* p_out)
{
    Rgb colour;
    float alpha = 1.0f;
    float river_distance = 1.0e9f;
    float river_half = 0.0f;
    p_land.nearestRiver(p_x, p_z, river_distance, river_half);
    if (p_height <= 0.0f)
    {
        // The sea bed seen through the water: lagoon, shallows, deep; two
        // lines of equal depth, as on a nautical chart.
        const float depth = -p_height;
        colour = mix(LAGOON, SHALLOW, smoothstep(0.0f, 18.0f, depth));
        colour = mix(colour, DEEP, smoothstep(15.0f, 85.0f, depth));
        for (const float level : { 6.0f, 30.0f })
        {
            const float lines = line(std::abs(depth - level) / std::max(p_slope, 1.0e-3f), 1.5f, p_pixel);
            colour = mix(colour, FOAM, 0.25f * lines);
        }
        alpha = SEA_ALPHA * std::min(1.0f, depth / SEABED);
    }
    else
    {
        const CellHit hit = hitCell(p_land.sites, Vector2f(p_x, p_z));
        const std::size_t cell = std::size_t(hit.cell);
        // Each cell has its own wetness (a biome, after Patel); the banks
        // of the rivers are wetter.
        const Vector2f site = p_land.sites[cell];
        const float wet = noise(site.x / 900.0f, site.y / 900.0f, p_land.seed + 41u) +
                          (0.35f * (1.0f - smoothstep(0.0f, 250.0f, river_distance)));
        colour = mix(GRASS, FOREST, smoothstep(0.45f, 0.7f, wet));
        colour = mix(SAND, colour, smoothstep(2.0f, 7.0f, p_height));
        colour = mix(colour, UPLAND, smoothstep(110.0f, 250.0f, p_height));
        colour = mix(colour, ROCK, smoothstep(250.0f, 400.0f, p_height));
        colour = mix(colour, SUMMIT, smoothstep(440.0f, 540.0f, p_height));
        // A mosaic: each cell a touch lighter or darker.
        const float tone = 0.95f + (0.1f * lattice(int(cell), 7, p_land.seed));
        colour = { colour.r * tone, colour.g * tone, colour.b * tone };

        const int region = p_land.regions_of[cell];
        const bool link = (p_land.parts_of[cell] >= 0);
        if (region >= 0)
        {
            // The hue of the island, lighter or darker per region; the cells
            // of the links stronger.
            Region const& owner = p_land.regions[std::size_t(region)];
            const Rgb hue = HUES[owner.island % HUES.size()];
            const float shade = 0.75f + (0.12f * float(region % 4));
            colour = mix(colour, Rgb{ hue.r * shade, hue.g * shade, hue.b * shade }, link ? 0.6f : 0.34f);
        }
        // The edges of the cells: faint inside a region, firmer around a
        // link, bold between two regions.
        if (hit.across >= 0)
        {
            const std::size_t across = std::size_t(hit.across);
            const bool other_region = (p_land.regions_of[across] != region);
            const bool other_part = other_region || link || (p_land.parts_of[across] >= 0);
            const float half = other_region ? 5.0f : (other_part ? 2.5f : 1.2f);
            const float strength = other_region ? 0.75f : (other_part ? 0.55f : 0.22f);
            if ((region >= 0) || other_region)
            {
                colour = mix(colour, BORDER, strength * line(hit.edge, half, p_pixel));
            }
        }
        // The river: its banks, then its water, which the shader animates.
        // A river stays a line from afar: never fainter than 60 %.
        const float bank = line(river_distance, river_half + std::max(2.0f, p_pixel), p_pixel);
        const float water = std::min(1.0f, line(river_distance, std::max(river_half, 0.5f * p_pixel), p_pixel) *
                                               std::max(0.6f, std::min(1.0f, river_half / (0.5f * p_pixel))));
        colour = mix(colour, BANK, 0.6f * bank);
        colour = mix(colour, RIVER, water);
        if (water > 0.5f)
        {
            alpha = RIVER_ALPHA;
        }
    }
    p_out[0] = std::byte(std::clamp(int((colour.r * 255.0f) + 0.5f), 0, 255));
    p_out[1] = std::byte(std::clamp(int((colour.g * 255.0f) + 0.5f), 0, 255));
    p_out[2] = std::byte(std::clamp(int((colour.b * 255.0f) + 0.5f), 0, 255));
    p_out[3] = std::byte(std::clamp(int((alpha * 255.0f) + 0.5f), 0, 255));
}

// --- Outlines ---------------------------------------------------------------

//! \brief The grid the outlines are traced on: 31 m.
constexpr int OUTLINE_GRID = 512;

// ----------------------------------------------------------------------------
//! \brief The outline of a field where it is zero (marching squares): a
//! corner is inside where the field is positive.
//! \param[in] p_box The corners to look at: i0, j0, i1, j1.
//! \return Two points per segment, a few metres over the ground.
// ----------------------------------------------------------------------------
std::vector<Vector3f> outline(Land const& p_land, std::vector<float> const& p_field, std::array<int, 4> const& p_box)
{
    std::vector<Vector3f> segments;
    constexpr float LIFT = 4.0f;
    constexpr float STEP = MAP / float(OUTLINE_GRID);
    auto at = [&](int p_i, int p_j) { return std::size_t((p_j * (OUTLINE_GRID + 1)) + p_i); };
    auto position = [&](int p_i, int p_j)
    { return Vector2f((-0.5f * MAP) + (float(p_i) * STEP), (-0.5f * MAP) + (float(p_j) * STEP)); };
    for (int j = p_box[1]; j < p_box[3]; ++j)
    {
        for (int i = p_box[0]; i < p_box[2]; ++i)
        {
            const std::array<std::array<int, 2>, 4> corners = { { { i, j }, { i + 1, j }, { i + 1, j + 1 }, { i, j + 1 } } };
            std::array<Vector3f, 4> crossings;
            std::size_t found = 0u;
            for (std::size_t e = 0u; e < 4u; ++e)
            {
                const std::array<int, 2> a = corners[e];
                const std::array<int, 2> b = corners[(e + 1u) % 4u];
                const float fa = p_field[at(a[0], a[1])];
                const float fb = p_field[at(b[0], b[1])];
                if ((fa > 0.0f) == (fb > 0.0f))
                {
                    continue;
                }
                const float t = fa / (fa - fb);
                const Vector2f p = position(a[0], a[1]) + ((position(b[0], b[1]) - position(a[0], a[1])) * t);
                // On the ground as the meshes show it, valleys of the rivers
                // included.
                crossings[found++] = Vector3f(p.x, std::max(0.0f, p_land.heightAt(p.x, p.y)) + LIFT, p.y);
            }
            for (std::size_t k = 0u; (k + 1u) < found; k += 2u)
            {
                segments.push_back(crossings[k]);
                segments.push_back(crossings[k + 1u]);
            }
        }
    }
    return segments;
}

} // namespace

// ----------------------------------------------------------------------------
void Land::nearestRiver(float p_x, float p_z, float& p_distance, float& p_half) const
{
    p_distance = 1.0e9f;
    p_half = 0.0f;
    const int i = std::clamp(int((p_x + (0.5f * MAP)) / BUCKET_SIZE), 0, BUCKETS - 1);
    const int j = std::clamp(int((p_z + (0.5f * MAP)) / BUCKET_SIZE), 0, BUCKETS - 1);
    for (const int index : buckets[std::size_t((j * BUCKETS) + i)])
    {
        Segment const& segment = segments[std::size_t(index)];
        const Vector2f ab = segment.b - segment.a;
        const float length2 = std::max((ab.x * ab.x) + (ab.y * ab.y), 1.0e-6f);
        const float t = std::clamp((((p_x - segment.a.x) * ab.x) + ((p_z - segment.a.y) * ab.y)) / length2, 0.0f, 1.0f);
        const Vector2f d = Vector2f(p_x, p_z) - (segment.a + (ab * t));
        const float distance = std::sqrt((d.x * d.x) + (d.y * d.y));
        if (distance < p_distance)
        {
            p_distance = distance;
            p_half = 0.5f * (segment.width_a + ((segment.width_b - segment.width_a) * t));
        }
    }
}

// ----------------------------------------------------------------------------
float Land::heightAt(float p_x, float p_z) const
{
    const float smooth = cellHeight(*this, p_x, p_z);
    // A little roughness on the slopes, none on the beaches.
    float ground = smooth + (7.0f * (fbm(p_x / 140.0f, p_z / 140.0f, seed + 81u, 2) - 0.5f) *
                             smoothstep(8.0f, 60.0f, smooth));
    // The river digs its valley, then its bed: the water runs below its
    // banks, the slopes lean toward it.
    float distance = 0.0f;
    float half = 0.0f;
    nearestRiver(p_x, p_z, distance, half);
    const float reach = valleyReach(half);
    if (distance < reach)
    {
        // A valley that narrows to the river (a smooth V), as deep as the
        // river is large; then a bed with banks, a few metres below.
        const float valley = 1.0f - (distance / reach);
        ground -= (6.0f + (0.6f * half)) * valley * valley * (3.0f - (2.0f * valley));
        const float banks = half + 4.0f;
        if (distance < banks)
        {
            const float bed = distance / banks;
            ground -= (2.0f + (0.2f * half)) * (1.0f - (bed * bed * bed * bed));
        }
    }
    return ground;
}

// ----------------------------------------------------------------------------
Place Land::placeOf(float p_x, float p_z) const
{
    const int cell = hitCell(sites, Vector2f(p_x, p_z)).cell;
    return { cell, regions_of[std::size_t(cell)], parts_of[std::size_t(cell)] };
}

// ----------------------------------------------------------------------------
std::vector<Vector2f> Land::cellPolygon(int p_cell) const
{
    // A square around the site, cut by the bisector with each site around
    // (Sutherland-Hodgman): what is left is nearer the site than any other.
    const Vector2f own = sites[std::size_t(p_cell)];
    const float reach = 2.0f * CELL_SIZE;
    std::vector<Vector2f> polygon = { own + Vector2f(-reach, -reach), own + Vector2f(reach, -reach),
                                      own + Vector2f(reach, reach), own + Vector2f(-reach, reach) };
    const int ci = p_cell % CELLS;
    const int cj = p_cell / CELLS;
    for (int j = std::max(0, cj - 2); j <= std::min(CELLS - 1, cj + 2); ++j)
    {
        for (int i = std::max(0, ci - 2); i <= std::min(CELLS - 1, ci + 2); ++i)
        {
            const int other = cellAt(i, j);
            if (other == p_cell)
            {
                continue;
            }
            const Vector2f site = sites[std::size_t(other)];
            std::vector<Vector2f> kept;
            for (std::size_t k = 0u; k < polygon.size(); ++k)
            {
                const Vector2f a = polygon[k];
                const Vector2f b = polygon[(k + 1u) % polygon.size()];
                const float da = bisector(own, site, a);
                const float db = bisector(own, site, b);
                if (da >= 0.0f)
                {
                    kept.push_back(a);
                }
                if ((da >= 0.0f) != (db >= 0.0f))
                {
                    kept.push_back(a + ((b - a) * (da / (da - db))));
                }
            }
            polygon = std::move(kept);
        }
    }
    return polygon;
}

// ----------------------------------------------------------------------------
Land makeLand(Recipe const& p_recipe, Progress const& p_progress)
{
    Land land;
    land.seed = p_recipe.seed;
    const std::uint32_t seed = p_recipe.seed;
    std::mt19937 random(seed);
    std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
    const std::size_t count = std::size_t(CELLS) * std::size_t(CELLS);
    // The share of the work each stage takes, for the progress bar.
    auto stage = [&](float p_from, float p_to, char const* p_what)
    { return [=, &p_progress](float p_done, char const*) { p_progress(p_from + ((p_to - p_from) * p_done), p_what); }; };

    // --- The cells: one site per grid square, moved a little --------------
    p_progress(0.0f, "Voronoi cells");
    land.sites.resize(count);
    for (int j = 0; j < CELLS; ++j)
    {
        for (int i = 0; i < CELLS; ++i)
        {
            land.sites[std::size_t(cellAt(i, j))] =
                Vector2f((-0.5f * MAP) + ((float(i) + 0.5f + (JITTER * unit(random))) * CELL_SIZE),
                         (-0.5f * MAP) + ((float(j) + 0.5f + (JITTER * unit(random))) * CELL_SIZE));
        }
    }

    // --- The islands, moved a little by the seed --------------------------
    for (std::size_t k = 0u; k < p_recipe.centres.size(); ++k)
    {
        Island island;
        island.centre = p_recipe.centres[k] + Vector2f(500.0f * unit(random), 500.0f * unit(random));
        island.radius = p_recipe.radii[k] * (1.0f + (0.1f * unit(random)));
        land.islands.push_back(island);
    }

    // --- Land or sea, per cell (Patel): bumps on bent coordinates ----------
    std::vector<float> shape(count, 0.0f);
    land.islands_of.assign(count, std::int8_t(-1));
    for (std::size_t c = 0u; c < count; ++c)
    {
        const Vector2f p = land.sites[c];
        const float wx = COAST_BEND * ((2.0f * fbm(p.x / 2600.0f, p.y / 2600.0f, seed + 11u, 4)) - 1.0f);
        const float wz = COAST_BEND * ((2.0f * fbm(p.x / 2600.0f, p.y / 2600.0f, seed + 12u, 4)) - 1.0f);
        float bump = 0.0f;
        for (std::size_t k = 0u; k < land.islands.size(); ++k)
        {
            const float dx = (p.x + wx) - land.islands[k].centre.x;
            const float dz = (p.y + wz) - land.islands[k].centre.y;
            const float t2 = ((dx * dx) + (dz * dz)) / (land.islands[k].radius * land.islands[k].radius);
            const float f = (t2 < 1.0f) ? ((1.0f - t2) * (1.0f - t2)) : 0.0f;
            if (f > bump)
            {
                bump = f;
                land.islands_of[c] = std::int8_t(k);
            }
        }
        const float edge = (0.5f * MAP) - std::max(std::abs(p.x), std::abs(p.y));
        const float rough = 0.12f + (0.4f * std::min(1.0f, 3.0f * bump));
        float e = bump + (rough * (fbm(p.x / 1500.0f, p.y / 1500.0f, seed, 5) - 0.5f)) - 0.12f;
        e -= 0.6f * std::max(0.0f, 1.0f - (edge / 1200.0f));
        shape[c] = e;
    }
    // The ocean is the water reached from the edge; water closed in by land
    // would be a lake: it is filled (no lakes on this map).
    std::vector<char> sea(count, 0);
    {
        std::vector<int> open;
        for (int k = 0; k < CELLS; ++k)
        {
            for (const int c : { cellAt(k, 0), cellAt(k, CELLS - 1), cellAt(0, k), cellAt(CELLS - 1, k) })
            {
                if ((sea[std::size_t(c)] == 0) && (shape[std::size_t(c)] <= 0.0f))
                {
                    sea[std::size_t(c)] = 1;
                    open.push_back(c);
                }
            }
        }
        while (!open.empty())
        {
            const int c = open.back();
            open.pop_back();
            const int ci = c % CELLS;
            const int cj = c / CELLS;
            for (const std::array<int, 2> d : { std::array<int, 2>{ 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } })
            {
                const int ni = ci + d[0];
                const int nj = cj + d[1];
                if ((ni < 0) || (nj < 0) || (ni >= CELLS) || (nj >= CELLS))
                {
                    continue;
                }
                const int n = cellAt(ni, nj);
                if ((sea[std::size_t(n)] == 0) && (shape[std::size_t(n)] <= 0.0f))
                {
                    sea[std::size_t(n)] = 1;
                    open.push_back(n);
                }
            }
        }
    }

    // --- The relief of the land cells: uplift and erosion -----------------
    std::vector<float> ground(count, 0.0f);
    std::vector<float> uplift(count, 0.0f);
    std::vector<char> outlet(count, 0);
    for (std::size_t c = 0u; c < count; ++c)
    {
        const Vector2f p = land.sites[c];
        const int i = int(c) % CELLS;
        const int j = int(c) / CELLS;
        outlet[c] = ((sea[c] != 0) || (i == 0) || (j == 0) || (i == CELLS - 1) || (j == CELLS - 1)) ? 1 : 0;
        if (outlet[c] == 0)
        {
            const float massif = 0.4f + (1.2f * fbm(p.x / 2200.0f, p.y / 2200.0f, seed + 61u, 3));
            uplift[c] = UPLIFT * massif * (0.25f + (0.75f * smoothstep(0.0f, 0.5f, shape[c])));
            ground[c] = 4.0f * noise(p.x / 400.0f, p.y / 400.0f, seed + 71u);
        }
    }
    const Drainage drainage =
        erode(land.sites, ground, uplift, outlet, p_recipe.erosion_steps, stage(0.0f, 0.55f, "Uplift and erosion"));
    // The shape of the relief is the model's; its height is the map's. A
    // land cell is at least a little above the sea; a sea cell deepens away
    // from the coast.
    const float tallest = std::max(1.0f, *std::max_element(ground.begin(), ground.end()));
    land.elevations.resize(count);
    for (std::size_t c = 0u; c < count; ++c)
    {
        land.elevations[c] = (outlet[c] == 0) ? (3.0f + (ground[c] * ((0.92f * PEAK) / tallest)))
                                              : ((-SEABED * smoothstep(0.0f, 0.14f, -shape[c])) - 6.0f);
        if (outlet[c] != 0)
        {
            land.islands_of[c] = std::int8_t(-1);
        }
    }

    // --- The strands: a main river per island, a tributary per region ----
    // "Noems as loci along a strand": the main river is the highest place of
    // an island; its tributaries are the places of its regions, from the
    // mouth to the source; the cells along a tributary hold the links.
    p_progress(0.6f, "Rivers");
    std::vector<std::vector<int>> island_regions(land.islands.size());
    for (std::size_t r = 0u; r < p_recipe.region_islands.size(); ++r)
    {
        Region region;
        region.island = p_recipe.region_islands[r];
        land.regions.push_back(region);
        island_regions[region.island].push_back(int(r));
    }
    std::vector<std::vector<int>> donors(count);
    for (std::size_t c = 0u; c < count; ++c)
    {
        if ((outlet[c] == 0) && (drainage.receivers[c] >= 0))
        {
            donors[std::size_t(drainage.receivers[c])].push_back(int(c));
        }
    }
    std::vector<char> on_river(count, 0);
    // From a cell up the river: always the donor that drains most, while it
    // drains at least p_least. The path goes upstream.
    auto upstream = [&](int p_from, std::size_t p_longest, float p_least)
    {
        std::vector<int> path{ p_from };
        on_river[std::size_t(p_from)] = 1;
        int c = p_from;
        while (path.size() < p_longest)
        {
            int best = -1;
            float area = p_least;
            for (const int d : donors[std::size_t(c)])
            {
                if ((on_river[std::size_t(d)] == 0) && (drainage.areas[std::size_t(d)] >= area))
                {
                    area = drainage.areas[std::size_t(d)];
                    best = d;
                }
            }
            if (best < 0)
            {
                break;
            }
            path.push_back(best);
            on_river[std::size_t(best)] = 1;
            c = best;
        }
        return path;
    };
    auto widthOf = [&](int p_c)
    { return RIVER_WIDTH + (RIVER_GROWTH * std::sqrt(drainage.areas[std::size_t(p_c)] / 1.0e6f)); };
    // A river from cells given upstream first, to an end point: a spline
    // through the sites, then its meanders.
    auto addRiver = [&](std::vector<int> const& p_upstream, Vector2f p_end)
    {
        std::vector<Vector2f> points;
        std::vector<float> widths;
        for (auto it = p_upstream.rbegin(); it != p_upstream.rend(); ++it)
        {
            points.push_back(land.sites[std::size_t(*it)]);
            widths.push_back(widthOf(*it));
        }
        points.push_back(p_end);
        widths.push_back(widths.back());
        std::vector<float> curve_widths;
        const std::vector<Vector2f> curve = spline(points, widths, curve_widths);
        land.rivers.push_back(meander(curve, curve_widths, seed));
    };
    std::vector<std::vector<int>> paths(land.regions.size());
    for (std::size_t k = 0u; k < land.islands.size(); ++k)
    {
        // The mouth: the coast cell of the island that drains most.
        int mouth = -1;
        float largest = 0.0f;
        for (std::size_t c = 0u; c < count; ++c)
        {
            const int receiver = drainage.receivers[c];
            if ((land.islands_of[c] == std::int8_t(k)) && (outlet[c] == 0) && (receiver >= 0) &&
                (outlet[std::size_t(receiver)] != 0) && (drainage.areas[c] > largest))
            {
                largest = drainage.areas[c];
                mouth = int(c);
            }
        }
        if (mouth < 0)
        {
            continue;
        }
        const std::vector<int> main = upstream(mouth, count, STRAND_AREA);
        // Into the sea: past the coast, between the mouth and the sea cell.
        const Vector2f sea_site = land.sites[std::size_t(drainage.receivers[std::size_t(mouth)])];
        addRiver(main, (land.sites[std::size_t(mouth)] * 0.3f) + (sea_site * 0.7f));

        // The tributaries, spread along the strand: the main river is cut
        // into as many stretches as the island has regions, and each
        // stretch gives the donor that drains most (on either bank).
        struct Root
        {
            int cell;
            std::size_t along;
            float area;
        };
        const std::size_t wanted = island_regions[k].size();
        std::vector<Root> chosen;
        for (std::size_t n = 0u; (n < wanted) && (wanted > 0u); ++n)
        {
            // From the stretch outward, until a donor is found.
            const std::size_t first = (n * main.size()) / wanted;
            const std::size_t last = ((n + 1u) * main.size()) / wanted;
            for (std::size_t widen = 0u; widen <= main.size(); ++widen)
            {
                Root best{ -1, 0u, -1.0f };
                const std::size_t from = (first > widen) ? (first - widen) : 0u;
                const std::size_t to = std::min(main.size(), last + widen);
                for (std::size_t i = from; i < to; ++i)
                {
                    for (const int d : donors[std::size_t(main[i])])
                    {
                        const bool taken = std::any_of(chosen.begin(), chosen.end(), [&](Root const& p_r) { return p_r.cell == d; });
                        if ((on_river[std::size_t(d)] == 0) && !taken && (drainage.areas[std::size_t(d)] > best.area))
                        {
                            best = { d, i, drainage.areas[std::size_t(d)] };
                        }
                    }
                }
                if (best.cell >= 0)
                {
                    chosen.push_back(best);
                    break;
                }
            }
        }
        std::sort(chosen.begin(), chosen.end(), [](Root const& p_a, Root const& p_b) { return p_a.along < p_b.along; });
        for (std::size_t n = 0u; n < chosen.size(); ++n)
        {
            const int r = island_regions[k][n];
            paths[std::size_t(r)] = upstream(chosen[n].cell, TRIBUTARY_CELLS, 2.0f * CELL_SIZE * CELL_SIZE);
            addRiver(paths[std::size_t(r)], land.sites[std::size_t(main[chosen[n].along])]);
        }
        // An island too small for all its tributaries: the regions left
        // start from cells of the main river.
        for (std::size_t n = chosen.size(); n < wanted; ++n)
        {
            paths[std::size_t(island_regions[k][n])] = { main[(n * main.size()) / std::max<std::size_t>(wanted, 1u)] };
        }
    }
    // The segments, and the squares their valleys reach.
    land.buckets.assign(std::size_t(BUCKETS) * BUCKETS, {});
    for (River const& river : land.rivers)
    {
        for (std::size_t k = 0u; (k + 1u) < river.points.size(); ++k)
        {
            const Land::Segment segment{ river.points[k], river.points[k + 1u], river.widths[k], river.widths[k + 1u] };
            const float reach = valleyReach(0.5f * std::max(segment.width_a, segment.width_b));
            const int index = int(land.segments.size());
            land.segments.push_back(segment);
            const int i0 = std::clamp(int((std::min(segment.a.x, segment.b.x) - reach + (0.5f * MAP)) / BUCKET_SIZE), 0, BUCKETS - 1);
            const int i1 = std::clamp(int((std::max(segment.a.x, segment.b.x) + reach + (0.5f * MAP)) / BUCKET_SIZE), 0, BUCKETS - 1);
            const int j0 = std::clamp(int((std::min(segment.a.y, segment.b.y) - reach + (0.5f * MAP)) / BUCKET_SIZE), 0, BUCKETS - 1);
            const int j1 = std::clamp(int((std::max(segment.a.y, segment.b.y) + reach + (0.5f * MAP)) / BUCKET_SIZE), 0, BUCKETS - 1);
            for (int j = j0; j <= j1; ++j)
            {
                for (int i = i0; i <= i1; ++i)
                {
                    land.buckets[std::size_t((j * BUCKETS) + i)].push_back(index);
                }
            }
        }
    }

    // --- The regions: the cells nearest each tributary ---------------------
    // A breadth-first walk on the graph of the cells, from all the
    // tributaries at once, inside their island: each cell goes to the
    // tributary it is fewest cells from.
    p_progress(0.7f, "Regions");
    land.regions_of.assign(count, -1);
    land.parts_of.assign(count, -1);
    std::vector<int> steps_from_river(count, -1);
    {
        std::vector<int> queue;
        for (std::size_t r = 0u; r < land.regions.size(); ++r)
        {
            for (const int c : paths[r])
            {
                if (land.regions_of[std::size_t(c)] < 0)
                {
                    land.regions_of[std::size_t(c)] = int(r);
                    steps_from_river[std::size_t(c)] = 0;
                    queue.push_back(c);
                }
            }
        }
        for (std::size_t head = 0u; head < queue.size(); ++head)
        {
            const int c = queue[head];
            const int ci = c % CELLS;
            const int cj = c / CELLS;
            for (int j = std::max(0, cj - 1); j <= std::min(CELLS - 1, cj + 1); ++j)
            {
                for (int i = std::max(0, ci - 1); i <= std::min(CELLS - 1, ci + 1); ++i)
                {
                    const std::size_t n = std::size_t(cellAt(i, j));
                    if ((land.regions_of[n] < 0) && (outlet[n] == 0) && (land.islands_of[n] == land.islands_of[std::size_t(c)]))
                    {
                        land.regions_of[n] = land.regions_of[std::size_t(c)];
                        steps_from_river[n] = steps_from_river[std::size_t(c)] + 1;
                        queue.push_back(int(n));
                    }
                }
            }
        }
    }
    // The links: cells spread along the tributary, from its mouth to its
    // source; a short tributary lends its nearest cells.
    for (std::size_t r = 0u; r < land.regions.size(); ++r)
    {
        Region& region = land.regions[r];
        std::vector<int> const& path = paths[r];
        const std::size_t links = p_recipe.region_parts[r];
        std::vector<int> cells;
        if (path.size() >= links)
        {
            for (std::size_t k = 0u; k < links; ++k)
            {
                cells.push_back(path[((2u * k + 1u) * path.size()) / (2u * links)]);
            }
        }
        else
        {
            cells = path;
            std::vector<int> near;
            for (std::size_t c = 0u; c < count; ++c)
            {
                if ((land.regions_of[c] == int(r)) && (steps_from_river[c] > 0))
                {
                    near.push_back(int(c));
                }
            }
            std::sort(near.begin(), near.end(), [&](int p_a, int p_b)
                      { return steps_from_river[std::size_t(p_a)] < steps_from_river[std::size_t(p_b)]; });
            for (std::size_t k = 0u; (cells.size() < links) && (k < near.size()); ++k)
            {
                cells.push_back(near[k]);
            }
        }
        for (std::size_t k = 0u; k < links; ++k)
        {
            // An island of a single cell: every link on it.
            const int cell = cells.empty() ? 0 : cells[std::min(k, cells.size() - 1u)];
            region.parts.push_back(Part{ cell, land.sites[std::size_t(cell)] });
            if (!cells.empty() && (k < cells.size()))
            {
                land.parts_of[std::size_t(cell)] = int(k);
            }
        }
        region.site = path.empty() ? region.parts.front().site : land.sites[std::size_t(path[path.size() / 2u])];
    }
    for (std::size_t k = 0u; k < land.islands.size(); ++k)
    {
        float highest = -1.0e9f;
        for (std::size_t c = 0u; c < count; ++c)
        {
            if ((land.islands_of[c] == std::int8_t(k)) && (land.elevations[c] > highest))
            {
                highest = land.elevations[c];
                land.islands[k].peak = Vector3f(land.sites[c].x, highest, land.sites[c].y);
            }
        }
    }

    // --- The outlines of the regions --------------------------------------
    // Traced on a grid of 31 m around the cells of each region: inside where
    // the distance to the border of the region is positive.
    {
        const std::size_t side = std::size_t(OUTLINE_GRID + 1);
        std::vector<float> field(side * side, -1.0f);
        constexpr float STEP = MAP / float(OUTLINE_GRID);
        for (std::size_t r = 0u; r < land.regions.size(); ++r)
        {
            p_progress(0.75f + (0.25f * float(r) / float(land.regions.size())), "Outlines");
            std::array<int, 4> box = { OUTLINE_GRID, OUTLINE_GRID, 0, 0 };
            for (std::size_t c = 0u; c < count; ++c)
            {
                if (land.regions_of[c] == int(r))
                {
                    const int i = int((land.sites[c].x + (0.5f * MAP)) / STEP);
                    const int j = int((land.sites[c].y + (0.5f * MAP)) / STEP);
                    box = { std::min(box[0], i), std::min(box[1], j), std::max(box[2], i), std::max(box[3], j) };
                }
            }
            if (box[2] < box[0])
            {
                continue;
            }
            const int margin = int(CELL_SIZE / STEP) + 2;
            box = { std::max(0, box[0] - margin), std::max(0, box[1] - margin), std::min(OUTLINE_GRID, box[2] + margin),
                    std::min(OUTLINE_GRID, box[3] + margin) };
            for (int j = box[1]; j <= box[3]; ++j)
            {
                for (int i = box[0]; i <= box[2]; ++i)
                {
                    const Vector2f p((-0.5f * MAP) + (float(i) * STEP), (-0.5f * MAP) + (float(j) * STEP));
                    field[(std::size_t(j) * side) + std::size_t(i)] =
                        groupField(land.sites, p, [&](int p_c) { return land.regions_of[std::size_t(p_c)] == int(r); });
                }
            }
            land.regions[r].border = outline(land, field, box);
        }
    }
    p_progress(1.0f, "Map ready");
    return land;
}

// ----------------------------------------------------------------------------
Patch makePatch(Land const& p_land, float p_x, float p_z, float p_size, int p_cells, int p_pixels, Progress const& p_progress)
{
    Patch patch;
    patch.x = p_x;
    patch.z = p_z;
    patch.size = p_size;
    patch.cells = p_cells;
    patch.pixels = p_pixels;
    const float step = p_size / float(p_cells);
    const int side = p_cells + 3;
    patch.heights.resize(std::size_t(side) * std::size_t(side));
    for (int j = -1; j <= p_cells + 1; ++j)
    {
        for (int i = -1; i <= p_cells + 1; ++i)
        {
            patch.heights[std::size_t(((j + 1) * side) + i + 1)] =
                p_land.heightAt(p_x + (float(i) * step), p_z + (float(j) * step));
        }
        if (p_progress)
        {
            p_progress(0.3f * float(j + 1) / float(p_cells + 2), "Relief");
        }
    }
    // The heights between the corners of the patch (bilinear): the picture
    // needs them at each pixel, and the mesh shows no more.
    auto heightAt = [&](float p_px, float p_pz)
    {
        const float gx = std::clamp((p_px - p_x) / step, -1.0f, float(p_cells) + 0.999f);
        const float gz = std::clamp((p_pz - p_z) / step, -1.0f, float(p_cells) + 0.999f);
        const int i = int(std::floor(gx));
        const int j = int(std::floor(gz));
        const float tx = gx - float(i);
        const float tz = gz - float(j);
        return (patch.height(i, j) * (1.0f - tx) * (1.0f - tz)) + (patch.height(i + 1, j) * tx * (1.0f - tz)) +
               (patch.height(i, j + 1) * (1.0f - tx) * tz) + (patch.height(i + 1, j + 1) * tx * tz);
    };
    const float pixel = p_size / float(p_pixels);
    patch.picture.resize(std::size_t(p_pixels) * std::size_t(p_pixels) * 4u);
    for (int t = 0; t < p_pixels; ++t)
    {
        for (int s = 0; s < p_pixels; ++s)
        {
            const float x = p_x + ((float(s) + 0.5f) * pixel);
            const float z = p_z + ((float(t) + 0.5f) * pixel);
            const float h = heightAt(x, z);
            const float slope = (std::abs(heightAt(x + pixel, z) - h) + std::abs(heightAt(x, z + pixel) - h)) / pixel;
            paint(p_land, x, z, h, slope, pixel, &patch.picture[((std::size_t(t) * std::size_t(p_pixels)) + std::size_t(s)) * 4u]);
        }
        if (p_progress && ((t % 16) == 0))
        {
            p_progress(0.3f + (0.7f * float(t) / float(p_pixels)), "Picture");
        }
    }
    if (p_progress)
    {
        p_progress(1.0f, "Picture");
    }
    return patch;
}

} // namespace examples::archipelago
