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

#include "30_WorldAndAssets/33a_TexturedSpheres.hpp"
#include "Common/DataPath.hpp"

namespace examples
{

//------------------------------------------------------------------------------
std::string TexturedSpheres::description() const
{
    return "Three spheres: a red one, one wearing a picture, a picture tinted "
           "blue. The camera turns slowly by itself; right drag turns it too.";
}

//------------------------------------------------------------------------------
compages::Status TexturedSpheres::setUp()
{
    // The camera turns by itself; right drag adds to that.
    m_scene.background(0.08f, 0.10f, 0.14f).ambient(0.16f, 0.16f, 0.18f);
    m_scene.camera()
        .position(0.0f, 2.5f, 9.0f)
        .add<compages::world::Orbit>(
            compages::core::Vector3f(0.0f, 0.5f, 0.0f));
    m_scene.activeCamera().get<compages::world::Orbit>().spin = 0.35f;
    m_scene.sun();

    // A colour, a picture, and the same picture multiplied by a tint.
    const std::string grass = dataPath("grassFlowers.png");
    compages::renderer::Look tinted = compages::renderer::texture(grass);
    tinted.color = compages::core::Vector3f(0.45f, 0.6f, 1.0f);

    m_spheres.emplace_back(
        m_scene.sphere("Red", compages::renderer::color(0.85f, 0.25f, 0.20f)));
    m_spheres.emplace_back(
        m_scene.sphere("Grass", compages::renderer::texture(grass)));
    m_spheres.emplace_back(m_scene.sphere("Tinted", tinted));
    for (std::size_t i = 0u; i < m_spheres.size(); ++i)
    {
        m_spheres[i].position((float(i) - 1.0f) * 2.5f, 0.5f, 0.0f).scale(1.8f);
    }
    return m_scene.prepare();
}

//------------------------------------------------------------------------------
void TexturedSpheres::draw(compages::world::ViewFrame const& p_frame)
{
    // Each sphere a little faster than the one before it.
    for (std::size_t i = 0u; i < m_spheres.size(); ++i)
    {
        m_spheres[i].rotate((0.4f + 0.1f * float(i)) * p_frame.elapsed,
                            { 0.0f, 1.0f, 0.0f });
    }
    m_scene.draw(p_frame);
}

} // namespace examples
