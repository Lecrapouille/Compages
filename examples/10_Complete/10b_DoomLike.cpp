// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/10b_DoomLike.hpp"
#include "Compages/World/Entity.hpp"

#include "Common/DataPath.hpp"
#include "Common/Gui.hpp"
#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Units.hpp"
#include "Compages/Renderer/Animation/Animator.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Scene/Looks.hpp"
#include "10_Complete/DoomLike/Materials.hpp"

#include <algorithm>
#include <cmath>

namespace examples
{

using namespace units::literals;

using doom::CELL;
using doom::PI;
using doom::ROBOT_SCALE;
using doom::WALL_HEIGHT;

// ----------------------------------------------------------------------------
//! \brief Makes the light of a wall lamp hum and, now and then, stutter, the
//! way a tired fluorescent or sodium lamp does.
// ----------------------------------------------------------------------------
struct DoomLike::Flicker: compages::world::Routine
{
    explicit Flicker(float p_phase) : phase(p_phase) {}

    void update(float) override
    {
        const float t = frame().total + phase;
        compages::world::PointLight& light = entity().get<compages::world::PointLight>();
        // A hum of two unrelated speeds; a short drop about one second in
        // twelve, at a moment that changes with the lamp.
        const float hum = 3.0f + (0.15f * std::sin(t * 13.0f)) + (0.1f * std::sin(t * 7.3f));
        const float slot = std::floor(t * 6.0f);
        const float chance = std::fmod(std::abs(std::sin((slot * 12.9898f) + (phase * 78.233f)) * 43758.5453f), 1.0f);
        light.intensity = (chance > 0.985f) ? (0.25f * hum) : hum;
    }

    float phase;
};

// ----------------------------------------------------------------------------
std::string DoomLike::description() const
{
    return "A small first person shooter in two levels, built from everything "
           "the engine does: walls, floors and ceilings in photographed "
           "materials (Poly Haven, CC0: colour, normal and roughness maps, "
           "triplanar), humming wall lamps, fog, glTF soldiers that aim and "
           "shoot, glTF robots that run and punch, a bolt-action rifle whose "
           "recoil and bolt the code moves, particles of blood, fire and "
           "smoke, marks on the walls and on the bodies, explosive barrels. "
           "Reach the green light to leave a level.\n\nClick the picture: the "
           "mouse then looks around (Escape gives it back). W A S D or the "
           "arrows walk, Shift runs, Q and E turn, a left click shoots, R "
           "reloads, F switches off or on the lamp aimed at, L switches the "
           "lantern. Green boxes are rounds, white kits heal.";
}

// ----------------------------------------------------------------------------
compages::Status DoomLike::setUp()
{
    // Loaded once for both levels: each Scene built later shares them.
    auto load = [this](std::string const& p_file) -> compages::Result<compages::renderer::PrefabId>
    {
        const std::filesystem::path path = dataPath(p_file);
        if (path.empty())
        {
            return compages::failure(p_file + " is missing from external/Compages-data/");
        }
        return m_content.assets.loadFile(path);
    };
    auto polyhaven = [&load](std::string const& p_name)
    { return load("polyhaven/models/" + p_name + "/" + p_name + ".gltf"); };
    COMPAGES_TRY_ASSIGN(m_content.soldier_prefab, load("Soldier.glb"));
    COMPAGES_TRY_ASSIGN(m_content.robot_prefab, load("RobotExpressive.glb"));
    COMPAGES_TRY_ASSIGN(m_content.rifle_prefab, polyhaven("bolt_action_rifle_7_62"));
    COMPAGES_TRY_ASSIGN(m_content.ammo_prefab, polyhaven("ammo_box"));
    COMPAGES_TRY_ASSIGN(m_content.medkit_prefab, polyhaven("medical_box"));
    COMPAGES_TRY_ASSIGN(m_content.barrel_prefab, polyhaven("barrel_03"));
    COMPAGES_TRY_ASSIGN(m_content.crate_prefab, polyhaven("old_military_crate"));
    COMPAGES_TRY_ASSIGN(m_content.sconce_prefab, polyhaven("industrial_caged_sconce"));
    COMPAGES_TRY_ASSIGN(m_content.tubes_prefab, polyhaven("mounted_fluorescent_lights"));
    // Every surface of both levels: a picture covers 3 m of a wall, 2 m of
    // the rest.
    for (doom::LevelData const& level : doom::levels())
    {
        for (char const* name : { level.wall, level.floor, level.ceiling, level.container })
        {
            if (!m_content.surfaces.contains(name))
            {
                const std::string_view named(name);
                const float tile = (named == level.wall) ? 3.0f : 2.0f;
                const float metallic =
                    (named.find("metal") != std::string_view::npos) || (named.find("iron") != std::string_view::npos)
                        ? 0.6f
                        : 0.0f;
                compages::renderer::MaterialId surface;
                COMPAGES_TRY_ASSIGN(surface, doom::makeSurface(m_content.assets, name, tile, metallic));
                m_content.surfaces.emplace(name, surface);
            }
        }
    }
    COMPAGES_TRY(m_content.effects.setUp());
    return buildLevel(0u);
}

// ----------------------------------------------------------------------------
compages::Status DoomLike::buildLevel(std::size_t p_level)
{
    doom::LevelData const& level = doom::levels()[p_level];
    const std::filesystem::path hazard_file = dataPath("hazard.png");
    if (hazard_file.empty())
    {
        return compages::failure("10b_DoomLike needs hazard.png of external/Compages-data/");
    }
    beginLevel(p_level);
    configureAtmosphere(level);
    COMPAGES_TRY(setupPlayerRig());
    COMPAGES_TRY(scanMapGrid(level));
    return m_level_scene.scene->prepare();
}

// ----------------------------------------------------------------------------
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
    m_player.shells = doom::LOAD;
    m_player.reserve = std::max(m_player.reserve, 2 * doom::LOAD);
    m_player.gun = Gun::Idle;
    m_player.hurt = 0.0f;
    m_player.shake = 0.0f;
    m_session.banner = 2.5f;
    m_player.clock = 0.0f;
    m_content.effects.reset(*m_level_scene.scene);
}

