//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#pragma once

#include "50_Complete/DoomLike/Effects.hpp"
#include "50_Complete/DoomLike/Map.hpp"
#include "Common/Example.hpp"

#include "Compages/Renderer/Scene.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief A small Doom-like game in two levels, built from the whole engine.
//!
//! Each level is a grid of characters (see doom::LevelData). Walking into the
//! green light of the exit goes to the next level; leaving the second one
//! wins. What each part of the engine does here:
//!   - the Scene: textured boxes for the walls, lamps for the torches, fog;
//!   - glTF: the soldiers and the robots, skinned and animated with the clips
//!     of their file, and the shotgun, whose Idle, Fire and Reload clips are
//!     written by DoomLike/make_shotgun.py;
//!   - the World: behaviors (the torches flicker), parents (the gun in the
//!     hands, the marks on the bones), and bones turned by hand after the
//!     clips so that a soldier aims;
//!   - the GPU layer: particles drawn after the Scene with its camera.
//!
//! The code is shared between this file (the levels and the frame),
//! DoomLike/Player.cpp (moving, shooting, using), DoomLike/Enemies.cpp (the
//! enemies, their shots, the barrels), DoomLike/Map.cpp and
//! DoomLike/Effects.cpp.
// ****************************************************************************
class DoomLike final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "53_DoomLike";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;
    [[nodiscard]] std::string hud() const override;
    [[nodiscard]] bool capturesMouse() const override
    {
        return true;
    }
    [[nodiscard]] compages::world::KeyMap keyMap() const override
    {
        return m_keys;
    }

