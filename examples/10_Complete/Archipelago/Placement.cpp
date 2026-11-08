// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Placement.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace examples::archipelago
{

using compages::Vector2f;
using units::literals::operator""_km;

//! \brief The share of the map the islands cover, and the sea kept along
//! its edges.
static constexpr float LAND = 0.45f;
static constexpr float MARGIN = compages::toMetres(1.4_km);
static constexpr float USABLE = MAP - (2.0f * MARGIN);
//! \brief The smallest island: a few cells across.
static constexpr float SMALLEST = 3.0f * CELL_SIZE;
//! \brief The strait between two islands, at least.
static constexpr float STRAIT = 4.0f * CELL_SIZE;

static float length(Vector2f p_v)
{
    return std::sqrt((p_v.x * p_v.x) + (p_v.y * p_v.y));
}

//! \brief The middle of points, and their spread (root mean square of the
//! distances to the middle).
struct Cloud
{
    Vector2f middle{ 0.0f, 0.0f };
    float spread = 0.0f;
};

static Cloud cloudOf(std::vector<Vector2f> const& p_points)
{
    Cloud cloud;
    for (const Vector2f p : p_points)
    {
        cloud.middle += p * (1.0f / float(p_points.size()));
    }
    float sum = 0.0f;
    for (const Vector2f p : p_points)
    {
        const Vector2f d = p - cloud.middle;
        sum += (d.x * d.x) + (d.y * d.y);
    }
    cloud.spread = std::sqrt(sum / float(p_points.size()));
    return cloud;
}

// ----------------------------------------------------------------------------
//! \brief The cloud of the nodes of each island (its own, its regions', its
//! links') and its disc: an area per link.
// ----------------------------------------------------------------------------
static std::vector<Cloud> islandsOf(Recipe const& p_recipe,
                                    std::vector<float>& p_radii)
{
    const std::size_t islands = p_recipe.islands.size();
    std::vector<std::vector<Vector2f>> members(islands);
    std::vector links(islands, 1.0f);
    for (std::size_t k = 0u; k < islands; ++k)
    {
        members[k].push_back(p_recipe.islands[k]);
    }
    for (Recipe::Group const& region : p_recipe.regions)
    {
        std::vector<Vector2f>& nodes = members[region.island];
        nodes.push_back(region.node);
        nodes.insert(nodes.end(), region.links.begin(), region.links.end());
        links[region.island] += float(region.links.size());
    }
    float total = 0.0f;
    for (const float n : links)
    {
        total += n;
    }
    std::vector<Cloud> clouds;
    p_radii.clear();
    for (std::size_t k = 0u; k < islands; ++k)
    {
        clouds.push_back(cloudOf(members[k]));
        const float area = LAND * USABLE * USABLE * links[k] / total;
        p_radii.push_back(
            std::max(SMALLEST, std::sqrt(area / std::numbers::pi_v<float>)));
    }
    return clouds;
}

// ----------------------------------------------------------------------------
//! \brief Push two discs apart when they are nearer than a strait; the
//! smaller one moves more. \return They were apart already.
// ----------------------------------------------------------------------------
static bool separate(Placement& p_placement, std::size_t p_i, std::size_t p_j)
{
    std::vector<Vector2f>& centres = p_placement.centres;
    std::vector<float> const& radii = p_placement.radii;
    const float need = radii[p_i] + radii[p_j] + STRAIT;
    Vector2f d = centres[p_i] - centres[p_j];
    float distance = length(d);
    if (distance >= need)
    {
        return true;
    }
    if (distance < 1.0f)
    {
        // On top of each other: apart along an angle of their own.
        const float angle = 2.39996323f * float(p_i + (7u * p_j));
        d = Vector2f(std::cos(angle), std::sin(angle));
        distance = 1.0f;
    }
    const float ai = radii[p_i] * radii[p_i];
    const float aj = radii[p_j] * radii[p_j];
    const Vector2f push = d * ((need - distance) / distance);
    centres[p_i] += push * (aj / (ai + aj));
    centres[p_j] -= push * (ai / (ai + aj));
    return false;
}

// ----------------------------------------------------------------------------
//! \brief The centres of the discs: the middles of the layout, centred and
//! scaled to the map, then pushed apart until a strait separates them all
//! (the discs shrink a little when the map has not room enough).
// ----------------------------------------------------------------------------
static void spread(Placement& p_placement, std::vector<Cloud> const& p_clouds)
{
    const std::size_t islands = p_clouds.size();
    std::vector<Vector2f> middles;
    for (Cloud const& cloud : p_clouds)
    {
        middles.push_back(cloud.middle);
    }
    const Cloud layout = cloudOf(middles);
    const float scale =
        (layout.spread > 0.0f) ? ((0.4f * USABLE) / layout.spread) : 0.0f;
    for (const Vector2f m : middles)
    {
        p_placement.centres.push_back((m - layout.middle) * scale);
    }
    for (int round = 0; round < 400; ++round)
    {
        bool apart = true;
        for (std::size_t i = 0u; i < islands; ++i)
        {
            for (std::size_t j = i + 1u; j < islands; ++j)
            {
                apart = separate(p_placement, i, j) && apart;
            }
            const float limit =
                std::max(0.0f, (0.5f * USABLE) - p_placement.radii[i]);
            p_placement.centres[i].x =
                std::clamp(p_placement.centres[i].x, -limit, limit);
            p_placement.centres[i].y =
                std::clamp(p_placement.centres[i].y, -limit, limit);
        }
        if (apart)
        {
            return;
        }
        if ((round % 40) == 39)
        {
            for (float& r : p_placement.radii)
            {
                r = std::max(SMALLEST, 0.95f * r);
            }
        }
    }
}

// ----------------------------------------------------------------------------
Placement place(Recipe const& p_recipe)
{
    Placement placement;
    const std::vector<Cloud> clouds = islandsOf(p_recipe, placement.radii);
    spread(placement, clouds);

    // The nodes of each island, scaled into its disc and kept inside it.
    auto into = [&clouds, &placement](std::size_t p_island, Vector2f p_node)
    {
        Cloud const& cloud = clouds[p_island];
        const float radius = placement.radii[p_island];
        const float s =
            (cloud.spread > 0.0f) ? ((0.62f * radius) / cloud.spread) : 0.0f;
        Vector2f d = (p_node - cloud.middle) * s;
        if (const float most = 0.9f * radius; length(d) > most)
        {
            d = d * (most / length(d));
        }
        return placement.centres[p_island] + d;
    };
    for (std::size_t k = 0u; k < p_recipe.islands.size(); ++k)
    {
        placement.islands.push_back(into(k, p_recipe.islands[k]));
    }
    for (Recipe::Group const& region : p_recipe.regions)
    {
        placement.regions.push_back(into(region.island, region.node));
        std::vector<Vector2f>& links = placement.links.emplace_back();
        for (const Vector2f link : region.links)
        {
            links.push_back(into(region.island, link));
        }
    }
    return placement;
}

} // namespace examples::archipelago