// ----------------------------------------------------------------------------
void DoomLike::configureAtmosphere(doom::LevelData const& p_level)
{
    m_level_scene.scene->background(
        p_level.fog.x, p_level.fog.y, p_level.fog.z);
    m_level_scene.scene->environment().default_light_color =
        compages::Vector3f(0.0f, 0.0f, 0.0f);
    m_level_scene.scene->environment().fog_color = p_level.fog;
    m_level_scene.scene->environment().fog_density = 0.055f;
    // Dust in the air: halos round the lamps, the light of the tubes in
    // cones, the shadows of the enemies in the air (Environment::volumetric).
    compages::renderer::VolumetricFog& dust = m_level_scene.scene->environment().volumetric;
    dust.enabled = true;
    dust.density = 0.012f;
    dust.height = 0.0f;
    dust.falloff = 0.1f;
    dust.color = compages::Vector3f(0.8f, 0.8f, 0.78f);
    dust.anisotropy = 0.45f;
    dust.noise = 0.6f;
    dust.wind = compages::Vector3f(0.15f, 0.02f, 0.05f);
    dust.distance = 40.0f;
}

// ----------------------------------------------------------------------------
//! The rifle of Poly Haven lies along +x, its trigger near x = -0.29. The
//! pivot is where the grip of the old shotgun was (barrel along -z, grip at
//! 0): a quarter turn round y and a shift put the rifle there, and the
//! muzzle at the end of the barrel.
// ----------------------------------------------------------------------------
compages::Result<compages::world::Entity>
DoomLike::placeRifle(compages::world::EntityId p_parent, compages::world::LocalTransform const& p_grip)
{
    compages::world::Entity pivot = m_level_scene.world->entity("Rifle");
    pivot.parent(compages::world::Entity(*m_level_scene.world, p_parent));
    pivot.position(p_grip.position).rotation(p_grip.rotation).scale(p_grip.scale);
    compages::world::LocalTransform along;
    along.rotation = compages::Quatf::fromAngleAxis(units::angle::radian_t(double(0.5f * PI)),
                                                    compages::Vector3f(0.0f, 1.0f, 0.0f));
    along.position = compages::Vector3f(0.0f, 0.0f, -0.29f);
    auto model = m_level_scene.scene->instantiate(m_content.rifle_prefab, pivot.id(), along);
    if (!model)
    {
        return compages::failure(model.error());
    }
    pivot.child("Muzzle").position(0.0f, 0.02f, -0.92f);
    return pivot;
}

