// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Wildlife.hpp"
#include "10_Complete/Archipelago/Swells.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace examples::archipelago
{

using compages::Quatf;
using compages::Vector2f;
using compages::Vector3f;

static constexpr float TURN = 2.0f * std::numbers::pi_v<float>;
static constexpr float HALF_TURN = std::numbers::pi_v<float>;

//! \brief A boat, a bird, a fish are a few metres long: from a few km they
//! would be smaller than a pixel. They are drawn at least this part of
//! their distance to the camera long.
static constexpr float BOAT_SEEN = 1.0f / 1100.0f;
//! \brief A gull (2 m across) life size is 2 or 3 pixels from the camera.
//! It is drawn larger, by (distance / BIRD_NEAR)^BIRD_GROWTH: less than
//! the distance, so on the screen it still gets larger when the camera
//! comes closer and smaller when it goes away, like the rest of the map,
//! only slower: each time the camera goes 2.5 times farther, it looks
//! about 1.4 times smaller. Life size closer than BIRD_NEAR. (Grown as
//! much as the distance, it kept the same size on the screen and looked
//! larger and larger next to the islands as the camera went away.)
static constexpr float BIRD_NEAR = 6.0f;
static constexpr float BIRD_GROWTH = 0.62f;
//! \brief But a gull never looks larger than a boat drawn at the same
//! distance (a boat is 11 m long, a gull 2.2 m across): from afar it is
//! drawn at most as long as a boat, so it stays visible without looking
//! bigger than the boats. (Two fifths of a boat made it a dot again.)
static constexpr float BOAT_LENGTH = 11.0f;
static constexpr float GULL_SPAN = 2.2f;
static constexpr float GULL_TO_BOAT = 1.0f;
//! \brief Gulls around each island: a small flock.
static constexpr std::size_t GULLS_PER_ISLAND = 4u;
static constexpr std::size_t GULLS = 48u;
static constexpr float FISH_SEEN = 1.0f / 600.0f;

//! \brief A loop round an island: one radius per 1/64 turn.
static constexpr std::size_t LOOP = 64u;

//! \brief A turn about the vertical: the heading. Forward (-Z) goes to
//! (-sin, 0, -cos) of the angle.
static Quatf heading(float p_radians)
{
    return Quatf::fromAngleAxis(units::angle::radian_t(double(p_radians)),
                                Vector3f::UNIT_Y);
}

//! \brief A turn about a local axis, after the heading.
static Quatf about(float p_radians, Vector3f const& p_axis)
{
    return Quatf::fromAngleAxis(units::angle::radian_t(double(p_radians)),
                                p_axis);
}

//! \brief At least the size the eye needs from where it is.
static float seen(Vector3f const& p_at, Vector3f const& p_eye, float p_part)
{
    return std::max(1.0f, compages::vector::norm(p_at - p_eye) * p_part);
}

//! \brief How much larger than life a gull is drawn (see BIRD_NEAR and
//! GULL_TO_BOAT).
static float birdScale(Vector3f const& p_at, Vector3f const& p_eye)
{
    const float grown = std::pow(
        compages::vector::norm(p_at - p_eye) / BIRD_NEAR, BIRD_GROWTH);
    const float boat = seen(p_at, p_eye, BOAT_SEEN) * BOAT_LENGTH;
    return std::max(1.0f,
                    std::min(grown, GULL_TO_BOAT * boat / GULL_SPAN));
}

//! \brief Water at least \p p_depth deep, well inside the map.
static bool isSea(Patch const& p_base, Vector2f p_point, float p_depth)
{
    return (std::abs(p_point.x) < 0.48f * MAP) &&
           (std::abs(p_point.y) < 0.48f * MAP) &&
           (p_base.heightAt(p_point.x, p_point.y) < -p_depth);
}

// ----------------------------------------------------------------------------
void Wildlife::makeBoats(compages::renderer::Scene& p_scene)
{
    const compages::renderer::Look wood =
        compages::renderer::color(0.45f, 0.30f, 0.20f);
    const compages::renderer::Look white =
        compages::renderer::color(0.95f, 0.95f, 0.92f);
    for (int k = 0; k < 7; ++k)
    {
        Boat boat{};
        boat.root = p_scene.world().entity("Boat");
        p_scene.box("Hull", wood)
            .parent(boat.root)
            .position(0.0f, 0.6f, 0.0f)
            .scale(3.2f, 1.6f, 11.0f);
        p_scene.box("Cabin", white)
            .parent(boat.root)
            .position(0.0f, 1.8f, 1.8f)
            .scale(2.2f, 1.0f, 3.0f);
        p_scene.cylinder("Mast", wood)
            .parent(boat.root)
            .position(0.0f, 6.0f, -1.0f)
            .scale(0.3f, 9.0f, 0.3f);
        p_scene.box("Sail", white)
            .parent(boat.root)
            .position(0.0f, 6.0f, 0.6f)
            .scale(0.12f, 7.0f, 3.4f);
        m_boats.push_back(boat);
    }
}

// ----------------------------------------------------------------------------
void Wildlife::makeBirds(compages::renderer::Scene& p_scene)
{
    // A gull: a white body, grey wings, black tips. Seen from above, a grey
    // and black shape stands out over the sea and over the land, where an
    // all white bird was only a white dot.
    const compages::renderer::Look body =
        compages::renderer::color(0.95f, 0.95f, 0.97f);
    const compages::renderer::Look feathers =
        compages::renderer::color(0.58f, 0.61f, 0.66f);
    const compages::renderer::Look tips =
        compages::renderer::color(0.08f, 0.08f, 0.10f);
    for (std::size_t k = 0u; k < GULLS; ++k)
    {
        Bird bird{};
        bird.root = p_scene.world().entity("Gull");
        p_scene.box("Body", body)
            .parent(bird.root)
            .scale(0.25f, 0.22f, 0.8f);
        bird.left = p_scene.world()
                        .entity("Left wing")
                        .parent(bird.root)
                        .position(-0.12f, 0.05f, 0.0f);
        bird.right = p_scene.world()
                         .entity("Right wing")
                         .parent(bird.root)
                         .position(0.12f, 0.05f, 0.0f);
        for (const auto& [wing, side] :
             { std::pair{ bird.left, -1.0f }, std::pair{ bird.right, 1.0f } })
        {
            p_scene.box("Wing", feathers)
                .parent(wing)
                .position(0.35f * side, 0.0f, 0.0f)
                .scale(0.7f, 0.07f, 0.3f);
            p_scene.box("Tip", tips)
                .parent(wing)
                .position(0.85f * side, 0.0f, 0.0f)
                .scale(0.3f, 0.07f, 0.22f);
        }
        m_birds.push_back(bird);
    }
}

// ----------------------------------------------------------------------------
void Wildlife::makeFish(compages::renderer::Scene& p_scene)
{
    for (int k = 0; k < 24; ++k)
    {
        Fish fish{};
        // Seen through the water: a dark shape just under the surface.
        fish.shadow = p_scene.sphere(
            "Fish",
            compages::renderer::seeThrough(
                compages::renderer::unlit(0.03f, 0.12f, 0.16f), 0.45f));
        fish.shadow.get<compages::renderer::MeshRenderer>().flags =
            compages::renderer::RenderFlags::NoShadowCaster;
        // Out of the water: silver, and lit.
        fish.jumper = p_scene.sphere(
            "Jumping fish", compages::renderer::color(0.75f, 0.80f, 0.85f));
        m_fish.push_back(fish);
    }
}

// ----------------------------------------------------------------------------
//! \brief A loop round an island where the water is 8 m deep or more, a
//! little farther out, smoothed; empty if it would cross land.
// ----------------------------------------------------------------------------
static std::vector<float>
loopRound(Island const& p_island, Patch const& p_base, float p_margin)
{
    std::vector<float> radii(LOOP, 0.0f);
    auto way = [](std::size_t p_a)
    {
        const float angle = TURN * float(p_a) / float(LOOP);
        return Vector2f(std::cos(angle), std::sin(angle));
    };
    for (std::size_t a = 0u; a < LOOP; ++a)
    {
        float r = 0.3f * p_island.radius;
        while ((r < 2.5f * p_island.radius) &&
               !isSea(p_base, p_island.centre + (way(a) * r), 8.0f))
        {
            r += 40.0f;
        }
        radii[a] = r + p_margin;
    }
    for (int pass = 0; pass < 4; ++pass)
    {
        std::vector<float> smooth(radii);
        for (std::size_t a = 0u; a < LOOP; ++a)
        {
            smooth[a] =
                std::max(radii[a],
                         0.25f * (radii[(a + LOOP - 1u) % LOOP] +
                                  (2.0f * radii[a]) + radii[(a + 1u) % LOOP]));
        }
        radii = std::move(smooth);
    }
    for (std::size_t a = 0u; a < LOOP; ++a)
    {
        if (!isSea(p_base, p_island.centre + (way(a) * radii[a]), 4.0f))
        {
            return {};
        }
    }
    return radii;
}

// ----------------------------------------------------------------------------
void Wildlife::placeBoats(Land const& p_land,
                          Patch const& p_base,
                          Random& p_random)
{
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    for (Boat& boat : m_boats)
    {
        boat.radii.clear();
        for (int attempt = 0; (attempt < 12) && boat.radii.empty(); ++attempt)
        {
            Island const& island =
                p_land.islands[p_random() % p_land.islands.size()];
            boat.radii =
                loopRound(island, p_base, 150.0f + (350.0f * unit(p_random)));
            boat.centre = island.centre;
        }
        boat.angle = TURN * unit(p_random);
        // 5 to 9 m/s (10 to 17 knots), either way round.
        boat.speed = (5.0f + (4.0f * unit(p_random))) *
                     ((unit(p_random) < 0.5f) ? -1.0f : 1.0f);
        boat.root.position(0.0f, -100.0f, 0.0f); // under the sea until it sails
    }
}

// ----------------------------------------------------------------------------
void Wildlife::placeBirds(Land const& p_land, Random& p_random)
{
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    for (std::size_t k = 0u; k < m_birds.size(); ++k)
    {
        Bird& bird = m_birds[k];
        Island const& island =
            p_land.islands[(k / GULLS_PER_ISLAND) % p_land.islands.size()];
        // Along the coast: the circle goes over the shore, the beaches and
        // the sea near them, where gulls are.
        bird.centre =
            Vector3f(island.centre.x + (200.0f * (unit(p_random) - 0.5f)),
                     0.0f,
                     island.centre.y + (200.0f * (unit(p_random) - 0.5f)));
        bird.radius = (0.6f + (0.4f * unit(p_random))) * island.radius;
        bird.altitude = 30.0f + (50.0f * unit(p_random));
        bird.height = -1.0f;
        bird.angle = TURN * unit(p_random);
        bird.speed = 11.0f + (4.0f * unit(p_random)); // m/s, a gull
        bird.phase = TURN * unit(p_random);
    }
}

// ----------------------------------------------------------------------------
//! Schools in the lagoons, 2 to 10 m deep.
// ----------------------------------------------------------------------------
void Wildlife::placeFish(Patch const& p_base, Random& p_random)
{
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::vector<Vector2f> lagoons;
    for (int tries = 0; (tries < 20000) && (lagoons.size() < 6u); ++tries)
    {
        const Vector2f p((unit(p_random) - 0.5f) * MAP,
                         (unit(p_random) - 0.5f) * MAP);
        if (const float h = p_base.heightAt(p.x, p.y);
            (h < -2.0f) && (h > -10.0f))
        {
            lagoons.push_back(p);
        }
    }
    for (std::size_t k = 0u; k < m_fish.size(); ++k)
    {
        Fish& fish = m_fish[k];
        fish.centre = lagoons.empty() ? Vector2f(0.0f, 0.0f)
                                      : lagoons[k % lagoons.size()];
        fish.radius = 15.0f + (40.0f * unit(p_random));
        fish.angle = TURN * unit(p_random);
        fish.speed = 0.8f + (1.2f * unit(p_random)); // m/s
        fish.wait = 2.0f + (20.0f * unit(p_random));
        fish.jump = -1.0f;
    }
}

// ----------------------------------------------------------------------------
void Wildlife::place(compages::renderer::Scene& p_scene,
                     Land const& p_land,
                     Patch const& p_base,
                     std::uint32_t p_seed)
{
    if (m_boats.empty())
    {
        makeBoats(p_scene);
        makeBirds(p_scene);
        makeFish(p_scene);
    }
    // Animals of a procedural map, not a secret: any generator will do.
    Random random(p_seed + 1000u); // NOSONAR
    placeBoats(p_land, p_base, random);
    placeBirds(p_land, random);
    placeFish(p_base, random);
}

// ----------------------------------------------------------------------------
//! Along its loop, on the swell, the bow (-Z) toward where it goes; it rolls.
// ----------------------------------------------------------------------------
void Wildlife::sail(Boat& p_boat,
                    Patch const& p_base,
                    float p_dt,
                    float p_time,
                    Vector3f p_eye)
{
    if (p_boat.radii.empty())
    {
        return;
    }
    // The radius between the two samples of the loop around the angle.
    auto radius = [&p_boat](float p_angle)
    {
        const float at = (std::fmod(p_angle + TURN, TURN) / TURN) * float(LOOP);
        const float base = std::floor(at);
        const auto a = std::size_t(int(base)) % LOOP;
        return p_boat.radii[a] +
               ((p_boat.radii[(a + 1u) % LOOP] - p_boat.radii[a]) *
                (at - base));
    };
    auto pointAt = [&](float p_angle)
    {
        return p_boat.centre + (Vector2f(std::cos(p_angle), std::sin(p_angle)) *
                                radius(p_angle));
    };
    p_boat.angle = std::fmod(
        p_boat.angle + ((p_boat.speed * p_dt) / radius(p_boat.angle)) + TURN,
        TURN);
    const Vector2f here = pointAt(p_boat.angle);
    const Vector2f ahead =
        pointAt(p_boat.angle + ((p_boat.speed > 0.0f) ? 0.01f : -0.01f)) - here;
    const float depth = -p_base.heightAt(here.x, here.y);
    const Vector3f at(
        here.x, swellHeight(here.x, here.y, depth, p_time) - 0.3f, here.y);
    const float roll =
        0.06f * std::sin((1.3f * p_time) + (40.0f * p_boat.angle));
    p_boat.root.position(at)
        .rotation(heading(std::atan2(-ahead.x, -ahead.y)) *
                  about(roll, Vector3f(0.0f, 0.0f, 1.0f)))
        .scale(seen(at, p_eye, BOAT_SEEN));
}

// ----------------------------------------------------------------------------
//! Along the circle (it goes toward (-sin, cos) of its angle), leaning into
//! the turn; flaps for a while, then glides with the wings held a little up.
// ----------------------------------------------------------------------------
void Wildlife::fly(Bird& p_bird,
                   Patch const& p_base,
                   float p_dt,
                   float p_time,
                   Vector3f p_eye)
{
    const float x = p_bird.centre.x + (p_bird.radius * std::cos(p_bird.angle));
    const float z = p_bird.centre.z + (p_bird.radius * std::sin(p_bird.angle));
    // Low over what is under it: it rises over a hill and comes down over
    // the sea smoothly, but never closer than 10 m to the ground.
    const float under = std::max(0.0f, p_base.heightAt(x, z));
    const float wanted = under + p_bird.altitude +
                         (6.0f * std::sin((0.7f * p_time) + p_bird.phase));
    p_bird.height = (p_bird.height < 0.0f)
                        ? wanted
                        : p_bird.height + ((wanted - p_bird.height) *
                                           (1.0f - std::exp(-1.5f * p_dt)));
    p_bird.height = std::max(p_bird.height, under + 10.0f);
    const Vector3f at(x, p_bird.height, z);
    p_bird.root.position(at)
        .rotation(heading(HALF_TURN - p_bird.angle) *
                  about(0.35f, Vector3f(0.0f, 0.0f, 1.0f)))
        .scale(birdScale(at, p_eye));
    const float beat = std::sin((7.0f * p_time) + p_bird.phase);
    const float flapping =
        std::clamp(2.0f * std::sin((0.4f * p_time) + p_bird.phase), 0.0f, 1.0f);
    // Gliding, the wings stay raised (a gull's shallow V): from a camera low
    // over the sea, flat wings would only be a line.
    // The lean of the turn, the V and the beat add up on the high wing:
    // kept under about 60 degrees, or the gull looks like a post.
    const float lift = 0.25f + (0.45f * flapping * beat);
    p_bird.left.rotation(-lift, Vector3f(0.0f, 0.0f, 1.0f));
    p_bird.right.rotation(lift, Vector3f(0.0f, 0.0f, 1.0f));
}

// ----------------------------------------------------------------------------
//! A metre under the surface; a jump is a parabola a few times its length
//! high, nose up then down.
// ----------------------------------------------------------------------------
void Wildlife::swim(Fish& p_fish, float p_dt, Vector3f p_eye)
{
    p_fish.angle =
        std::fmod(p_fish.angle + ((p_fish.speed * p_dt) / p_fish.radius), TURN);
    const Vector2f here = p_fish.centre + (Vector2f(std::cos(p_fish.angle),
                                                    std::sin(p_fish.angle)) *
                                           p_fish.radius);
    const Vector3f at(here.x, -1.2f, here.y);
    const float size = seen(at, p_eye, FISH_SEEN);
    const Quatf way = heading(HALF_TURN - p_fish.angle);
    p_fish.shadow.position(at).rotation(way).scale(
        0.35f * size, 0.1f * size, size);
    p_fish.wait -= p_dt;
    if ((p_fish.wait <= 0.0f) && (p_fish.jump < 0.0f))
    {
        p_fish.jump = 0.0f;
    }
    if (p_fish.jump < 0.0f)
    {
        p_fish.jumper.position(here.x, -50.0f, here.y); // under the sea
        return;
    }
    p_fish.jump += p_dt / 0.9f;
    const float t = std::min(p_fish.jump, 1.0f);
    p_fish.jumper.position(here.x, 4.0f * size * t * (1.0f - t), here.y)
        .rotation(way * about(1.2f * (0.5f - t), Vector3f(1.0f, 0.0f, 0.0f)))
        .scale(0.3f * size, 0.25f * size, size);
    if (p_fish.jump >= 1.0f)
    {
        p_fish.jump = -1.0f;
        p_fish.wait = 6.0f + std::fmod(p_fish.angle * 97.0f, 20.0f);
    }
}

// ----------------------------------------------------------------------------
void Wildlife::animate(Patch const& p_base,
                       float p_dt,
                       float p_time,
                       Vector3f p_eye)
{
    for (Boat& boat : m_boats)
    {
        sail(boat, p_base, p_dt, p_time, p_eye);
    }
    for (Bird& bird : m_birds)
    {
        bird.angle =
            std::fmod(bird.angle + ((bird.speed * p_dt) / bird.radius), TURN);
        fly(bird, p_base, p_dt, p_time, p_eye);
    }
    for (Fish& fish : m_fish)
    {
        swim(fish, p_dt, p_eye);
    }
}

} // namespace examples::archipelago
