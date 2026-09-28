// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "50_Complete/53_DoomLike.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Units.hpp"
#include "Compages/Renderer/Systems/AnimationSystem.hpp"

#include <algorithm>
#include <cmath>

namespace examples
{

using namespace doom;

//! \brief A turn around an axis, in radians.
static Quatf turn(float p_radians, Vector3f const& p_axis)
{
    return Quatf::fromAngleAxis(units::angle::radian_t(double(p_radians)), p_axis);
}

//! \brief The length of a vector lying on the floor.
static float flatLength(Vector3f const& p_v)
{
    return std::sqrt((p_v.x * p_v.x) + (p_v.z * p_v.z));
}

//! \brief The shortest turn bringing +Y onto \c p_to, of length one.
static Quatf fromUp(Vector3f const& p_to)
{
    const Vector3f up(0.0f, 1.0f, 0.0f);
    const float cosine = std::clamp(compages::vector::dot(up, p_to), -1.0f, 1.0f);
    Vector3f axis = compages::vector::cross(up, p_to);
    if (axis.norm() < 1.0e-5f)
    {
        return (cosine > 0.0f) ? Quatf() : turn(PI, Vector3f(1.0f, 0.0f, 0.0f));
    }
    axis.normalize();
    return turn(std::acos(cosine), axis);
}

void DoomLike::moveEnemies(float p_dt)
{
    for (Enemy& enemy : m_cast.enemies)
    {
        const bool soldier = (enemy.kind == Kind::Soldier);
        const Vector3f position = enemy.root.position();
        // The soldier model looks down -Z, the robot down +Z.
        const float facing = soldier ? PI : 0.0f;

        if (enemy.mood == Mood::Dead)
        {
            // A robot has a clip to die with; a soldier tips over backward.
            enemy.action += p_dt;
            if (soldier)
            {
                const float tilt = std::min(enemy.action * 3.0f, PI * 0.5f);
                enemy.root.rotation(turn(std::atan2(enemy.heading.x, enemy.heading.z) + facing,
                                         Vector3f(0.0f, 1.0f, 0.0f)) *
                                    turn(tilt, Vector3f(1.0f, 0.0f, 0.0f)));
            }
            continue;
        }

        const Vector3f to_player = m_player.position - position;
        const float distance = flatLength(to_player);
        const bool sees = (distance < 22.0f) && m_content.map.clearLine(position, m_player.position);
        const Vector3f toward = to_player * (1.0f / std::max(distance, 1.0e-3f));
        float speed = 0.0f;

        if (enemy.mood == Mood::Hurt)
        {
            // Staggered for a moment, then after whoever did it.
            enemy.action += p_dt;
            enemy.heading = toward;
            if (enemy.action > 0.3f)
            {
                enemy.mood = Mood::Chase;
            }
        }
        else if (soldier)
        {
            if (sees && (distance < 15.0f))
            {
                // Stop, face, aim (the arms are turned in aimArms()), shoot.
                if (enemy.mood != Mood::Attack)
                {
                    enemy.cooldown = std::max(enemy.cooldown, 0.6f);
                }
                enemy.mood = Mood::Attack;
                enemy.heading = toward;
                playLoop(enemy.root.id(), "Idle");
                enemy.cooldown -= p_dt;
                if ((enemy.cooldown <= 0.0f) && bool(enemy.muzzle))
                {
                    enemy.cooldown = m_content.effects.random(1.1f, 1.9f);
                    const Vector3f from = enemy.muzzle.worldPosition();
                    const Vector3f target = eye() + Vector3f(m_content.effects.random(-0.4f, 0.4f),
                                                             m_content.effects.random(-0.5f, 0.1f),
                                                             m_content.effects.random(-0.4f, 0.4f));
                    Vector3f direction = target - from;
                    direction.normalize();
                    m_cast.bolts.emplace_back(Bolt{ from, direction * 16.0f, 2.5f });
                    m_content.effects.muzzle(from, direction);
                    playOnce(enemy.gun.id(), "Fire");
                }
            }
            else if (sees)
            {
                enemy.mood = Mood::Chase;
                enemy.heading = toward;
                speed = 2.8f;
                playLoop(enemy.root.id(), "Run");
            }
            else
            {
                enemy.mood = Mood::Wander;
                speed = 1.1f;
                playLoop(enemy.root.id(), "Walk");
            }
        }
        else if (enemy.mood == Mood::Attack)
        {
            // The punch lands halfway through its clip, if the player is
            // still at arm's length.
            enemy.action += p_dt;
            enemy.heading = toward;
            const float length = clipLength(enemy.root.id(), "Punch");
            if (!enemy.struck && (enemy.action >= length * 0.45f))
            {
                enemy.struck = true;
                if (distance < 2.3f)
                {
                    hurtPlayer(20.0f);
                }
            }
            if (enemy.action >= length)
            {
                enemy.mood = Mood::Chase;
            }
        }
        else if (sees && (distance < 1.7f))
        {
            enemy.mood = Mood::Attack;
            enemy.action = 0.0f;
            enemy.struck = false;
            enemy.heading = toward;
            playOnce(enemy.root.id(), "Punch");
        }
        else if (sees)
        {
            enemy.mood = Mood::Chase;
            enemy.heading = toward;
            speed = (distance > 1.3f) ? 3.6f : 0.0f;
            playLoop(enemy.root.id(), "Running");
        }
        else
        {
            enemy.mood = Mood::Wander;
            speed = 1.2f;
            playLoop(enemy.root.id(), "Walking");
        }

        // Wandering: straight on, turning at the walls and now and then.
        const Vector3f step = enemy.heading * (speed * p_dt);
        const Vector3f moved = m_content.map.slide(position, step, soldier ? SOLDIER_RADIUS : ROBOT_RADIUS);
        const bool stuck = (speed > 0.0f) && (flatLength(moved - position) < speed * p_dt * 0.5f);
        enemy.wander -= p_dt;
        if ((enemy.mood == Mood::Wander) && (stuck || (enemy.wander < 0.0f)))
        {
            const float angle = float(int(m_content.effects.random(0.0f, 3.99f))) * (PI * 0.5f);
            enemy.heading = Vector3f(std::sin(angle), 0.0f, std::cos(angle));
            enemy.wander = m_content.effects.random(2.0f, 5.0f);
        }
        enemy.root.position(moved).rotation(
            turn(std::atan2(enemy.heading.x, enemy.heading.z) + facing, Vector3f(0.0f, 1.0f, 0.0f)));
    }
}

void DoomLike::aimArms()
{
    // After the clips: the arms of the soldiers aiming are pointed at the
    // player, the forearms straight, and the skins posed again. The bones
    // of the model run along their own +Y, so each arm is given the turn
    // bringing +Y onto the direction wanted, in the space of its parent.
    bool turned = false;
    const Vector3f target = eye() - Vector3f(0.0f, 0.2f, 0.0f);
    for (Enemy& enemy : m_cast.enemies)
    {
        if ((enemy.kind != Kind::Soldier) || (enemy.mood != Mood::Attack) ||
            !enemy.right_arm.valid() || !enemy.left_arm.valid())
        {
            continue;
        }
        const Vector3f right = compages::vector::cross(enemy.heading, Vector3f(0.0f, 1.0f, 0.0f));
        for (int side = 0; side < 2; ++side)
        {
            const compages::world::EntityId arm = (side == 0) ? enemy.right_arm : enemy.left_arm;
            const compages::world::EntityId forearm = (side == 0) ? enemy.right_forearm : enemy.left_forearm;
            Matrix44f const& shoulder = m_level_scene.world->worldMatrix(arm);
            Vector3f direction = target - Vector3f(shoulder[3].x, shoulder[3].y, shoulder[3].z);
            direction.normalize();
            if (side == 1)
            {
                // The left hand reaches across, under the barrel.
                direction = direction + (right * 0.35f);
                direction.normalize();
            }
            Matrix44f const& parent = m_level_scene.world->worldMatrix(m_level_scene.world->parent(arm));
            Vector3f local;
            for (int axis = 0; axis < 3; ++axis)
            {
                Vector3f row(parent[std::size_t(axis)].x, parent[std::size_t(axis)].y,
                             parent[std::size_t(axis)].z);
                row.normalize();
                local[std::size_t(axis)] = compages::vector::dot(direction, row);
            }
            local.normalize();
            compages::world::Entity(*m_level_scene.world, arm).rotation(fromUp(local));
            if (forearm.valid())
            {
                compages::world::Entity(*m_level_scene.world, forearm).rotation(Quatf());
            }
            turned = true;
        }
    }
    if (turned)
    {
        m_level_scene.world->update();
        compages::gpu::check(compages::renderer::AnimationSystem::pose(*m_level_scene.world, m_content.assets));
    }
}

void DoomLike::moveBolts(float p_dt)
{
    for (Bolt& bolt : m_cast.bolts)
    {
        const Vector3f from = bolt.position;
        bolt.position = bolt.position + (bolt.velocity * p_dt);
        bolt.life -= p_dt;
        Vector3f direction = bolt.velocity;
        direction.normalize();

        // What it looks like: a hot ball and a short streak behind it.
        m_content.effects.glow(bolt.position, Vector3f(1.0f, 0.45f, 0.1f), 0.18f);
        m_content.effects.tracer(bolt.position - (direction * 0.6f), bolt.position,
                         Vector3f(1.0f, 0.5f, 0.15f), 0.02f);

        // The player, a barrel, or the stone.
        const Vector3f to_player = bolt.position - m_player.position;
        if ((flatLength(to_player) < PLAYER_RADIUS + 0.1f) && (bolt.position.y < EYE + 0.2f))
        {
            hurtPlayer(9.0f);
            bolt.life = 0.0f;
            continue;
        }
        for (Barrel& barrel : m_cast.barrels)
        {
            if (!barrel.gone && (flatLength(bolt.position - barrel.at) < 0.45f) && (bolt.position.y < 1.1f))
            {
                bolt.life = 0.0f;
                barrel.health -= 1.0f;
                if (barrel.health <= 0.0f)
                {
                    explode(barrel);
                }
            }
        }
        if (bolt.life <= 0.0f)
        {
            continue;
        }
        const float step = (bolt.position - from).norm();
        if (const std::optional<Hit> hit = m_content.map.cast(from, direction, step + 0.05f))
        {
            m_content.effects.sparks(hit->point, hit->normal, 8);
            m_content.effects.mark(Mark::Scorch, hit->point, hit->normal, 0.18f);
            bolt.life = 0.0f;
        }
    }
    std::erase_if(m_cast.bolts, [](Bolt const& b) { return b.life <= 0.0f; });
}

void DoomLike::moveBarrels(float p_dt)
{
    // A barrel caught in a blast goes off a moment later: a chain.
    for (Barrel& barrel : m_cast.barrels)
    {
        if (!barrel.gone && (barrel.fuse >= 0.0f))
        {
            barrel.fuse -= p_dt;
            if (barrel.fuse < 0.0f)
            {
                explode(barrel);
            }
        }
    }
}

void DoomLike::damageEnemy(Enemy& p_enemy, float p_damage, Vector3f p_point, Vector3f p_direction)
{
    if (p_enemy.mood == Mood::Dead)
    {
        return;
    }
    const bool soldier = (p_enemy.kind == Kind::Soldier);
    float damage = p_damage;
    if (soldier)
    {
        // Blood out of both sides, a splash on the body where the pellet went
        // in, and on the wall behind if there is one near.
        damage *= (p_point.y > 1.5f) ? 2.0f : 1.0f;
        m_content.effects.blood(p_point, p_direction, 8);
        m_content.effects.blood(p_point, p_direction * -1.0f, 3);
        m_content.effects.markBody(Mark::Blood, p_enemy.root.id(), p_point, m_content.effects.random(0.05f, 0.09f));
        if (const std::optional<Hit> wall = m_content.map.cast(p_point, p_direction, 2.5f))
        {
            m_content.effects.mark(Mark::Blood, wall->point, wall->normal, m_content.effects.random(0.4f, 0.8f));
        }
    }
    else
    {
        // A robot bleeds oil and sparks.
        m_content.effects.sparks(p_point, p_direction * -1.0f, 6);
        m_content.effects.markBody(Mark::Oil, p_enemy.root.id(), p_point, m_content.effects.random(0.05f, 0.08f));
    }

    p_enemy.health -= damage;
    const Vector3f position = p_enemy.root.position();
    if (p_enemy.health <= 0.0f)
    {
        p_enemy.mood = Mood::Dead;
        p_enemy.action = 0.0f;
        ++m_session.kills;
        m_content.effects.pool(soldier ? Mark::Blood : Mark::Oil, position, soldier ? 1.4f : 1.1f);
        if (soldier)
        {
            playLoop(p_enemy.root.id(), "Idle");
            say("Soldier down");
        }
        else
        {
            playOnce(p_enemy.root.id(), "Death");
            say("Robot down");
        }
        return;
    }

    // Pushed back a step, staggered, and now after the player.
    const Vector3f push(p_direction.x, 0.0f, p_direction.z);
    p_enemy.root.position(m_content.map.slide(position, push * 0.12f, soldier ? SOLDIER_RADIUS : ROBOT_RADIUS));
    if (p_enemy.mood != Mood::Attack)
    {
        p_enemy.mood = Mood::Hurt;
        p_enemy.action = 0.0f;
    }
}

void DoomLike::explode(Barrel& p_barrel)
{
    if (p_barrel.gone)
    {
        return;
    }
    p_barrel.gone = true;
    p_barrel.body.enable(false);
    const Vector3f middle = p_barrel.at + Vector3f(0.0f, 0.6f, 0.0f);

    // Fire, smoke, a flash of light, a black mark on the floor and on the
    // walls it reaches.
    m_content.effects.explosion(middle);
    m_cast.blasts.emplace_back(Blast{
        m_level_scene.scene->lamp("Blast", Vector3f(1.0f, 0.6f, 0.25f), 9.0f, 12.0f).position(middle + Vector3f(0.0f, 0.6f, 0.0f)) });
    m_content.effects.mark(Mark::Scorch, Vector3f(p_barrel.at.x, 0.0f, p_barrel.at.z), Vector3f(0.0f, 1.0f, 0.0f), 3.2f);
    for (int i = 0; i < 8; ++i)
    {
        const float angle = float(i) * PI * 0.25f;
        const Vector3f direction(std::sin(angle), 0.0f, std::cos(angle));
        if (const std::optional<Hit> wall = m_content.map.cast(middle, direction, 2.5f))
        {
            m_content.effects.mark(Mark::Scorch, wall->point, wall->normal, 2.0f);
        }
    }

    // What is near is hurt, the nearer the more; barrels near go off next.
    constexpr float REACH = 5.0f;
    for (Enemy& enemy : m_cast.enemies)
    {
        const Vector3f to = enemy.root.position() - p_barrel.at;
        const float d = flatLength(to);
        if ((d < REACH) && (enemy.mood != Mood::Dead) && m_content.map.clearLine(p_barrel.at, enemy.root.position()))
        {
            Vector3f direction(to.x, 0.3f, to.z);
            direction.normalize();
            damageEnemy(enemy, 14.0f * (1.0f - (d / REACH)) + 1.0f,
                        enemy.root.position() + Vector3f(0.0f, 1.0f, 0.0f), direction);
        }
    }
    const float d = flatLength(m_player.position - p_barrel.at);
    if ((d < REACH) && m_content.map.clearLine(p_barrel.at, m_player.position))
    {
        hurtPlayer(60.0f * (1.0f - (d / REACH)));
    }
    m_player.shake = std::max(m_player.shake, 0.7f * std::max(0.0f, 1.0f - (d / 12.0f)));
    for (Barrel& other : m_cast.barrels)
    {
        if (!other.gone && (other.fuse < 0.0f) && (flatLength(other.at - p_barrel.at) < 3.5f))
        {
            other.fuse = 0.25f;
        }
    }
}

void DoomLike::hurtPlayer(float p_damage)
{
    if (m_session.state != State::Playing)
    {
        return;
    }
    m_player.health -= p_damage;
    m_player.hurt = 0.4f;
    m_player.shake = std::max(m_player.shake, 0.15f);
}

} // namespace examples