// ----------------------------------------------------------------------------
compages::Status DoomLike::setupPlayerRig()
{
    m_rig.camera = m_level_scene.scene->camera("Player");
    m_rig.camera.get<compages::world::Camera>().fov = 75.0_deg;
    m_rig.camera.get<compages::world::Camera>().near_plane = 0.05f;
    m_rig.camera.get<compages::world::Camera>().far_plane = 80.0f;
    m_rig.lantern = m_level_scene.scene->lamp(
        "Lantern", compages::Vector3f(0.9f, 0.95f, 1.0f), 1.5f, 9.0f);
    m_rig.lantern.parent(m_rig.camera)
        .position(0.3f, -0.2f, 0.0f)
        .enable(m_player.lantern_on);
    compages::world::LocalTransform grip;
    grip.scale = compages::Vector3f(0.62f, 0.62f, 0.62f);
    COMPAGES_TRY_ASSIGN(m_rig.weapon, placeRifle(m_rig.camera.id(), grip));
    m_rig.muzzle = compages::world::Entity(*m_level_scene.world, findNamed(m_rig.weapon.id(), "Muzzle"));
    m_rig.bolt_a = compages::world::Entity(*m_level_scene.world,
                                           findNamed(m_rig.weapon.id(), "bolt_action_rifle_7_62_bolt_a"));
    m_rig.bolt_b = compages::world::Entity(*m_level_scene.world,
                                           findNamed(m_rig.weapon.id(), "bolt_action_rifle_7_62_bolt_b"));
    if (m_rig.bolt_a)
    {
        m_rig.bolt_a_rest = m_rig.bolt_a.position();
    }
    if (m_rig.bolt_b)
    {
        m_rig.bolt_b_rest = m_rig.bolt_b.position();
    }
    return compages::success();
}

// ----------------------------------------------------------------------------
compages::Status DoomLike::scanMapGrid(doom::LevelData const& p_level)
{
    std::vector<std::string> const& rows = m_content.map.rows();
    for (std::size_t row = 0u; row < rows.size(); ++row)
    {
        for (std::size_t column = 0u; column < rows[row].size(); ++column)
        {
            COMPAGES_TRY(placeCell(rows[row][column], column, row, p_level));
        }
    }
    return compages::success();
}

// ----------------------------------------------------------------------------
compages::world::Entity
DoomLike::surfaceBox(char const* p_name, std::string_view p_surface, compages::Vector3f p_tint)
{
    compages::world::Entity box = m_level_scene.scene->box(p_name);
    if (const auto found = m_content.surfaces.find(p_surface); found != m_content.surfaces.end())
    {
        m_level_scene.scene->look(box, found->second);
        if (compages::renderer::MaterialInstance* instance = m_content.assets.materialInstance(
                box.get<compages::renderer::MeshRenderer>().material_instance))
        {
            instance->base_color_factor = compages::renderer::srgbToLinear(p_tint);
        }
    }
    return box;
}

// ----------------------------------------------------------------------------
compages::Result<compages::world::Entity>
DoomLike::placeProp(compages::renderer::PrefabId p_prefab, std::initializer_list<std::string_view> p_keep)
{
    auto placed = m_level_scene.scene->instantiate(p_prefab);
    if (!placed)
    {
        return compages::failure(placed.error());
    }
    compages::world::Entity root = placed.value();
    if (p_keep.size() > 0u)
    {
        root.children([&p_keep](compages::world::Entity p_child) {
            const bool kept = std::find(p_keep.begin(), p_keep.end(), p_child.name()) != p_keep.end();
            p_child.enable(kept);
        });
    }
    return root;
}

