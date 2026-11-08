// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Rivers.hpp"
#include "10_Complete/Archipelago/Noise.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace examples::archipelago
{

using compages::Vector2f;
using units::literals::operator""_m;

//! \brief A brook is 5 m wide, 8 m more per square root of km2 drained.
static constexpr float RIVER_WIDTH = compages::toMetres(5.0_m);
static constexpr float RIVER_GROWTH = compages::toMetres(8.0_m);
//! \brief A river meanders with a wavelength of about eleven times its
//! width, about its width to each side.
static constexpr float MEANDER_WAVELENGTH = 11.0f;
static constexpr float MEANDER_SWING = 0.9f;
//! \brief A point of a meandering river every 6 m.
static constexpr float MEANDER_STEP = compages::toMetres(6.0_m);
//! \brief The squares that index the segments.
static constexpr int BUCKETS = 64;
static constexpr float BUCKET_SIZE = MAP / float(BUCKETS);

// ----------------------------------------------------------------------------
float riverWidth(float p_area)
{
    return RIVER_WIDTH + (RIVER_GROWTH * std::sqrt(p_area / 1.0e6f));
}

// ----------------------------------------------------------------------------
float valleyReach(float p_half)
{
    return (6.0f * p_half) + compages::toMetres(90.0_m);
}

// ----------------------------------------------------------------------------
//! A Catmull-Rom spline: six points between two of the path.
// ----------------------------------------------------------------------------
static void spline(std::vector<Vector2f> const& p_points,
                   std::vector<float> const& p_widths,
                   std::vector<Vector2f>& p_curve,
                   std::vector<float>& p_curve_widths)
{
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
            p_curve.push_back(
                ((p1 * 2.0f) + ((p2 - p0) * t) +
                 (((p0 * 2.0f) - (p1 * 5.0f) + (p2 * 4.0f) - p3) * t2) +
                 (((p1 * 3.0f) - p0 - (p2 * 3.0f) + p3) * t3)) *
                0.5f);
            p_curve_widths.push_back(p_widths[k] +
                                     ((p_widths[k + 1u] - p_widths[k]) * t));
        }
    }
    p_curve.push_back(p_points.back());
    p_curve_widths.push_back(p_widths.back());
}

// ----------------------------------------------------------------------------
//! Points every few metres, pushed across the path by a sine whose
//! wavelength follows the width. No two bends alike: the wavelength and the
//! swing vary with the noise, and a second harmonic makes the loops lean.
// ----------------------------------------------------------------------------
static River meander(std::vector<Vector2f> const& p_points,
                     std::vector<float> const& p_widths,
                     std::uint32_t p_seed)
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
    for (int n = 0; float(n) * MEANDER_STEP <= total; ++n)
    {
        const float s = float(n) * MEANDER_STEP;
        while (((k + 2u) < along.size()) && (along[k + 1u] < s))
        {
            ++k;
        }
        const float span = std::max(along[k + 1u] - along[k], 1.0e-3f);
        const float t = std::clamp((s - along[k]) / span, 0.0f, 1.0f);
        const Vector2f a = p_points[k];
        const Vector2f b = p_points[k + 1u];
        const float width =
            p_widths[k] + ((p_widths[k + 1u] - p_widths[k]) * t);
        const Vector2f tangent = (b - a) * (1.0f / span);
        const Vector2f across(-tangent.y, tangent.x);
        const Vector2f at = a + ((b - a) * t);
        const float stretch =
            0.7f + (0.8f * noise(at.x / 300.0f, at.y / 300.0f, p_seed + 93u));
        phase += 2.0f * std::numbers::pi_v<float> * MEANDER_STEP /
                 (MEANDER_WAVELENGTH * width * stretch);
        const float ends = std::min(
            1.0f, std::min(s, total - s) / (2.0f * MEANDER_WAVELENGTH * width));
        const float swing =
            MEANDER_SWING * width * ends *
            (0.25f +
             (1.75f * noise(at.x / 500.0f, at.y / 500.0f, p_seed + 91u)));
        const float bend =
            std::sin(phase) + (0.3f * std::sin((2.0f * phase) + 0.8f));
        result.points.push_back(at + (across * (swing * bend)));
        result.widths.push_back(width);
    }
    result.points.push_back(p_points.back());
    result.widths.push_back(p_widths.back());
    return result;
}

// ----------------------------------------------------------------------------
River makeRiver(std::vector<Vector2f> const& p_points,
                std::vector<float> const& p_widths,
                std::uint32_t p_seed)
{
    std::vector<Vector2f> curve;
    std::vector<float> widths;
    spline(p_points, p_widths, curve, widths);
    return meander(curve, widths, p_seed);
}

//! \brief The square of the index a coordinate falls in.
static int bucketOf(float p_coordinate)
{
    return std::clamp(
        int((p_coordinate + (0.5f * MAP)) / BUCKET_SIZE), 0, BUCKETS - 1);
}

// ----------------------------------------------------------------------------
void RiverIndex::build(std::vector<River> const& p_rivers)
{
    m_segments.clear();
    m_buckets.assign(std::size_t(BUCKETS) * BUCKETS, {});
    for (River const& river : p_rivers)
    {
        for (std::size_t k = 0u; (k + 1u) < river.points.size(); ++k)
        {
            const Segment segment{ river.points[k],
                                   river.points[k + 1u],
                                   river.widths[k],
                                   river.widths[k + 1u] };
            const float reach =
                valleyReach(0.5f * std::max(segment.width_a, segment.width_b));
            const auto index = int(m_segments.size());
            m_segments.push_back(segment);
            const int i1 = bucketOf(std::max(segment.a.x, segment.b.x) + reach);
            const int j1 = bucketOf(std::max(segment.a.y, segment.b.y) + reach);
            for (int j = bucketOf(std::min(segment.a.y, segment.b.y) - reach);
                 j <= j1;
                 ++j)
            {
                for (int i =
                         bucketOf(std::min(segment.a.x, segment.b.x) - reach);
                     i <= i1;
                     ++i)
                {
                    m_buckets[std::size_t((j * BUCKETS) + i)].push_back(index);
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
void RiverIndex::nearest(float p_x,
                         float p_z,
                         float& p_distance,
                         float& p_half) const
{
    p_distance = 1.0e9f;
    p_half = 0.0f;
    if (m_buckets.empty())
    {
        return;
    }
    for (const int index :
         m_buckets[std::size_t((bucketOf(p_z) * BUCKETS) + bucketOf(p_x))])
    {
        Segment const& segment = m_segments[std::size_t(index)];
        const Vector2f ab = segment.b - segment.a;
        const float length2 = std::max((ab.x * ab.x) + (ab.y * ab.y), 1.0e-6f);
        const float t = std::clamp(
            (((p_x - segment.a.x) * ab.x) + ((p_z - segment.a.y) * ab.y)) /
                length2,
            0.0f,
            1.0f);
        const Vector2f d = Vector2f(p_x, p_z) - (segment.a + (ab * t));
        if (const float distance = std::sqrt((d.x * d.x) + (d.y * d.y));
            distance < p_distance)
        {
            p_distance = distance;
            p_half = 0.5f * (segment.width_a +
                             ((segment.width_b - segment.width_a) * t));
        }
    }
}

} // namespace examples::archipelago