private:

    //! \brief Makes a torch flicker.
    struct Flicker;

    enum class State
    {
        Playing,
        Dead,
        Won,
    };

    //! \brief A soldier shoots from afar; a robot runs and punches.
    enum class Kind
    {
        Soldier,
        Robot,
    };

    enum class Mood
    {
        Wander,
        Chase,
        Attack,
        Hurt,
        Dead,
    };

    struct Enemy
    {
        Kind kind;
        compages::world::Entity root;
        Mood mood = Mood::Wander;
        compages::core::Vector3f heading{ 0.0f, 0.0f, 1.0f };
        float health = 6.0f;
        //! \brief Seconds before choosing another way to wander.
        float wander = 0.0f;
        //! \brief Seconds before the next shot or punch.
        float cooldown = 0.0f;
        //! \brief Seconds into the current punch, the stagger, or death.
        float action = 0.0f;
        //! \brief The punch already landed.
        bool struck = false;
        //! \brief For a soldier: its gun, where the shot leaves, and the
        //! bones turned to aim.
        compages::world::Entity gun;
        compages::world::Entity muzzle;
        compages::world::EntityId right_arm;
        compages::world::EntityId left_arm;
        compages::world::EntityId right_forearm;
        compages::world::EntityId left_forearm;
    };

    struct Torch
    {
        compages::world::Entity light;
        compages::world::Entity flame;
        compages::core::Vector3f at;
        bool lit = true;
    };

    struct Barrel
    {
        compages::world::Entity body;
        compages::core::Vector3f at;
        float health = 3.0f;
        //! \brief Seconds before it goes off, set by a blast nearby.
        float fuse = -1.0f;
        bool gone = false;
    };

    struct Pickup
    {
        compages::world::Entity body;
        compages::core::Vector3f at;
        //! \brief 'A' shells, 'H' health.
        char kind;
        bool taken = false;
    };

    //! \brief A shot of a soldier, slow enough to be seen and dodged.
    struct Bolt
    {
        compages::core::Vector3f position;
        compages::core::Vector3f velocity;
        float life;
    };

    //! \brief The light of an explosion, fading.
    struct Blast
    {
        compages::world::Entity light;
        float age = 0.0f;
    };

    enum class Gun
    {
        Idle,
        Firing,
        Reloading,
    };

    //! \brief What a shot of the player met first.
    struct Target
    {
        float distance;
        compages::core::Vector3f point;
        compages::core::Vector3f normal;
        Enemy* enemy = nullptr;
        Barrel* barrel = nullptr;
    };

    //! \brief Where the run is: level index, win or death, score.
    struct Session
    {
        State state = State::Playing;
        std::size_t level = 0u;
        std::size_t kills = 0u;
        //! \brief Seconds left showing the name of the level in large.
        float banner = 0.0f;
    };

    //! \brief The body on the map: position, view angles, health, ammo.
    struct PlayerState
    {
        compages::core::Vector3f position{ 0.0f, 0.0f, 0.0f };
        float yaw = 0.0f;
        float pitch = 0.0f;
        float health = 100.0f;
        int shells = 8;
        int reserve = 24;
        Gun gun = Gun::Idle;
        float gun_time = 0.0f;
        bool casing_thrown = true;
        //! \brief Held trigger: fire again as soon as the gun is ready.
        bool trigger = false;
        //! \brief Seconds since the level started, for what turns and bobs.
        float clock = 0.0f;
        float bob = 0.0f;
        float walking = 0.0f;
        bool lantern_on = true;
        //! \brief Red after a hit, flash after a shot, shake after a blast.
        float hurt = 0.0f;
        float flash = 0.0f;
        float shake = 0.0f;
    };

    //! \brief Camera, lantern and shotgun in the Scene graph.
    struct PlayerRig
    {
        compages::world::Entity camera;
        compages::world::Entity lantern;
        compages::world::Entity weapon;
        compages::world::Entity muzzle;
    };

    //! \brief Short messages and the keys of the previous frame.
    struct UiState
    {
        std::string message;
        float message_time = 0.0f;
        compages::world::Input previous;
    };

    //! \brief Everyone and everything placed from the level grid.
    struct LevelCast
    {
        std::vector<Enemy> enemies;
        std::vector<Torch> torches;
        std::vector<Barrel> barrels;
        std::vector<Pickup> pickups;
        std::vector<Bolt> bolts;
        std::vector<Blast> blasts;
        compages::core::Vector3f exit{ 0.0f, 0.0f, 0.0f };
    };

    //! \brief glTF and textures loaded once for both levels.
    struct Content
    {
        compages::renderer::AssetManager assets;
        compages::renderer::PrefabId soldier_prefab;
        compages::renderer::PrefabId robot_prefab;
        compages::renderer::PrefabId shotgun_prefab;
        doom::Effects effects;
        doom::Map map;
    };

    //! \brief The active World and Scene, rebuilt per level.
    struct LevelScene
    {
        std::unique_ptr<compages::world::World> world;
        std::unique_ptr<compages::renderer::Scene> scene;
    };

    // The levels (53_DoomLike.cpp).
    [[nodiscard]] compages::Status buildLevel(std::size_t p_level);
    void beginLevel(std::size_t p_level);
    [[nodiscard]] compages::Status setupPlayerRig();
    void configureAtmosphere(doom::LevelData const& p_level);
    [[nodiscard]] compages::Status
    scanMapGrid(doom::LevelData const& p_level,
                std::string const& p_wall_file,
                std::string const& p_floor_file,
                std::string const& p_crate_file,
                std::string const& p_hazard_file);
    [[nodiscard]] compages::Status
    placeCell(char p_cell,
              std::size_t p_column,
              std::size_t p_row,
              compages::renderer::Look const& p_wall,
              compages::renderer::Look const& p_ceiling,
              compages::renderer::Look const& p_floor,
              compages::renderer::Look const& p_crate,
              compages::renderer::Look const& p_hazard);
    void placeTorch(std::size_t p_column, std::size_t p_row);
    [[nodiscard]] compages::Status placeEnemy(Kind p_kind,
                                              compages::core::Vector3f p_at);
    void say(std::string p_message);

    void tickTimers(float p_dt);
    void tickPlaying(compages::world::ViewFrame const& p_frame, float p_dt);
    void tickAfterDeath(compages::world::ViewFrame const& p_frame);
    void updateBlastLights(float p_dt);
    void drawAimCrosshair();

    // The player (DoomLike/Player.cpp).
    void movePlayer(compages::world::ViewFrame const& p_frame);
    void placeView(compages::world::ViewFrame const& p_frame);
    void useWeapon(compages::world::ViewFrame const& p_frame);
    void fire();
    void reload();
    void useTorch();
    void pickUp(float p_dt);
    [[nodiscard]] std::optional<Target>
    shootRay(compages::core::Vector3f p_origin,
             compages::core::Vector3f p_direction);
    //! \brief The torch under the crosshair, close enough to reach.
    [[nodiscard]] std::optional<std::size_t> torchAimedAt() const;
    [[nodiscard]] compages::core::Vector3f eye() const;
    [[nodiscard]] compages::core::Vector3f aim() const;

    // The enemies (DoomLike/Enemies.cpp).
    void moveEnemies(float p_dt);
    void aimArms();
    void moveBolts(float p_dt);
    void moveBarrels(float p_dt);
    void damageEnemy(Enemy& p_enemy,
                     float p_damage,
                     compages::core::Vector3f p_point,
                     compages::core::Vector3f p_direction);
    void explode(Barrel& p_barrel);
    void hurtPlayer(float p_damage);

    // The clips of a model.
    void playOnce(compages::world::EntityId p_model, std::string_view p_clip);
    void playLoop(compages::world::EntityId p_model, std::string_view p_clip);
    [[nodiscard]] float clipLength(compages::world::EntityId p_model,
                                   std::string_view p_clip) const;
    [[nodiscard]] compages::world::EntityId
    findNamed(compages::world::EntityId p_root, std::string_view p_name) const;

    Content m_content;
    LevelScene m_level_scene;
    Session m_session;
    PlayerState m_player;
    PlayerRig m_rig;
    LevelCast m_cast;
    UiState m_ui;
    compages::world::KeyMap m_keys = compages::world::KeyMap::defaults();
};

} // namespace examples