// ----------------------------------------------------------------------------
compages::Status DoomLike::placeCell(char p_cell, std::size_t p_column, std::size_t p_row,
                                     doom::LevelData const& p_level)
{
    const compages::Vector3f at = doom::Map::center(p_column, p_row);
    if (p_cell == '#')
    {
        surfaceBox("Wall", p_level.wall, p_level.wall_tint)
            .position(at.x, WALL_HEIGHT * 0.5f, at.z)
            .scale(CELL, WALL_HEIGHT, CELL);
        return compages::success();
    }

    surfaceBox("Floor", p_level.floor, compages::Vector3f(0.8f, 0.8f, 0.8f))
        .position(at.x, -0.1f, at.z)
        .scale(CELL, 0.2f, CELL);
    surfaceBox("Ceiling", p_level.ceiling, p_level.wall_tint * 0.6f)
        .position(at.x, WALL_HEIGHT + 0.1f, at.z)
        .scale(CELL, 0.2f, CELL);
    if ((p_cell != 'T') && (((p_column * 3u) + p_row) % 4u == 0u))
    {
        placeTubes(p_column, p_row);
    }

    switch (p_cell)
    {
        case 'S':
            m_player.position = at;
            m_player.yaw = PI;
            m_player.pitch = 0.0f;
            break;
        case 'C':
        {
            // A steel container, a military crate left on top.
            surfaceBox("Container", p_level.container, compages::Vector3f(0.75f, 0.8f, 0.75f))
                .position(at.x, doom::CRATE * 0.5f, at.z)
                .scale(doom::CRATE);
            compages::world::Entity crate;
            COMPAGES_TRY_ASSIGN(crate, placeProp(m_content.crate_prefab,
                                                 { "old_military_crate_a", "old_military_crate_lid_a",
                                                   "old_military_crate_loop_a", "old_military_crate_latch_a",
                                                   "old_military_crate_cloth_a" }));
            const float turn = float((p_column * 7u) + p_row);
            crate.position(at.x, doom::CRATE, at.z)
                .rotation(turn, compages::Vector3f(0.0f, 1.0f, 0.0f))
                .scale(1.4f);
            crate.children([](compages::world::Entity p_part) {
                // Variant a stands at x = -0.5 in its file.
                p_part.position(p_part.position() + compages::Vector3f(0.5f, 0.0f, 0.0f));
            });
            break;
        }
        case 'T':
            placeTorch(p_column, p_row);
            break;
        case 'B':
        {
            compages::world::Entity barrel;
            COMPAGES_TRY_ASSIGN(barrel, placeProp(m_content.barrel_prefab, {}));
            barrel.position(at.x, 0.0f, at.z)
                .rotation(float(p_column + p_row), compages::Vector3f(0.0f, 1.0f, 0.0f))
                .scale(1.2f);
            m_cast.barrels.emplace_back(barrel, at);
            break;
        }
        case 'A':
        case 'H':
        {
            const bool rounds = (p_cell == 'A');
            compages::world::Entity body;
            COMPAGES_TRY_ASSIGN(body, placeProp(rounds ? m_content.ammo_prefab : m_content.medkit_prefab, {}));
            body.scale(rounds ? 2.6f : 1.4f);
            m_cast.pickups.emplace_back(Pickup{ body, at, p_cell });
            break;
        }
        case 'E':
        {
            m_cast.exit = at;
            compages::renderer::Look pad = compages::renderer::texture(dataPath("hazard.png"));
            m_level_scene.scene->box("ExitPad", pad)
                .position(at.x, 0.02f, at.z)
                .scale(2.6f, 0.05f, 2.6f);
            m_level_scene.scene
                ->box("ExitLight", compages::renderer::color(0.3f, 4.0f, 0.8f))
                .position(at.x, WALL_HEIGHT * 0.5f, at.z)
                .scale(0.25f, WALL_HEIGHT, 0.25f);
            m_level_scene.scene
                ->lamp("ExitLamp",
                       compages::Vector3f(0.3f, 1.0f, 0.45f),
                       3.0f,
                       10.0f)
                .position(at.x, 2.0f, at.z);
            break;
        }
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

// ----------------------------------------------------------------------------
//! A fixture of fluorescent tubes under the ceiling, its tube glowing, and a
//! cold lamp of short range under it: one of the many lamps of a level that
//! the Renderer sorts into clusters (the four nearest have uniforms).
// ----------------------------------------------------------------------------
void DoomLike::placeTubes(std::size_t p_column, std::size_t p_row)
{
    const compages::Vector3f at = doom::Map::center(p_column, p_row);
    auto fixture = placeProp(m_content.tubes_prefab, { "mounted_fluorescent_lights_d" });
    if (!fixture)
    {
        return;
    }
    const float turn = ((p_column + p_row) % 2u == 0u) ? 0.0f : 0.5f * PI;
    fixture.value()
        .position(at.x, WALL_HEIGHT, at.z)
        .rotation(turn, compages::Vector3f(0.0f, 1.0f, 0.0f))
        .scale(1.6f);
    m_level_scene.scene->box("Tube glow", compages::renderer::unlit(0.75f, 0.95f, 0.9f))
        .parent(fixture.value())
        .position(0.0f, -0.032f, 0.0f)
        .scale(0.86f, 0.012f, 0.022f)
        .get<compages::renderer::MeshRenderer>()
        .flags = compages::renderer::RenderFlags::NoShadowCaster;
    m_level_scene.scene->lamp("Tube light", compages::Vector3f(0.75f, 0.9f, 1.0f), 1.3f, 6.5f)
        .position(at.x, WALL_HEIGHT - 0.25f, at.z);
}

// ----------------------------------------------------------------------------
//! \brief What holds a lamp casts no shadow of that lamp: the cage would
//! throw a huge one on the ceiling.
// ----------------------------------------------------------------------------
static void castNoShadow(compages::world::Entity p_entity)
{
    if (p_entity.has<compages::renderer::MeshRenderer>())
    {
        p_entity.get<compages::renderer::MeshRenderer>().flags = compages::renderer::RenderFlags::NoShadowCaster;
    }
    p_entity.children([](compages::world::Entity p_child) { castNoShadow(p_child); });
}

// ----------------------------------------------------------------------------
void DoomLike::placeTorch(std::size_t p_column, std::size_t p_row)
{
    // A caged lamp on the first wall around the cell. With no wall around, on
    // a steel post in the middle of the cell.
    const compages::Vector3f at = doom::Map::center(p_column, p_row);
    const long column = long(p_column);
    const long row = long(p_row);
    compages::Vector3f toward(0.0f, 0.0f, 0.0f);
    if (m_content.map.at(column, row - 1) == '#')
    {
        toward = compages::Vector3f(0.0f, 0.0f, -1.0f);
    }
    else if (m_content.map.at(column, row + 1) == '#')
    {
        toward = compages::Vector3f(0.0f, 0.0f, 1.0f);
    }
    else if (m_content.map.at(column - 1, row) == '#')
    {
        toward = compages::Vector3f(-1.0f, 0.0f, 0.0f);
    }
    else if (m_content.map.at(column + 1, row) == '#')
    {
        toward = compages::Vector3f(1.0f, 0.0f, 0.0f);
    }
    const bool post = (toward.norm() < 0.5f);
    if (post)
    {
        toward = compages::Vector3f(0.0f, 0.0f, -1.0f);
        castNoShadow(m_level_scene.scene->box("Lamp post", compages::renderer::color(0.18f, 0.2f, 0.2f))
                         .position(at.x, 1.1f, at.z)
                         .scale(0.12f, 2.2f, 0.12f));
    }
    // The sconce of the file faces +z from its wall at z = 0: turn it to face
    // away from the wall.
    const compages::Vector3f wall = post ? at : (at + (toward * (CELL * 0.5f)));
    const float yaw = std::atan2(-toward.x, -toward.z);
    compages::world::Entity sconce;
    if (auto placed = placeProp(m_content.sconce_prefab, { "industrial_caged_sconce_a" }))
    {
        sconce = placed.value();
        sconce.position(wall.x, post ? 2.1f : 2.2f, wall.z)
            .rotation(yaw, compages::Vector3f(0.0f, 1.0f, 0.0f))
            .scale(1.6f);
        castNoShadow(sconce);
        sconce.children([](compages::world::Entity p_part) {
            // Variant a stands at x = 0.5 in its file.
            p_part.position(p_part.position() - compages::Vector3f(0.5f, 0.0f, 0.0f));
        });
    }
    // The bulb in the cage, glowing; the light just in front of it.
    const compages::Vector3f out(-toward.x, 0.0f, -toward.z);
    const compages::Vector3f bulb = wall + (out * 0.2f) + compages::Vector3f(0.0f, post ? 2.25f : 2.35f, 0.0f);
    compages::world::Entity flame = m_level_scene.scene->sphere("Bulb", compages::renderer::unlit(1.0f, 0.85f, 0.55f));
    flame.position(bulb).scale(0.09f);
    flame.get<compages::renderer::MeshRenderer>().flags = compages::renderer::RenderFlags::NoShadowCaster;
    const compages::Vector3f glow = bulb + (out * 0.3f);
    compages::world::Entity light = m_level_scene.scene->lamp(
        "Wall lamp", compages::Vector3f(1.0f, 0.8f, 0.58f), 3.0f, 11.0f);
    light.position(glow).add<Flicker>(float((p_row * 7u) + p_column));
    // The soldiers, the robots and the barrels cast shadows on the walls.
    light.get<compages::world::PointLight>().shadows = true;
    m_cast.torches.emplace_back(light, flame, bulb);
}

// ----------------------------------------------------------------------------
compages::Status DoomLike::placeEnemy(Kind p_kind, compages::Vector3f p_at)
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

    // A soldier carries a rifle in its right hand. The hand is a bone of a
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
        compages::Matrix44f const& m = m_level_scene.world->worldMatrix(hand);
        const float scale = std::max(
            compages::Vector3f(m[0].x, m[0].y, m[0].z).norm(), 1.0e-6f);
        compages::world::LocalTransform grip;
        grip.position = compages::Vector3f(0.0f, 0.09f / scale, 0.03f / scale);
        grip.rotation = compages::Quatf::fromAngleAxis(
            units::angle::radian_t(double(PI * 0.5f)),
            compages::Vector3f(1.0f, 0.0f, 0.0f));
        grip.scale =
            compages::Vector3f(0.75f / scale, 0.75f / scale, 0.75f / scale);
        auto gun = placeRifle(hand, grip);
        if (!gun)
        {
            return compages::failure(gun.error());
        }
        enemy.gun = gun.value();
        enemy.muzzle = compages::world::Entity(
            *m_level_scene.world, findNamed(enemy.gun.id(), "Muzzle"));
    }
    m_cast.enemies.emplace_back(enemy);
    return compages::success();
}

