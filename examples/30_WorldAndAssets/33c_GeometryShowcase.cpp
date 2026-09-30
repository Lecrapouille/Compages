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

#include "30_WorldAndAssets/33c_GeometryShowcase.hpp"

#include "Compages/Renderer/Assets/Primitives.hpp"

namespace examples
{

//------------------------------------------------------------------------------
std::string GeometryShowcase::description() const
{
    return "Box, sphere, cone, cylinder, pyramid and a tube on a plateau, "
           "drawn lit, through the textured shader, as depth and as normals. "
           "The camera turns around the plateau.";
}

//------------------------------------------------------------------------------
compages::Status GeometryShowcase::setUp()
{
    // The camera turns around the plateau by itself.
    m_scene.background(0.06f, 0.07f, 0.10f).ambient(0.22f, 0.22f, 0.24f);
    m_scene.camera().position(0.0f, 7.0f, 14.0f).add<compages::world::Orbit>();
    m_scene.activeCamera().get<compages::world::Orbit>().spin = 0.28f;
    m_scene.sun();

    // The plane stands in XY: turned a quarter around X, it lies flat.
    m_scene.plane("Plateau", compages::renderer::color(0.42f, 0.44f, 0.48f))
        .rotation(-1.5707963f, { 1.0f, 0.0f, 0.0f })
        .scale(16.0f, 12.0f, 1.0f);

    // The textured shader without a picture: it samples white.
    compages::renderer::Look through_pbr = compages::renderer::texture({});
    through_pbr.color = compages::core::Vector3f(0.28f, 0.45f, 0.92f);

    // One of each look the Scene can give a shape: a colour, a tinted
    // texture, a depth ramp, and the normals.
    m_props.emplace_back(
        m_scene.box("cube", compages::renderer::color(0.85f, 0.28f, 0.24f))
            .position(-3.0f, 0.5f, -3.0f));
    m_props.emplace_back(
        m_scene.box("box", compages::renderer::color(0.30f, 0.72f, 0.38f))
            .position(0.0f, 0.35f, -3.0f)
            .scale(1.2f, 0.7f, 0.9f));
    m_props.emplace_back(m_scene.sphere("sphere", through_pbr)
                             .position(3.0f, 0.45f, -3.0f)
                             .scale(0.9f));
    m_props.emplace_back(
        m_scene.cone("cone", compages::renderer::color(0.92f, 0.55f, 0.18f))
            .position(-3.0f, 0.5f, 0.0f));
    m_props.emplace_back(
        m_scene
            .cylinder("cylinder",
                      compages::renderer::color(0.25f, 0.70f, 0.82f))
            .position(0.0f, 0.5f, 0.0f));
    m_props.emplace_back(
        m_scene
            .pyramid("pyramid", compages::renderer::color(0.90f, 0.82f, 0.25f))
            .position(3.0f, 0.5f, 0.0f)
            .scale(1.3f, 1.0f, 1.3f));
    m_props.emplace_back(
        m_scene.box("depth", compages::renderer::depth(8.0f, 22.0f))
            .position(0.0f, 0.5f, 3.0f));
    m_props.emplace_back(
        m_scene.sphere("normals", compages::renderer::normals())
            .position(3.0f, 0.42f, 3.0f)
            .scale(0.84f));

    // No shortcut for a tube: built along Z, it is stood up by its rotation.
    compages::renderer::MeshAsset tube;
    COMPAGES_TRY_ASSIGN(tube,
                        compages::renderer::makeTube(0.35f, 0.55f, 0.95f, 18u));
    m_scene.mesh(std::move(tube), "tube", compages::renderer::normals())
        .position(-3.0f, 0.48f, 3.0f)
        .rotation(1.5707963f, { 1.0f, 0.0f, 0.0f });
    return m_scene.prepare();
}

//------------------------------------------------------------------------------
void GeometryShowcase::draw(compages::world::ViewFrame const& p_frame)
{
    // The shapes turn; the plateau stays.
    for (compages::world::Entity& prop : m_props)
    {
        prop.rotate(0.45f * p_frame.elapsed, { 0.0f, 1.0f, 0.0f });
    }
    m_scene.draw(p_frame);
}

} // namespace examples
