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

#include "30_WorldAndAssets/32c_MiscLookAt.hpp"

#include "Compages/Renderer/Assets/Primitives.hpp"

#include <cmath>
#include <random>

namespace examples
{

//------------------------------------------------------------------------------
std::string MiscLookAt::description() const
{
    return "Three.js misc_lookat: a thousand cones look at a sphere moving on "
           "a Lissajous path, while the camera eases toward the mouse.";
}

//------------------------------------------------------------------------------
compages::gpu::Status MiscLookAt::setUp()
{
    // White, so the cones read as the Three.js demo they come from.
    m_scene.background(1.0f, 1.0f, 1.0f).ambient(0.45f, 0.45f, 0.45f);
    m_scene.environment().default_light_direction = { 0.3f, -0.8f, -0.5f };

    m_camera = m_scene.camera().position(0.0f, 0.0f, 3200.0f);
    m_camera.set(compages::world::Camera{ .fov = units::angle::degree_t(40.0),
                                .near_plane = 1.0f,
                                .far_plane = 15000.0f });

    // Painted with its normals, so it stays visible against the white.
    m_sphere = m_scene.sphere("Target", compages::renderer::normals()).scale(200.0f);

    // One cone, then nine hundred and ninety nine copies of it.
    compages::renderer::MeshAsset cone;
    COMPAGES_TRY_ASSIGN(cone, compages::renderer::makeCone(10.0f, 0.0f, 100.0f, 12u));
    compages::world::Entity first = m_scene.mesh(
        std::move(cone), "Cone", compages::renderer::color(0.55f, 0.55f, 0.58f));
    std::mt19937 random(42u);
    std::uniform_real_distribution<float> place(-2000.0f, 2000.0f);
    std::uniform_real_distribution<float> size(2.0f, 6.0f);
    for (int i = 0; i < 1000; ++i)
    {
        compages::world::Entity cone_i = (i == 0) ? first : m_scene.copy(first);
        m_cones.emplace_back(
            cone_i.position(place(random), place(random), place(random))
                .scale(size(random)));
    }
    return m_scene.prepare();
}

//------------------------------------------------------------------------------
void MiscLookAt::draw(Frame const& p_frame)
{
    // A Lissajous path. Every cone is aimed at the same point.
    const float t = p_frame.total;
    const Vector3f target(std::sin(t * 0.7f) * 900.0f - 25.0f,
                          std::cos(t * 0.5f) * 400.0f - 25.0f,
                          std::cos(t * 0.3f) * 900.0f - 25.0f);
    m_sphere.position(target);
    for (compages::world::Entity& cone : m_cones)
    {
        cone.lookAt(target);
    }

    // The camera slides a little toward the mouse each frame, and keeps
    // looking at the middle.
    const Vector2f mouse =
        p_frame.input.mouse_over
            ? (p_frame.input.mouse -
               Vector2f(float(p_frame.width), float(p_frame.height)) * 0.5f) *
                  0.25f
            : Vector2f(0.0f, 0.0f);
    Vector3f eye = m_camera.position();
    eye.x += (mouse.x - eye.x) * 0.05f;
    eye.y += (mouse.y - eye.y) * 0.05f;
    m_camera.position(eye).lookAt(0.0f, 0.0f, 0.0f);

    m_scene.draw(p_frame);
}

} // namespace examples