// ----------------------------------------------------------------------------
void DoomLike::say(std::string p_message)
{
    m_ui.message = std::move(p_message);
    m_ui.message_time = 2.0f;
}

// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
void DoomLike::tickTimers(float p_dt)
{
    m_session.banner = std::max(0.0f, m_session.banner - p_dt);
    m_player.hurt = std::max(0.0f, m_player.hurt - p_dt);
    m_player.flash = std::max(0.0f, m_player.flash - p_dt);
    m_player.shake = std::max(0.0f, m_player.shake - p_dt);
    m_ui.message_time = std::max(0.0f, m_ui.message_time - p_dt);
}

// ----------------------------------------------------------------------------
void DoomLike::tickPlaying(compages::world::ViewFrame const& p_frame,
                           float p_dt)
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

// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
void DoomLike::drawAimCrosshair()
{
    const compages::Vector3f ahead = eye() + (aim() * 0.5f);
    const compages::Vector3f right(std::cos(m_player.yaw) * 0.008f,
                                   0.0f,
                                   -std::sin(m_player.yaw) * 0.008f);
    const compages::Vector3f up(0.0f, 0.008f, 0.0f);
    const compages::Vector3f color = (m_player.gun == Gun::Reloading)
                                         ? compages::Vector3f(0.5f, 0.5f, 0.5f)
                                         : compages::Vector3f(0.9f, 0.9f, 0.9f);
    m_level_scene.scene->debug().line(ahead - right, ahead + right, color);
    m_level_scene.scene->debug().line(ahead - up, ahead + up, color);
}

