// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/FlowingWater/FlowBake.hpp"
#include "10_Complete/FlowingWater/Pictures.hpp"

#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <random>

namespace examples::flowing
{

using compages::Vector2f;
using compages::Vector3f;
using compages::Vector4f;

namespace
{

//! \brief Half the thickness of the slab around the water: what is further
//! from the level does not touch the water (a catwalk above, a floor below).
constexpr float SLAB = 0.25f;

float smoothstep(float p_from, float p_to, float p_x)
{
    const float t = std::clamp((p_x - p_from) / (p_to - p_from), 0.0f, 1.0f);
    return t * t * (3.0f - (2.0f * t));
}

float length(Vector2f p_v)
{
    return std::hypot(p_v.x, p_v.y);
}

// ----------------------------------------------------------------------------
//! \brief The bump of a feature at x = r / R: zero at the centre (the water
//! rises or turns there) and at the rim, one at its strongest. It is what the
//! paper gets from metaballs: the delta between a point and where the
//! metaball pulls it.
// ----------------------------------------------------------------------------
float bump(float p_x)
{
    constexpr float PEAK = 0.2862f; // x (1 - x^2)^2 at x = 1 / sqrt(5)
    const float rim = 1.0f - (p_x * p_x);
    return (p_x * rim * rim) / PEAK;
}

} // namespace

// ----------------------------------------------------------------------------
FlowBake::FlowBake(compages::Grid2D const& p_grid)
    : m_grid(p_grid), m_solid(p_grid.count(), false), m_nearest(p_grid.count(), Vector2f(0.0f, 0.0f)),
      m_distance(p_grid, 1.0e6f), m_away(p_grid, Vector2f(0.0f, 0.0f)), m_flow(p_grid, Vector2f(0.0f, 0.0f)),
      m_foam(p_grid, 0.0f), m_turbulence(p_grid, 0.0f), m_open(p_grid, 1.0f),
      m_packed(p_grid, Vector4f(0.0f, 0.0f, 0.0f, 1.0f))
{
}

// ----------------------------------------------------------------------------
void FlowBake::footprint(std::span<const compages::world::Entity> p_solids,
                         compages::renderer::AssetManager const& p_assets,
                         float p_level)
{
    std::fill(m_solid.begin(), m_solid.end(), false);
    for (compages::world::Entity const& solid : p_solids)
    {
        if (!solid.has<compages::renderer::MeshRenderer>())
        {
            continue;
        }
        compages::renderer::MeshAsset const* mesh =
            p_assets.mesh(solid.get<compages::renderer::MeshRenderer>().mesh);
        if (mesh == nullptr)
        {
            continue;
        }
        compages::Matrix44f const& matrix = solid.worldMatrix();
        auto place = [&](std::uint32_t p_index) {
            Vector3f const& p = mesh->source_vertices[p_index].position;
            const Vector4f w = matrix * Vector4f(p.x, p.y, p.z, 1.0f);
            return Vector3f(w.x, w.y, w.z);
        };
        std::vector<std::uint32_t> const& indices = mesh->source_indices;
        for (std::size_t k = 0u; k + 2u < indices.size(); k += 3u)
        {
            scatter(place(indices[k]), place(indices[k + 1u]), place(indices[k + 2u]), p_level);
        }
    }
    distanceTransform();
}

// ----------------------------------------------------------------------------
//! Points on a triangle at half the spacing of the grid (so that no cell is
//! missed), by its barycentric coordinates; only those in the slab count.
// ----------------------------------------------------------------------------
void FlowBake::scatter(Vector3f const& p_a, Vector3f const& p_b, Vector3f const& p_c, float p_level)
{
    const float low = std::min({ p_a.y, p_b.y, p_c.y });
    const float high = std::max({ p_a.y, p_b.y, p_c.y });
    if ((low > p_level + SLAB) || (high < p_level - SLAB))
    {
        return;
    }
    const float edge = std::max({ compages::vector::distance(p_a, p_b), compages::vector::distance(p_b, p_c),
                                  compages::vector::distance(p_c, p_a) });
    const int n = std::clamp(int(std::ceil(edge / (0.5f * m_grid.spacing))), 1, 600);
    for (int i = 0; i <= n; ++i)
    {
        for (int j = 0; i + j <= n; ++j)
        {
            const float s = float(i) / float(n);
            const float t = float(j) / float(n);
            const Vector3f p = p_a + ((p_b - p_a) * s) + ((p_c - p_a) * t);
            if (std::abs(p.y - p_level) > SLAB)
            {
                continue;
            }
            const Vector2f flat(p.x, p.z);
            const compages::Vector2i cell = m_grid.nearest(flat);
            if (!m_grid.contains(cell.x, cell.y))
            {
                continue;
            }
            const std::size_t k = m_grid.index(cell.x, cell.y);
            m_solid[k] = true;
            m_nearest[k] = flat;
        }
    }
}

// ----------------------------------------------------------------------------
//! Each point takes the nearest scattered point among those its neighbours
//! know: a pass from the bottom left, a pass back from the top right, twice
//! (dead reckoning; close to the true distance, and it keeps the point, so
//! the direction away from the wall comes with it).
// ----------------------------------------------------------------------------
void FlowBake::distanceTransform()
{
    std::vector<bool> known = m_solid;
    auto relax = [&](int p_i, int p_j, int p_di, int p_dj) {
        if (!m_grid.contains(p_i + p_di, p_j + p_dj))
        {
            return;
        }
        const std::size_t from = m_grid.index(p_i + p_di, p_j + p_dj);
        if (!known[from])
        {
            return;
        }
        const std::size_t to = m_grid.index(p_i, p_j);
        const Vector2f p = m_grid.point(p_i, p_j);
        if (!known[to] || (length(p - m_nearest[from]) < length(p - m_nearest[to])))
        {
            m_nearest[to] = m_nearest[from];
            known[to] = true;
        }
    };
    constexpr std::array<std::array<int, 2>, 4> before = { { { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 } } };
    for (int round = 0; round < 2; ++round)
    {
        for (int j = 0; j < m_grid.rows; ++j)
        {
            for (int i = 0; i < m_grid.columns; ++i)
            {
                for (auto const& [di, dj] : before)
                {
                    relax(i, j, di, dj);
                }
            }
        }
        for (int j = m_grid.rows - 1; j >= 0; --j)
        {
            for (int i = m_grid.columns - 1; i >= 0; --i)
            {
                for (auto const& [di, dj] : before)
                {
                    relax(i, j, -di, -dj);
                }
            }
        }
    }
    for (std::size_t k = 0u; k < m_grid.count(); ++k)
    {
        if (!known[k])
        {
            m_distance[k] = 1.0e6f;
            m_away[k] = Vector2f(0.0f, 0.0f);
            continue;
        }
        const Vector2f out = m_grid.point(k) - m_nearest[k];
        m_distance[k] = length(out);
        m_away[k] = (m_distance[k] > 1.0e-4f) ? (out * (1.0f / m_distance[k])) : Vector2f(0.0f, 0.0f);
    }
}

// ----------------------------------------------------------------------------
//! The slow motion of the pool. One direction, bent round the middle of the
//! pool by the circulation; near a wall the part going into it is taken
//! away (the current slides along the wall), and some is thrown back (the
//! paper projects the normals of the props onto the water: here the
//! direction away from the nearest wall).
// ----------------------------------------------------------------------------
Vector2f FlowBake::current(std::size_t p_index, FlowRecipe const& p_recipe) const
{
    const float angle = p_recipe.direction * std::numbers::pi_v<float> / 180.0f;
    const Vector2f along(std::cos(angle), std::sin(angle));
    const Vector2f half(0.5f * m_grid.spacing * float(m_grid.columns - 1),
                        0.5f * m_grid.spacing * float(m_grid.rows - 1));
    const Vector2f centre = m_grid.origin + half;
    const Vector2f p = m_grid.point(p_index);
    const Vector2f q((p.x - centre.x) / half.x, (p.y - centre.y) / half.y);
    const float out = length(q);
    const Vector2f round = (out > 1.0e-4f) ? Vector2f(-q.y / out, q.x / out) : Vector2f(0.0f, 0.0f);
    Vector2f v = (along * (1.0f - p_recipe.circulation)) + (round * (p_recipe.circulation * std::min(1.5f * out, 1.0f)));
    const float size = length(v);
    v = (size > 1.0e-4f) ? (v * (p_recipe.speed / size)) : Vector2f(0.0f, 0.0f);

    const Vector2f n = m_away[p_index];
    const float d = m_distance[p_index];
    const float into = compages::vector::dot(v, n);
    if (into < 0.0f)
    {
        v -= n * (into * std::exp(-d / (1.5f * p_recipe.edge_width)));
        v += n * (p_recipe.deflection * -into * std::exp(-d / p_recipe.edge_width));
    }
    return v;
}

// ----------------------------------------------------------------------------
void FlowBake::placeFeatures(FlowRecipe const& p_recipe)
{
    m_features.clear();
    std::mt19937 random(p_recipe.seed);
    const float width = m_grid.spacing * float(m_grid.columns - 1);
    const float depth = m_grid.spacing * float(m_grid.rows - 1);
    std::uniform_real_distribution<float> x(0.0f, width);
    std::uniform_real_distribution<float> z(0.0f, depth);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    auto place = [&](int p_count, float p_radius, float p_strength, bool p_vortex) {
        for (int n = 0; n < p_count; ++n)
        {
            for (int attempt = 0; attempt < 60; ++attempt)
            {
                const Vector2f centre = m_grid.origin + Vector2f(x(random), z(random));
                const float radius = p_radius * (0.75f + (0.5f * unit(random)));
                if (m_distance.sample(centre) < (p_vortex ? 0.5f : 0.7f) * radius)
                {
                    continue;
                }
                m_features.push_back({ .centre = centre,
                                       .radius = radius,
                                       .strength = p_strength * (0.7f + (0.5f * unit(random))),
                                       .vortex = p_vortex,
                                       .spin = (unit(random) < 0.5f) ? -1.0f : 1.0f });
                break;
            }
        }
    };
    // The upwellings are much larger than the vortices: the interesting
    // motion is where they meet.
    place(p_recipe.upwellings, p_recipe.upwelling_radius, p_recipe.upwelling_strength, false);
    place(p_recipe.vortices, p_recipe.vortex_radius, p_recipe.vortex_strength, true);
}

// ----------------------------------------------------------------------------
void FlowBake::bake(FlowRecipe const& p_recipe, std::span<const Feature> p_extra)
{
    const std::size_t count = m_grid.count();

    // 1. The current, smoothed a little (the bends round the walls are
    //    sharp from point to point).
    for (std::size_t k = 0u; k < count; ++k)
    {
        m_flow[k] = current(k, p_recipe);
    }
    compages::Field<Vector2f> smoothed = m_flow;
    for (int pass = 0; pass < 4; ++pass)
    {
        for (std::size_t k = 0u; k < count; ++k)
        {
            Vector2f sum = m_flow[k];
            float weight = 1.0f;
            for (std::size_t n : m_grid.neighbours8(k))
            {
                if (!m_solid[n])
                {
                    sum += m_flow[n];
                    weight += 1.0f;
                }
            }
            smoothed[k] = sum * (1.0f / weight);
        }
        std::swap(m_flow, smoothed);
    }

    // 2. The turbulence mask: in the paper the artist paints it; here a slow
    //    noise, so that some parts of the pool are calm.
    placeFeatures(p_recipe);
    for (std::size_t k = 0u; k < count; ++k)
    {
        const Vector2f p = m_grid.point(k);
        m_turbulence[k] = smoothstep(0.38f, 0.62f, tiledFbm(p.x / 7.0f, p.y / 7.0f, 1024, 3, p_recipe.seed + 100u));
    }

    // 3. Upwellings spread out, vortices turn (the cross product of the
    //    spreading with the up vector), added where the mask lets them; the
    //    features of the user count everywhere.
    std::vector<Feature> all = m_features;
    all.insert(all.end(), p_extra.begin(), p_extra.end());
    for (std::size_t k = 0u; k < count; ++k)
    {
        const Vector2f p = m_grid.point(k);
        Vector2f turbulent(0.0f, 0.0f);
        Vector2f clicked(0.0f, 0.0f);
        float rising = 0.0f;
        for (std::size_t f = 0u; f < all.size(); ++f)
        {
            Feature const& feature = all[f];
            const Vector2f delta = p - feature.centre;
            const float r = length(delta);
            if ((r >= feature.radius) || (r < 1.0e-5f))
            {
                continue;
            }
            const float x = r / feature.radius;
            const Vector2f out = delta * (1.0f / r);
            const Vector2f push = feature.vortex ? (Vector2f(-out.y, out.x) * feature.spin) : out;
            const Vector2f added = push * (feature.strength * bump(x));
            (f < m_features.size() ? turbulent : clicked) += added;
            if (!feature.vortex)
            {
                const float rim = 1.0f - (x * x);
                rising = std::max(rising, rim * rim);
            }
        }
        const float open = smoothstep(0.0f, p_recipe.edge_width, m_distance[k]);
        m_open[k] = open;
        const float turbulence = p_recipe.turbulence * m_turbulence[k];
        Vector2f v = (m_flow[k] + (turbulent * turbulence) + clicked) * (0.08f + (0.92f * open));
        if (m_solid[k])
        {
            v = Vector2f(0.0f, 0.0f);
        }
        m_flow[k] = v;

        // 4. The foam: less where the water rises (it pushes the foam away)
        //    and where it is turbulent, more where it is slow and along the
        //    walls, where it gathers.
        const float slow = 1.0f - std::min(length(v) / std::max(1.2f * p_recipe.speed, 1.0e-3f), 1.0f);
        float foam = 0.5f + (0.35f * (1.0f - open)) + (0.3f * slow);
        foam -= 0.8f * rising;
        foam -= 0.2f * turbulence;
        m_foam[k] = std::clamp(foam, 0.0f, 1.0f);
        m_packed[k] = Vector4f(v.x, v.y, m_foam[k], open);
    }
}

} // namespace examples::flowing
