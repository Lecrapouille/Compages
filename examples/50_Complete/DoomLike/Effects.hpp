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

#include "Compages/GPU/Drawable.hpp"
#include "Compages/Renderer/Scene.hpp"

#include <array>
#include <deque>
#include <random>
#include <vector>

namespace examples::doom
{

//! \brief What a mark left on a surface is.
enum class Mark
{
    Blood,
    Hole,
    Scorch,
    Oil,
};

// ****************************************************************************
//! \brief Everything that is not the world itself: particles of blood, of
//! fire, of smoke and sparks, the streaks of the shots, and the marks left on
//! the walls, the floor and the bodies.
//!
//! The particles are points drawn by the GPU layer, after the Scene, with the
//! camera the Scene used: one drawable adding light for what glows, one
//! blending for what hides. The marks are entities of the Scene, copies of a
//! few splash-shaped meshes built once per level, the oldest reused when too
//! many are left around.
// ****************************************************************************
class Effects
{
public:

    //! \brief The shaders. Once, when the game is set up.
    [[nodiscard]] compages::Status setUp();

    //! \brief Forget the previous level and make the marks of this one.
    void reset(compages::renderer::Scene& p_scene);

    //! \brief A spurt of blood leaving \c p_at toward \c p_direction.
    void blood(compages::core::Vector3f p_at,
               compages::core::Vector3f p_direction,
               int p_count);
    //! \brief Sparks off a hard surface, and a puff of dust.
    void sparks(compages::core::Vector3f p_at,
                compages::core::Vector3f p_normal,
                int p_count);
    //! \brief The flash of a shot at the muzzle, toward \c p_direction.
    void muzzle(compages::core::Vector3f p_at,
                compages::core::Vector3f p_direction);
    //! \brief A spent shell thrown to the right of the gun.
    void casing(compages::core::Vector3f p_at,
                compages::core::Vector3f p_right);
    //! \brief Fire, smoke and sparks in every direction.
    void explosion(compages::core::Vector3f p_at);
    //! \brief A glowing ball, what a projectile looks like as it flies.
    void glow(compages::core::Vector3f p_at,
              compages::core::Vector3f p_color,
              float p_size);
    //! \brief The streak of a shot, fading in \c p_life seconds.
    void tracer(compages::core::Vector3f p_from,
                compages::core::Vector3f p_to,
                compages::core::Vector3f p_color,
                float p_life);

    //! \brief A mark on a wall, the floor or the ceiling. \c p_normal leaves
    //! the surface.
    void mark(Mark p_mark,
              compages::core::Vector3f p_point,
              compages::core::Vector3f p_normal,
              float p_size);
    //! \brief A pool spreading on the floor under a body.
    void pool(Mark p_mark, compages::core::Vector3f p_at, float p_size);
    //! \brief A splash on a body, held by the bone nearest to \c p_point so
    //! that it follows the animation.
    void markBody(Mark p_mark,
                  compages::world::EntityId p_body,
                  compages::core::Vector3f p_point,
                  float p_size);

    //! \brief Move the particles, fade the streaks, spread the pools.
    void update(float p_dt);

    //! \brief The streaks, as lines of the debug drawing: before render().
    void lines(compages::renderer::DebugDraw& p_debug) const;

    //! \brief The particles: after render(), in the same pass, with the
    //! camera it used.
    void draw(compages::renderer::CameraFrame const& p_camera,
              float p_fov_degrees,
              float p_height);

    //! \brief A number between the two, from the generator of the effects.
    [[nodiscard]] float random(float p_low, float p_high);

private:

    struct Particle
    {
        compages::core::Vector3f position;
        compages::core::Vector3f velocity;
        compages::core::Vector3f color;
        float size;
        float life;
        float age = 0.0f;
        float gravity = 0.0f;
        float drag = 0.0f;
        float grow = 0.0f;
        bool glows = false;
    };

    struct Tracer
    {
        compages::core::Vector3f from;
        compages::core::Vector3f to;
        compages::core::Vector3f color;
        float life;
        float age = 0.0f;
    };

    struct Spread
    {
        compages::world::Entity entity;
        float size;
        float target;
    };

    //! \brief One vertex per particle, what the shader reads.
    struct Point
    {
        compages::core::Vector3f position;
        compages::core::Vector4f color;
        float size;
    };

    void emit(Particle p_particle);
    [[nodiscard]] compages::core::Vector3f randomDirection();
    [[nodiscard]] compages::world::Entity takeMark(Mark p_mark);

    static constexpr std::size_t MARKS = 4u;
    static constexpr std::size_t SHAPES = 3u;
    static constexpr std::size_t MARKS_KEPT = 70u;

    compages::renderer::Scene* m_scene = nullptr;
    std::vector<Particle> m_particles;
    std::vector<Tracer> m_tracers;
    std::vector<Spread> m_spreading;
    //! \brief Per kind of mark: the hidden meshes copied, and the copies
    //! placed, the oldest first.
    std::array<std::array<compages::world::EntityId, SHAPES>, MARKS> m_shapes{};
    std::array<compages::world::EntityId, MARKS> m_drops{};
    std::array<std::deque<compages::world::Entity>, MARKS> m_placed;
    std::deque<compages::world::Entity> m_body_marks;
    compages::gpu::Drawable m_glowing;
    compages::gpu::Drawable m_hiding;
    std::mt19937 m_random{ 53u };
};

} // namespace examples::doom
