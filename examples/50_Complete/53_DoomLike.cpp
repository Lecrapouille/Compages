// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "50_Complete/53_DoomLike.hpp"
#include "Compages/World/Entity.hpp"

#include "Common/DataPath.hpp"
#include "Common/Gui.hpp"
#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Units.hpp"
#include "Compages/Renderer/Components/Animator.hpp"

#include <algorithm>
#include <cmath>

namespace examples
{

using namespace units::literals;

using doom::CELL;
using doom::PI;
using doom::ROBOT_SCALE;
using doom::WALL_HEIGHT;

//! \brief Makes the light of a torch waver, the way a flame does.
struct DoomLike::Flicker: compages::world::Behavior
{
    explicit Flicker(float p_phase) : phase(p_phase) {}

    void update(float) override
    {
        // Two sines of unrelated speeds: never quite the same twice.
        const float t = frame().total + phase;
        compages::world::PointLight& light =
            entity().get<compages::world::PointLight>();
        light.intensity =
            3.0f + (0.35f * std::sin(t * 13.0f)) + (0.25f * std::sin(t * 7.3f));
    }

    float phase;
};

std::string DoomLike::description() const
{
    return "A small Doom-like in two levels, built from everything the engine "
           "does: textured walls, flickering torches, fog, glTF soldiers that "
           "aim and shoot, glTF robots that run and punch, an animated glTF "
           "shotgun, particles of blood, fire and smoke, marks on the walls "
           "and "
           "on the bodies, explosive barrels. Reach the green light to leave a "
           "level.\n\n"
           "Click the picture: the mouse then looks around (Escape gives it "
           "back). W A S D or the arrows walk, Shift runs, Q and E turn, a "
           "left "
           "click shoots, R reloads, F puts out or lights the torch aimed at, "
           "L switches the lantern. Red boxes are shells, white boxes heal.";
}

compages::Status DoomLike::setUp()
{
    // Loaded once for both levels: each Scene built later shares them.
    auto need = [](char const* p_file) -> compages::Result<std::string>
    {
        const std::string path = dataPath(p_file);
        if (path.empty())
        {
            return compages::failure(
                std::string(p_file) +
                " is missing: see external/ "
                "(Shotgun.glb is written by "
                "examples/50_Complete/DoomLike/make_shotgun.py)");
        }
        return path;
    };
    std::string soldier;
    COMPAGES_TRY_ASSIGN(soldier, need("Soldier.glb"));
    std::string robot;
    COMPAGES_TRY_ASSIGN(robot, need("RobotExpressive.glb"));
    std::string shotgun;
    COMPAGES_TRY_ASSIGN(shotgun, need("Shotgun.glb"));
    COMPAGES_TRY_ASSIGN(m_content.soldier_prefab,
                        m_content.assets.load(soldier));
    COMPAGES_TRY_ASSIGN(m_content.robot_prefab, m_content.assets.load(robot));
    COMPAGES_TRY_ASSIGN(m_content.shotgun_prefab,
                        m_content.assets.load(shotgun));
    COMPAGES_TRY(m_content.effects.setUp());
    return buildLevel(0u);
}

compages::Status DoomLike::buildLevel(std::size_t p_level)
{
    doom::LevelData const& level = doom::levels()[p_level];
    const std::string wall_file = dataPath(level.wall);
    const std::string floor_file = dataPath(level.floor);
    const std::string crate_file = dataPath("wooden-crate.jpg");
    const std::string hazard_file = dataPath("hazard.png");
    if (wall_file.empty() || floor_file.empty() || crate_file.empty() ||
        hazard_file.empty())
    {
        return compages::failure(
            "53_DoomLike needs the textures of external/Compages-data/");
    }

    beginLevel(p_level);
    configureAtmosphere(level);
    COMPAGES_TRY(setupPlayerRig());
    COMPAGES_TRY(
        scanMapGrid(level, wall_file, floor_file, crate_file, hazard_file));
    return m_level_scene.scene->prepare();
}

void DoomLike::beginLevel(std::size_t p_level)
{
    doom::LevelData const& level = doom::levels()[p_level];
    m_level_scene.scene.reset();
    m_level_scene.world = std::make_unique<compages::world::World>();
    m_level_scene.scene = std::make_unique<compages::renderer::Scene>(
        *m_level_scene.world, m_content.assets);
    m_session.level = p_level;
    m_content.map.load(level.rows);
    m_cast.enemies.clear();
    m_cast.torches.clear();
    m_cast.barrels.clear();
    m_cast.pickups.clear();
    m_cast.bolts.clear();
    m_cast.blasts.clear();
    m_session.state = State::Playing;
    m_player.health = 100.0f;
    m_player.shells = 8;
    m_player.reserve = std::max(m_player.reserve, 16);
    m_player.gun = Gun::Idle;
    m_player.hurt = 0.0f;
    m_player.shake = 0.0f;
    m_session.banner = 2.5f;
    m_player.clock = 0.0f;
    m_content.effects.reset(*m_level_scene.scene);
}

void DoomLike::configureAtmosphere(doom::LevelData const& p_level)
{
    m_level_scene.scene->background(
        p_level.fog.x, p_level.fog.y, p_level.fog.z);
    m_level_scene.scene->environment().default_light_color =
        compages::core::Vector3f(0.0f, 0.0f, 0.0f);
    m_level_scene.scene->environment().fog_color = p_level.fog;
    m_level_scene.scene->environment().fog_density = 0.055f;
}

compages::Status DoomLike::setupPlayerRig()
{
    m_rig.camera = m_level_scene.scene->camera("Player");
    m_rig.camera.get<compages::world::Camera>().fov = 75.0_deg;
    m_rig.camera.get<compages::world::Camera>().near_plane = 0.05f;
    m_rig.camera.get<compages::world::Camera>().far_plane = 80.0f;
    m_rig.lantern = m_level_scene.scene->lamp(
        "Lantern", compages::core::Vector3f(1.0f, 0.85f, 0.6f), 1.6f, 9.0f);
    m_rig.lantern.parent(m_rig.camera)
        .position(0.3f, -0.2f, 0.0f)
        .enable(m_player.lantern_on);
    auto weapon = m_level_scene.scene->instantiate(m_content.shotgun_prefab,
                                                   m_rig.camera.id());
    if (!weapon)
    {
        return compages::failure(weapon.error());
    }
    m_rig.weapon = weapon.value();
    m_rig.weapon.scale(0.7f);
    m_rig.muzzle = compages::world::Entity(
        *m_level_scene.world, findNamed(m_rig.weapon.id(), "Muzzle"));
    playLoop(m_rig.weapon.id(), "Idle");
    return compages::success();
}

compages::Status DoomLike::scanMapGrid(doom::LevelData const& p_level,
                                       std::string const& p_wall_file,
                                       std::string const& p_floor_file,
                                       std::string const& p_crate_file,
                                       std::string const& p_hazard_file)
{
    compages::renderer::Look wall = compages::renderer::texture(p_wall_file);
    wall.color = p_level.wall_tint;
    compages::renderer::Look ceiling = compages::renderer::texture(p_wall_file);
    ceiling.color = p_level.wall_tint * 0.35f;
    compages::renderer::Look floor = compages::renderer::texture(p_floor_file);
    floor.color = compages::core::Vector3f(0.75f, 0.7f, 0.7f);
    const compages::renderer::Look crate =
        compages::renderer::texture(p_crate_file);
    const compages::renderer::Look hazard =
        compages::renderer::texture(p_hazard_file);

    std::vector<std::string> const& rows = m_content.map.rows();
    for (std::size_t row = 0u; row < rows.size(); ++row)
    {
        for (std::size_t column = 0u; column < rows[row].size(); ++column)
        {
            COMPAGES_TRY(placeCell(rows[row][column],
                                   column,
                                   row,
                                   wall,
                                   ceiling,
                                   floor,
                                   crate,
                                   hazard));
        }
    }
    return compages::success();
}

compages::Status DoomLike::placeCell(char p_cell,
                                     std::size_t p_column,
                                     std::size_t p_row,
                                     compages::renderer::Look const& p_wall,
                                     compages::renderer::Look const& p_ceiling,
                                     compages::renderer::Look const& p_floor,
                                     compages::renderer::Look const& p_crate,
                                     compages::renderer::Look const& p_hazard)
{
    const compages::core::Vector3f at = doom::Map::center(p_column, p_row);
    if (p_cell == '#')
    {
        m_level_scene.scene->box("Wall", p_wall)
            .position(at.x, WALL_HEIGHT * 0.5f, at.z)
            .scale(CELL, WALL_HEIGHT, CELL);
        return compages::success();
    }

    m_level_scene.scene->box("Floor", p_floor)
        .position(at.x, -0.1f, at.z)
        .scale(CELL, 0.2f, CELL);
    m_level_scene.scene->box("Ceiling", p_ceiling)
        .position(at.x, WALL_HEIGHT + 0.1f, at.z)
        .scale(CELL, 0.2f, CELL);

    switch (p_cell)
    {
        case 'S':
            m_player.position = at;
            m_player.yaw = PI;
            m_player.pitch = 0.0f;
            break;
        case 'C':
            m_level_scene.scene->box("Crate", p_crate)
                .position(at.x, doom::CRATE * 0.5f, at.z)
                .scale(doom::CRATE);
            break;
        case 'T':
            placeTorch(p_column, p_row);
            break;
        case 'B':
            m_cast.barrels.emplace_back(
                Barrel{ m_level_scene.scene->cylinder("Barrel", p_hazard)
                            .position(at.x, 0.55f, at.z)
                            .scale(0.8f, 1.1f, 0.8f),
                        at });
            break;
        case 'A':
        case 'H':
        {
            const bool shells = (p_cell == 'A');
            compages::world::Entity body = m_level_scene.scene->box(
                shells ? "Shells" : "Health",
                shells ? compages::renderer::color(0.7f, 0.08f, 0.05f)
                       : compages::renderer::color(0.9f, 0.9f, 0.9f));
            body.position(at.x, 0.4f, at.z).scale(0.5f, 0.3f, 0.4f);
            if (!shells)
            {
                m_level_scene.scene
                    ->box("Cross",
                          compages::renderer::color(0.9f, 0.05f, 0.05f))
                    .parent(body)
                    .position(0.0f, 0.52f, 0.0f)
                    .scale(0.7f, 0.05f, 0.2f);
                m_level_scene.scene
                    ->box("Cross",
                          compages::renderer::color(0.9f, 0.05f, 0.05f))
                    .parent(body)
                    .position(0.0f, 0.52f, 0.0f)
                    .scale(0.2f, 0.05f, 0.7f);
            }
            m_cast.pickups.emplace_back(Pickup{ body, at, p_cell });
            break;
        }
        case 'E':
            m_cast.exit = at;
            m_level_scene.scene->box("ExitPad", p_hazard)
                .position(at.x, 0.02f, at.z)
                .scale(2.6f, 0.05f, 2.6f);
            m_level_scene.scene
                ->box("ExitLight", compages::renderer::color(0.3f, 4.0f, 0.8f))
                .position(at.x, WALL_HEIGHT * 0.5f, at.z)
                .scale(0.25f, WALL_HEIGHT, 0.25f);
            m_level_scene.scene
                ->lamp("ExitLamp",
                       compages::core::Vector3f(0.3f, 1.0f, 0.45f),
                       3.0f,
                       10.0f)
                .position(at.x, 2.0f, at.z);
            break;
        case 'M':
            COMPAGES_TRY(placeEnemy(Kind::Soldier, at));
            break;
        case 'R':
            COMPAGES_TRY(placeEnemy(Kind::Robot, at));
            break;
        default:
            break;
    }
    return compages::success();
}

void DoomLike::placeTorch(std::size_t p_column, std::size_t p_row)
{
    // On the first wall around the cell, a bracket and a flame. With no wall
    // around, a brazier on a pole in the middle of the cell.
    const compages::core::Vector3f at = doom::Map::center(p_column, p_row);
    const long column = long(p_column);
    const long row = long(p_row);
    compages::core::Vector3f toward(0.0f, 0.0f, 0.0f);
    if (m_content.map.at(column, row - 1) == '#')
    {
        toward = compages::core::Vector3f(0.0f, 0.0f, -1.0f);
    }
    else if (m_content.map.at(column, row + 1) == '#')
    {
        toward = compages::core::Vector3f(0.0f, 0.0f, 1.0f);
    }
    else if (m_content.map.at(column - 1, row) == '#')
    {
        toward = compages::core::Vector3f(-1.0f, 0.0f, 0.0f);
    }
    else if (m_content.map.at(column + 1, row) == '#')
    {
        toward = compages::core::Vector3f(1.0f, 0.0f, 0.0f);
    }
    const bool pole = (toward.norm() < 0.5f);

    const compages::core::Vector3f base =
        at + (toward * ((CELL * 0.5f) - 0.15f));
    m_level_scene.scene
        ->box("Bracket", compages::renderer::color(0.12f, 0.1f, 0.08f))
        .position(base.x, pole ? 1.05f : 1.9f, base.z)
        .scale(0.14f, pole ? 2.1f : 0.6f, 0.14f);
    compages::world::Entity flame = m_level_scene.scene->box(
        "Flame", compages::renderer::color(4.0f, 1.8f, 0.5f));
    flame.position(base.x, 2.3f, base.z)
        .scale(0.18f, 0.3f, 0.18f)
        .rotate(0.785f, compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    const compages::core::Vector3f glow =
        at + (toward * ((CELL * 0.5f) - 0.5f));
    compages::world::Entity light = m_level_scene.scene->lamp(
        "Torch", compages::core::Vector3f(1.0f, 0.55f, 0.22f), 3.0f, 11.0f);
    light.position(glow.x, 2.3f, glow.z)
        .add<Flicker>(float((p_row * 7u) + p_column));
    m_cast.torches.emplace_back(
        Torch{ light, flame, compages::core::Vector3f(base.x, 2.3f, base.z) });
}

compages::Status DoomLike::placeEnemy(Kind p_kind,
                                      compages::core::Vector3f p_at)
{
    const bool soldier = (p_kind == Kind::Soldier);
    auto placed = m_level_scene.scene->instantiate(
        soldier ? m_content.soldier_prefab : m_content.robot_prefab);
    if (!placed)
    {
        return compages::failure(placed.error());
    }
    Enemy enemy;
    enemy.kind = p_kind;
    enemy.root = placed.value();
    enemy.root.position(p_at);
    enemy.wander = p_at.x;
    enemy.health = soldier ? 6.0f : 9.0f;
    if (!soldier)
    {
        enemy.root.scale(ROBOT_SCALE);
        playLoop(enemy.root.id(), "Walking");
        m_cast.enemies.emplace_back(enemy);
        return compages::success();
    }

    // A soldier carries a shotgun in its right hand. The hand is a bone of a
    // model made in centimetres: the gun is scaled back up by the scale the
    // hand ends with, and turned so that it points along the fingers.
    playLoop(enemy.root.id(), "Walk");
    enemy.right_arm = findNamed(enemy.root.id(), "mixamorig:RightArm");
    enemy.left_arm = findNamed(enemy.root.id(), "mixamorig:LeftArm");
    enemy.right_forearm = findNamed(enemy.root.id(), "mixamorig:RightForeArm");
    enemy.left_forearm = findNamed(enemy.root.id(), "mixamorig:LeftForeArm");
    const compages::world::EntityId hand =
        findNamed(enemy.root.id(), "mixamorig:RightHand");
    if (hand.valid())
    {
        m_level_scene.world->update();
        compages::core::Matrix44f const& m =
            m_level_scene.world->worldMatrix(hand);
        const float scale = std::max(
            compages::core::Vector3f(m[0].x, m[0].y, m[0].z).norm(), 1.0e-6f);
        compages::world::LocalTransform grip;
        grip.position =
            compages::core::Vector3f(0.0f, 0.09f / scale, 0.03f / scale);
        grip.rotation = compages::core::Quatf::fromAngleAxis(
            units::angle::radian_t(double(PI * 0.5f)),
            compages::core::Vector3f(1.0f, 0.0f, 0.0f));
        grip.scale = compages::core::Vector3f(
            0.75f / scale, 0.75f / scale, 0.75f / scale);
        auto gun = m_level_scene.scene->instantiate(
            m_content.shotgun_prefab, hand, grip);
        if (!gun)
        {
            return compages::failure(gun.error());
        }
        enemy.gun = gun.value();
        enemy.muzzle = compages::world::Entity(
            *m_level_scene.world, findNamed(enemy.gun.id(), "Muzzle"));
        playLoop(enemy.gun.id(), "Idle");
    }
    m_cast.enemies.emplace_back(enemy);
    return compages::success();
}

void DoomLike::say(std::string p_message)
{
    m_ui.message = std::move(p_message);
    m_ui.message_time = 2.0f;
}

void DoomLike::playOnce(compages::world::EntityId p_model,
                        std::string_view p_clip)
{
    // From its start, and held on its last pose at the end.
    m_level_scene.scene->play(p_model, p_clip);
    if (compages::renderer::Animator* animator =
            m_level_scene.world->tryGet<compages::renderer::Animator>(p_model))
    {
        animator->loop = false;
        animator->time = 0.0f;
        animator->playing = true;
    }
}

void DoomLike::playLoop(compages::world::EntityId p_model,
                        std::string_view p_clip)
{
    if (compages::renderer::Animator* animator =
            m_level_scene.world->tryGet<compages::renderer::Animator>(p_model))
    {
        animator->loop = true;
    }
    m_level_scene.scene->play(p_model, p_clip);
}

float DoomLike::clipLength(compages::world::EntityId p_model,
                           std::string_view p_clip) const
{
    if (compages::renderer::Animator const* animator =
            m_level_scene.world->tryGet<compages::renderer::Animator>(p_model))
    {
        for (compages::renderer::AnimationClipId const id : animator->clips)
        {
            compages::renderer::AnimationClip const* clip =
                m_content.assets.animation(id);
            if ((clip != nullptr) && (clip->name == p_clip))
            {
                return clip->duration;
            }
        }
    }
    return 1.0f;
}

compages::world::EntityId DoomLike::findNamed(compages::world::EntityId p_root,
                                              std::string_view p_name) const
{
    std::vector<compages::world::EntityId> open{ p_root };
    while (!open.empty())
    {
        const compages::world::EntityId id = open.back();
        open.pop_back();
        if (!id.valid())
        {
            continue;
        }
        if (m_level_scene.world->name(id) == p_name)
        {
            return id;
        }
        open.emplace_back(m_level_scene.world->firstChild(id));
        if (!(id == p_root))
        {
            open.emplace_back(m_level_scene.world->nextSibling(id));
        }
    }
    return {};
}

void DoomLike::tickTimers(float p_dt)
{
    m_session.banner = std::max(0.0f, m_session.banner - p_dt);
    m_player.hurt = std::max(0.0f, m_player.hurt - p_dt);
    m_player.flash = std::max(0.0f, m_player.flash - p_dt);
    m_player.shake = std::max(0.0f, m_player.shake - p_dt);
    m_ui.message_time = std::max(0.0f, m_ui.message_time - p_dt);
}

void DoomLike::tickPlaying(compages::world::ViewFrame const& p_frame, float p_dt)
{
    movePlayer(p_frame);
    useWeapon(p_frame);
    compages::world::Input const& input = p_frame.input;
    if (compages::world::pressed(input, m_ui.previous, compages::world::Key::F))
    {
        useTorch();
    }
    if (compages::world::pressed(input, m_ui.previous, compages::world::Key::L))
    {
        m_player.lantern_on = !m_player.lantern_on;
        m_rig.lantern.enable(m_player.lantern_on);
        say(m_player.lantern_on ? "Lantern on" : "Lantern off");
    }
    pickUp(p_dt);
    moveEnemies(p_dt);
    moveBolts(p_dt);
    moveBarrels(p_dt);
    if (m_player.health <= 0.0f)
    {
        m_player.health = 0.0f;
        m_session.state = State::Dead;
    }
}

void DoomLike::tickAfterDeath(compages::world::ViewFrame const& p_frame)
{
    if (!compages::world::pressed(
            p_frame.input, m_ui.previous, compages::world::Key::Space))
    {
        return;
    }
    const bool won = (m_session.state == State::Won);
    if (won)
    {
        m_session.kills = 0u;
        m_player.reserve = 24;
    }
    const compages::Status built = buildLevel(won ? 0u : m_session.level);
    if (!built)
    {
        compages::gpu::reportError(built.error());
    }
}

void DoomLike::updateBlastLights(float p_dt)
{
    for (Blast& blast : m_cast.blasts)
    {
        blast.age += p_dt;
        blast.light.get<compages::world::PointLight>().intensity =
            std::max(0.0f, 9.0f * (1.0f - (blast.age * 2.0f)));
        blast.light.enable(blast.age < 0.5f);
    }
    std::erase_if(m_cast.blasts, [](Blast const& b) { return b.age >= 0.5f; });
}

void DoomLike::drawAimCrosshair()
{
    const compages::core::Vector3f ahead = eye() + (aim() * 0.5f);
    const compages::core::Vector3f right(std::cos(m_player.yaw) * 0.008f,
                                         0.0f,
                                         -std::sin(m_player.yaw) * 0.008f);
    const compages::core::Vector3f up(0.0f, 0.008f, 0.0f);
    const compages::core::Vector3f color =
        (m_player.gun == Gun::Reloading)
            ? compages::core::Vector3f(0.5f, 0.5f, 0.5f)
            : compages::core::Vector3f(0.9f, 0.9f, 0.9f);
    m_level_scene.scene->debug().line(ahead - right, ahead + right, color);
    m_level_scene.scene->debug().line(ahead - up, ahead + up, color);
}

void DoomLike::draw(compages::world::ViewFrame const& p_frame)
{
    const float dt = p_frame.elapsed;
    tickTimers(dt);

    if (m_session.state == State::Playing)
    {
        tickPlaying(p_frame, dt);
    }
    else
    {
        tickAfterDeath(p_frame);
    }
    if (m_session.state == State::Won)
    {
        m_player.yaw += 0.3f * dt;
    }

    updateBlastLights(dt);
    placeView(p_frame);

    m_rig.lantern.get<compages::world::PointLight>().intensity =
        (m_player.flash > 0.0f) ? 6.0f : 1.6f;
    m_rig.lantern.enable(m_player.lantern_on || (m_player.flash > 0.0f));
    m_level_scene.scene->ambient(0.05f + (1.2f * m_player.hurt), 0.05f, 0.06f);

    m_content.effects.update(dt);
    m_level_scene.scene->update(p_frame);
    aimArms();

    if (m_session.state == State::Playing)
    {
        drawAimCrosshair();
    }
    m_content.effects.lines(m_level_scene.scene->debug());
    m_level_scene.scene->render();
    m_content.effects.draw(
        m_level_scene.scene->lastCamera(),
        m_rig.camera.get<compages::world::Camera>().fov.template to<float>(),
        float(p_frame.height));
    m_ui.previous = p_frame.input;
}

std::string DoomLike::hud() const
{
    doom::LevelData const& level = doom::levels()[m_session.level];
    std::size_t standing = 0u;
    for (Enemy const& enemy : m_cast.enemies)
    {
        standing += (enemy.mood != Mood::Dead) ? 1u : 0u;
    }
    std::string text = "Level " + std::to_string(m_session.level + 1u) + "/" +
                       std::to_string(doom::levels().size()) + "  " +
                       level.title + "\n" + "Health " +
                       std::to_string(int(std::ceil(m_player.health))) +
                       "    Shells " + std::to_string(m_player.shells) + " | " +
                       std::to_string(m_player.reserve) +
                       ((m_player.gun == Gun::Reloading) ? "  reloading" : "") +
                       "    Enemies " + std::to_string(standing) + "\n";
    if (m_ui.message_time > 0.0f)
    {
        text += m_ui.message;
    }
    else if (m_session.state == State::Playing)
    {
        if (const std::optional<std::size_t> torch = torchAimedAt())
        {
            text += m_cast.torches[*torch].lit ? "F: put out the torch"
                                               : "F: light the torch";
        }
        else if (!m_ui.previous.mouse_captured)
        {
            text += "Click the picture: the pointer hides for look (Escape "
                    "gives it back)";
        }
    }
    if (m_session.state == State::Won)
    {
        text += "\n!YOU WIN\n!Press SPACE to play again";
    }
    else if (m_session.state == State::Dead)
    {
        text += "\n!YOU DIED\n!Press SPACE to try again";
    }
    else if (m_session.banner > 0.0f)
    {
        text += std::string("\n!") + level.title + "\n!Find the green light";
    }
    return text;
}

namespace
{

struct NamedKey
{
    char const* label;
    int code;
};

constexpr NamedKey BINDABLE_KEYS[]{
    { "W", 87 },      { "A", 65 },           { "S", 83 },
    { "D", 68 },      { "Q", 81 },           { "E", 69 },
    { "R", 82 },      { "F", 70 },           { "L", 76 },
    { "Space", 32 },  { "Left Shift", 340 }, { "Right Shift", 344 },
    { "Up", 265 },    { "Down", 264 },       { "Left", 263 },
    { "Right", 262 },
};

constexpr char const* BINDABLE_LABELS[]{
    "W", "A",     "S",          "D",           "Q",  "E",    "R",    "F",
    "L", "Space", "Left Shift", "Right Shift", "Up", "Down", "Left", "Right",
};

[[nodiscard]] int indexForCode(int p_code)
{
    for (std::size_t i = 0u; i < std::size(BINDABLE_KEYS); ++i)
    {
        if (BINDABLE_KEYS[i].code == p_code)
        {
            return int(i);
        }
    }
    return 0;
}

void keyBindingCombo(char const* p_label,
                     compages::world::KeyMap& p_map,
                     compages::world::Key p_key)
{
    int& code = p_map.codes[static_cast<std::size_t>(p_key)];
    int index = indexForCode(code);
    if (ImGui::Combo(
            p_label, &index, BINDABLE_LABELS, int(std::size(BINDABLE_LABELS))))
    {
        code = BINDABLE_KEYS[static_cast<std::size_t>(index)].code;
    }
}

void bindingTab(
    compages::world::KeyMap& p_map,
    std::initializer_list<std::pair<char const*, compages::world::Key>> p_rows)
{
    for (std::pair<char const*, compages::world::Key> const& row : p_rows)
    {
        keyBindingCombo(row.first, p_map, row.second);
    }
}

} // namespace

void DoomLike::controls()
{
    ImGui::Text("Health %.0f  Shells %d | %d  Down %zu",
                static_cast<double>(m_player.health),
                m_player.shells,
                m_player.reserve,
                m_session.kills);
    for (std::size_t i = 0u; i < doom::levels().size(); ++i)
    {
        const std::string label = std::string("Level ") +
                                  std::to_string(i + 1u) + ": " +
                                  doom::levels()[i].title;
        if (ImGui::Button(label.c_str()))
        {
            const compages::Status built = buildLevel(i);
            if (!built)
            {
                compages::gpu::reportError(built.error());
            }
        }
    }
    if (ImGui::Button("Fill up: health and shells"))
    {
        m_player.health = 100.0f;
        m_player.shells = 8;
        m_player.reserve = 48;
    }
    ImGui::SliderFloat(
        "Fog", &m_level_scene.scene->environment().fog_density, 0.0f, 0.15f);

    if (ImGui::BeginTabBar("##doom_keys"))
    {
        if (ImGui::BeginTabItem("Move"))
        {
            bindingTab(m_keys,
                       { { "Forward (W)", compages::world::Key::W },
                         { "Back (S)", compages::world::Key::S },
                         { "Strafe left (A)", compages::world::Key::A },
                         { "Strafe right (D)", compages::world::Key::D },
                         { "Turn left (Q)", compages::world::Key::Q },
                         { "Turn right (E)", compages::world::Key::E },
                         { "Run (Shift)", compages::world::Key::Shift },
                         { "Arrow up", compages::world::Key::Up },
                         { "Arrow down", compages::world::Key::Down },
                         { "Arrow left", compages::world::Key::Left },
                         { "Arrow right", compages::world::Key::Right } });
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Fight"))
        {
            bindingTab(m_keys, { { "Reload", compages::world::Key::R } });
            ImGui::TextUnformatted(
                "Shoot: left mouse button (not remapped here).");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Use"))
        {
            bindingTab(m_keys,
                       { { "Torch", compages::world::Key::F },
                         { "Lantern", compages::world::Key::L },
                         { "Restart / again", compages::world::Key::Space } });
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    if (ImGui::Button("Reset keys to defaults"))
    {
        m_keys = compages::world::KeyMap::defaults();
    }
    ImGui::TextUnformatted(
        "Click the picture to hide the pointer; Escape gives it back.");
}

} // namespace examples
