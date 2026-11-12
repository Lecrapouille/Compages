// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/10b_DoomLike.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Units.hpp"

#include <algorithm>
#include <cmath>

namespace examples
{

using namespace doom;

//! \brief One bullet a shot, barely spread; what it takes off a body (a
//! soldier has 6, a robot 9; twice as much in the head).
constexpr int PELLETS = 1;
constexpr float SPREAD = 0.004f;
constexpr float BULLET = 5.0f;
//! \brief Seconds of a shot (the kick, then the bolt worked back and
//! forth) and of a reload.
constexpr float FIRE_TIME = 1.0f;
constexpr float RELOAD_TIME = 1.9f;

//! \brief 0 before p_from, 1 after p_to, smooth between.
static float smoothstep(float p_from, float p_to, float p_x)
{
    const float t = std::clamp((p_x - p_from) / (p_to - p_from), 0.0f, 1.0f);
    return t * t * (3.0f - (2.0f * t));
}

//! \brief The length of a vector lying on the floor.
static float flatLength(compages::Vector3f const& p_v)
{
    return std::sqrt((p_v.x * p_v.x) + (p_v.z * p_v.z));
}

//! \brief How far along a ray it enters a standing cylinder, if it does.
static std::optional<float> enterCylinder(compages::Vector3f p_origin,
                                          compages::Vector3f p_direction,
                                          compages::Vector3f p_base,
                                          float p_radius,
                                          float p_height)
{
    const float ox = p_origin.x - p_base.x;
    const float oz = p_origin.z - p_base.z;
    const float a =
        (p_direction.x * p_direction.x) + (p_direction.z * p_direction.z);
    const float b = 2.0f * ((ox * p_direction.x) + (oz * p_direction.z));
    const float c = (ox * ox) + (oz * oz) - (p_radius * p_radius);
    const float discriminant = (b * b) - (4.0f * a * c);
    if ((a < 1.0e-6f) || (discriminant < 0.0f))
    {
        return std::nullopt;
    }
    const float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    const float y = p_origin.y + (p_direction.y * t) - p_base.y;
    if ((t < 0.0f) || (y < 0.0f) || (y > p_height))
    {
        return std::nullopt;
    }
    return t;
}

// ----------------------------------------------------------------------------
compages::Vector3f DoomLike::eye() const
{
    // On the floor when dead.
    return compages::Vector3f(m_player.position.x,
                                    (m_session.state == State::Dead) ? 0.4f
                                                                     : EYE,
                                    m_player.position.z);
}

// ----------------------------------------------------------------------------
compages::Vector3f DoomLike::aim() const
{
    // At a yaw of zero the eye looks down -Z; the pitch raises it.
    return compages::Vector3f(
        -std::sin(m_player.yaw) * std::cos(m_player.pitch),
        std::sin(m_player.pitch),
        -std::cos(m_player.yaw) * std::cos(m_player.pitch));
}

// ----------------------------------------------------------------------------
void DoomLike::movePlayer(compages::world::ViewFrame const& p_frame)
{
    compages::world::Input const& input = p_frame.input;
    const float dt = p_frame.elapsed;
    m_player.clock += dt;

    // Looking: the mouse alone once it is held by the picture, a right drag
    // otherwise, Q and E always.
    if (input.down(compages::world::Key::Q))
    {
        m_player.yaw += 2.2f * dt;
    }
    if (input.down(compages::world::Key::E))
    {
        m_player.yaw -= 2.2f * dt;
    }
    const float sensitivity =
        input.mouse_captured ? 0.0025f : (input.mouse_right ? 0.004f : 0.0f);
    m_player.yaw -= input.mouse_delta.x * sensitivity;
    m_player.pitch = std::clamp(
        m_player.pitch + (input.mouse_delta.y * sensitivity), -1.2f, 1.2f);

    // Walking on the floor whatever the pitch.
    const compages::Vector3f forward(
        -std::sin(m_player.yaw), 0.0f, -std::cos(m_player.yaw));
    const compages::Vector3f right(
        std::cos(m_player.yaw), 0.0f, -std::sin(m_player.yaw));
    compages::Vector3f walk(0.0f, 0.0f, 0.0f);
    if (input.down(compages::world::Key::W) ||
        input.down(compages::world::Key::Up))
    {
        walk += forward;
    }
    if (input.down(compages::world::Key::S) ||
        input.down(compages::world::Key::Down))
    {
        walk -= forward;
    }
    if (input.down(compages::world::Key::D) ||
        input.down(compages::world::Key::Right))
    {
        walk += right;
    }
    if (input.down(compages::world::Key::A) ||
        input.down(compages::world::Key::Left))
    {
        walk -= right;
    }
    const float length = flatLength(walk);
    m_player.walking = 0.0f;
    if (length > 1.0e-4f)
    {
        const float speed =
            input.down(compages::world::Key::Shift) ? 7.0f : 4.0f;
        m_player.position = m_content.map.slide(
            m_player.position, walk * (speed * dt / length), PLAYER_RADIUS);
        m_player.walking = speed / 4.0f;
        m_player.bob += dt * speed * 2.0f;
    }

    // Out through the green light.
    if (flatLength(m_player.position - m_cast.exit) < 1.2f)
    {
        if (m_session.level + 1u < levels().size())
        {
            const compages::Status built = buildLevel(m_session.level + 1u);
            if (!built)
            {
                compages::gpu::reportError(built.error());
            }
        }
        else
        {
            m_session.state = State::Won;
            m_session.banner = 0.0f;
        }
    }
}

// ----------------------------------------------------------------------------
void DoomLike::placeView(compages::world::ViewFrame const& p_frame)
{
    (void)p_frame;
    // The eye, shaken for a moment by a blast.
    compages::Vector3f at = eye();
    if (m_player.shake > 0.0f)
    {
        const float amount = m_player.shake * 0.12f;
        at +=
            compages::Vector3f(m_content.effects.random(-amount, amount),
                                     m_content.effects.random(-amount, amount),
                                     m_content.effects.random(-amount, amount));
    }
    m_rig.camera.position(at).rotation(
        compages::Quatf::fromAngleAxis(
            units::angle::radian_t(double(m_player.yaw)),
            compages::Vector3f(0.0f, 1.0f, 0.0f)) *
        compages::Quatf::fromAngleAxis(
            units::angle::radian_t(double(m_player.pitch)),
            compages::Vector3f(1.0f, 0.0f, 0.0f)));

    // The rifle low on the right, swinging with the steps. A shot kicks it
    // back and up, then the bolt slides back and forth (the empty case
    // flies out); a reload turns it on its side and down.
    const float swing = std::min(m_player.walking, 1.5f);
    const compages::Vector3f bob(std::sin(m_player.bob) * 0.012f * swing,
                                       -std::abs(std::cos(m_player.bob)) *
                                           0.012f * swing,
                                       0.0f);
    const float lowered = (m_session.state == State::Playing) ? 0.0f : -0.4f;
    const float t = m_player.gun_time;
    float kick = 0.0f;
    float bolt = 0.0f;
    float dip = 0.0f;
    float roll = 0.0f;
    if (m_player.gun == Gun::Firing)
    {
        kick = (t < 0.05f) ? (t / 0.05f) : std::exp(-(t - 0.05f) * 9.0f);
        bolt = smoothstep(0.35f, 0.55f, t) * (1.0f - smoothstep(0.65f, 0.9f, t));
        dip = 0.025f * bolt;
        roll = 0.25f * bolt;
    }
    else if (m_player.gun == Gun::Reloading)
    {
        const float u = t / RELOAD_TIME;
        const float e = std::sin(PI * std::clamp(u, 0.0f, 1.0f));
        bolt = smoothstep(0.1f, 0.25f, u) * (1.0f - smoothstep(0.75f, 0.9f, u));
        dip = 0.11f * e;
        roll = 0.7f * e;
        kick = -0.6f * e; // pitched down, not up
    }
    m_rig.weapon
        .position(compages::Vector3f(0.14f, -0.17f + lowered - dip, -0.22f + (0.06f * std::max(kick, 0.0f))) + bob)
        .rotation(compages::Quatf::fromAngleAxis(units::angle::radian_t(double(0.14f * kick)),
                                                 compages::Vector3f(1.0f, 0.0f, 0.0f)) *
                  compages::Quatf::fromAngleAxis(units::angle::radian_t(double(roll)),
                                                 compages::Vector3f(0.0f, 0.0f, 1.0f)));
    // The bolt slides toward the stock (-x in the file of the rifle).
    if (m_rig.bolt_a)
    {
        m_rig.bolt_a.position(m_rig.bolt_a_rest - compages::Vector3f(0.09f * bolt, 0.0f, 0.0f));
    }
    if (m_rig.bolt_b)
    {
        m_rig.bolt_b.position(m_rig.bolt_b_rest - compages::Vector3f(0.09f * bolt, 0.0f, 0.0f));
    }
}

// ----------------------------------------------------------------------------
void DoomLike::useWeapon(compages::world::ViewFrame const& p_frame)
{
    compages::world::Input const& input = p_frame.input;
    m_player.gun_time += p_frame.elapsed;

    // A shot, then a reload, last a fixed time.
    if (m_player.gun == Gun::Firing)
    {
        if (!m_player.casing_thrown && (m_player.gun_time > 0.5f) &&
            bool(m_rig.muzzle))
        {
            // The bolt, pulled back, throws the empty case out, to the right.
            m_player.casing_thrown = true;
            const compages::Vector3f right(
                std::cos(m_player.yaw), 0.0f, -std::sin(m_player.yaw));
            m_content.effects.casing(
                m_rig.weapon.worldPosition() + (aim() * 0.15f), right);
        }
        if (m_player.gun_time >= FIRE_TIME)
        {
            m_player.gun = Gun::Idle;
        }
    }
    else if ((m_player.gun == Gun::Reloading) && (m_player.gun_time >= RELOAD_TIME))
    {
        const int added = std::min(LOAD - m_player.shells, m_player.reserve);
        m_player.shells += added;
        m_player.reserve -= added;
        m_player.gun = Gun::Idle;
    }

    if (compages::world::pressed(input, m_ui.previous, compages::world::Key::R))
    {
        reload();
    }

    // A click fires; held, the gun fires again each time it is ready.
    if (input.mouse_left_pressed)
    {
        m_player.trigger = true;
    }
    if (!input.mouse_left)
    {
        m_player.trigger = false;
    }
    if ((input.mouse_left_pressed || m_player.trigger) &&
        (m_player.gun == Gun::Idle))
    {
        if (m_player.shells > 0)
        {
            fire();
        }
        else if (m_player.reserve > 0)
        {
            reload();
        }
        else if (input.mouse_left_pressed)
        {
            say("Out of rounds");
        }
    }
}

// ----------------------------------------------------------------------------
void DoomLike::fire()
{
    --m_player.shells;
    m_player.gun = Gun::Firing;
    m_player.gun_time = 0.0f;
    m_player.casing_thrown = false;
    m_player.flash = 0.07f;

    // The pellets leave the eye, where the crosshair is; their streaks leave
    // the muzzle, where the eye expects them.
    const compages::Vector3f from = eye();
    const compages::Vector3f forward = aim();
    const compages::Vector3f muzzle = bool(m_rig.muzzle)
                                                ? m_rig.muzzle.worldPosition()
                                                : (from + (forward * 0.6f));
    m_content.effects.muzzle(muzzle, forward);
    compages::Vector3f right = compages::vector::cross(
        forward, compages::Vector3f(0.0f, 1.0f, 0.0f));
    right.normalize();
    const compages::Vector3f up =
        compages::vector::cross(right, forward);

    for (int i = 0; i < PELLETS; ++i)
    {
        compages::Vector3f direction =
            forward + (right * m_content.effects.random(-SPREAD, SPREAD)) +
            (up * m_content.effects.random(-SPREAD, SPREAD));
        direction.normalize();
        const std::optional<Target> target = shootRay(from, direction);
        const compages::Vector3f end =
            target ? target->point : (from + (direction * 40.0f));
        m_content.effects.tracer(
            muzzle, end, compages::Vector3f(1.0f, 0.85f, 0.45f), 0.06f);
        if (!target)
        {
            continue;
        }
        if (target->enemy != nullptr)
        {
            damageEnemy(*target->enemy, BULLET, target->point, direction);
        }
        else if (target->barrel != nullptr)
        {
            m_content.effects.sparks(target->point, target->normal, 5);
            target->barrel->health -= BULLET;
            if (target->barrel->health <= 0.0f)
            {
                explode(*target->barrel);
            }
        }
        else
        {
            // A hole where it landed, and what flies off the stone.
            m_content.effects.mark(Mark::Hole,
                                   target->point,
                                   target->normal,
                                   m_content.effects.random(0.06f, 0.1f));
            m_content.effects.sparks(target->point, target->normal, 3);
        }
    }
}

// ----------------------------------------------------------------------------
std::optional<DoomLike::Target>
// ----------------------------------------------------------------------------
DoomLike::shootRay(compages::Vector3f p_origin,
                   compages::Vector3f p_direction)
{
    // The nearest of the wall, the enemies standing and the barrels.
    std::optional<Target> best;
    if (const std::optional<Hit> wall =
            m_content.map.cast(p_origin, p_direction, 40.0f))
    {
        best = Target{ wall->distance, wall->point, wall->normal };
    }
    auto closer = [&best](float t) { return !best || (t < best->distance); };
    for (Enemy& enemy : m_cast.enemies)
    {
        if (enemy.mood == Mood::Dead)
        {
            continue;
        }
        const bool soldier = (enemy.kind == Kind::Soldier);
        const std::optional<float> t =
            enterCylinder(p_origin,
                          p_direction,
                          enemy.root.position(),
                          soldier ? SOLDIER_RADIUS : ROBOT_RADIUS,
                          soldier ? SOLDIER_HEIGHT : ROBOT_HEIGHT);
        if (t && closer(*t))
        {
            best = Target{
                *t, p_origin + (p_direction * *t), p_direction * -1.0f, &enemy
            };
        }
    }
    for (Barrel& barrel : m_cast.barrels)
    {
        if (barrel.gone)
        {
            continue;
        }
        const std::optional<float> t =
            enterCylinder(p_origin, p_direction, barrel.at, 0.4f, 1.1f);
        if (t && closer(*t))
        {
            best = Target{ *t,
                           p_origin + (p_direction * *t),
                           p_direction * -1.0f,
                           nullptr,
                           &barrel };
        }
    }
    return best;
}

// ----------------------------------------------------------------------------
void DoomLike::reload()
{
    if ((m_player.gun != Gun::Idle) || (m_player.shells >= LOAD) ||
        (m_player.reserve <= 0))
    {
        return;
    }
    m_player.gun = Gun::Reloading;
    m_player.gun_time = 0.0f;
}

// ----------------------------------------------------------------------------
std::optional<std::size_t> DoomLike::torchAimedAt() const
{
    // Within reach, near the line of the crosshair, not behind a wall.
    std::optional<std::size_t> chosen;
    float nearest = 0.8f;
    const compages::Vector3f from = eye();
    const compages::Vector3f forward = aim();
    for (std::size_t i = 0u; i < m_cast.torches.size(); ++i)
    {
        const compages::Vector3f to = m_cast.torches[i].at - from;
        const float along = compages::vector::dot(to, forward);
        if ((along < 0.0f) || (along > 4.5f))
        {
            continue;
        }
        const float off = (to - (forward * along)).norm();
        if ((off < nearest) &&
            m_content.map.clearLine(m_player.position, m_cast.torches[i].at))
        {
            nearest = off;
            chosen = i;
        }
    }
    return chosen;
}

// ----------------------------------------------------------------------------
void DoomLike::useTorch()
{
    const std::optional<std::size_t> index = torchAimedAt();
    if (!index)
    {
        return;
    }
    Torch& torch = m_cast.torches[*index];
    torch.lit = !torch.lit;
    torch.light.enable(torch.lit);
    torch.flame.enable(torch.lit);
    m_content.effects.sparks(torch.at,
                             compages::Vector3f(0.0f, 1.0f, 0.0f),
                             torch.lit ? 12 : 2);
    say(torch.lit ? "Torch lit" : "Torch put out");
}

// ----------------------------------------------------------------------------
void DoomLike::pickUp(float p_dt)
{
    (void)p_dt;
    for (Pickup& pickup : m_cast.pickups)
    {
        if (pickup.taken)
        {
            continue;
        }
        // Turning and floating, so that it is seen from afar.
        pickup.body
            .position(pickup.at.x,
                      0.25f + (0.08f *
                              std::sin((m_player.clock * 2.5f) + pickup.at.x)),
                      pickup.at.z)
            .rotation(m_player.clock * 1.5f,
                      compages::Vector3f(0.0f, 1.0f, 0.0f));
        if (flatLength(m_player.position - pickup.at) > 1.0f)
        {
            continue;
        }
        if (pickup.kind == 'A')
        {
            if (m_player.reserve >= CARRIED)
            {
                continue;
            }
            m_player.reserve = std::min(CARRIED, m_player.reserve + LOAD);
            say("+" + std::to_string(LOAD) + " rounds");
        }
        else
        {
            if (m_player.health >= 100.0f)
            {
                continue;
            }
            m_player.health = std::min(100.0f, m_player.health + 40.0f);
            say("+40 health");
        }
        pickup.taken = true;
        pickup.body.enable(false);
    }
}

} // namespace examples