// ----------------------------------------------------------------------------
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
    // Colours are sRGB, like a colour picker: 0.22 is a dim grey.
    m_level_scene.scene->ambient(0.22f + (1.2f * m_player.hurt), 0.22f, 0.24f);

    m_content.effects.update(dt);
    m_level_scene.scene->update(p_frame);
    aimArms();

    if (m_session.state == State::Playing)
    {
        drawAimCrosshair();
    }
    m_content.effects.lines(m_level_scene.scene->debug());
    m_level_scene.scene->render();
    {
        // The particles are hidden by the walls the Scene drew: they test the
        // depth it left, so they use the same reversed depth.
        compages::gpu::PassDesc over = compages::gpu::currentPass();
        over.clear_color = false;
        over.clear_depth = false;
        over.reversed_depth = true;
        compages::gpu::RenderPass const pass(over);
        m_content.effects.draw(
            m_level_scene.scene->lastCamera(),
            m_rig.camera.get<compages::world::Camera>().fov.template to<float>(),
            float(p_frame.height));
    }
    m_ui.previous = p_frame.input;
}

// ----------------------------------------------------------------------------
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
                       "    Rounds " + std::to_string(m_player.shells) + " | " +
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

// ----------------------------------------------------------------------------
void DoomLike::controls()
{
    ImGui::Text("Health %.0f  Rounds %d | %d  Down %zu",
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
    if (ImGui::Button("Fill up: health and rounds"))
    {
        m_player.health = 100.0f;
        m_player.shells = doom::LOAD;
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
