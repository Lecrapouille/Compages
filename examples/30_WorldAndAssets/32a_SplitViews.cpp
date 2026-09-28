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

#include "30_WorldAndAssets/32a_SplitViews.hpp"

#include <cmath>

namespace examples
{

//------------------------------------------------------------------------------
std::string SplitViews::description() const
{
    return "One World drawn twice per frame: an orbiting perspective camera "
           "on the left, an orthographic top-down map on the right. The World "
           "moves once; each camera draws into its own half.";
}

//------------------------------------------------------------------------------
compages::gpu::Status SplitViews::setUp()
{
    m_scene.background(0.05f, 0.08f, 0.20f);
    m_map.background(0.10f, 0.10f, 0.10f);
    m_scene.sun();

    // A pillar turning in the middle of a ring of towers, on flat ground.
    m_scene.box("Ground", compages::renderer::color(0.20f, 0.28f, 0.20f))
        .position(0.0f, -0.5f, 0.0f)
        .scale(140.0f, 1.0f, 140.0f);
    m_pillar = m_scene.box("Pillar", compages::renderer::color(0.85f, 0.55f, 0.35f))
                   .position(0.0f, 8.0f, 0.0f)
                   .scale(4.0f, 16.0f, 4.0f);
    for (int i = 0; i < 8; ++i)
    {
        const float a = 6.2831853f * float(i) / 8.0f;
        m_scene.box("Tower", compages::renderer::color(0.55f, 0.60f, 0.70f))
            .position(40.0f * std::cos(a), 6.0f, 40.0f * std::sin(a))
            .scale(4.0f, 12.0f, 4.0f);
    }

    // The left half: a perspective camera, that the right mouse button turns.
    m_eye = m_scene.camera("Eye").position(30.0f, 55.0f, 85.0f);
    m_eye.get<compages::world::Camera>().viewport = { 0.0f, 0.0f, 0.5f, 1.0f };
    m_eye.add<compages::world::Orbit>(Vector3f(0.0f, 8.0f, 0.0f));

    // The right half: an orthographic camera straight above, 80 units across
    // half its height, so that the whole ring fits.
    m_top = m_scene.camera("Top")
                .position(0.0f, 120.0f, 0.0f)
                .rotation(-1.5707963f, { 1.0f, 0.0f, 0.0f })
                .set(compages::world::Camera{ .projection =
                                        compages::world::Camera::Projection::Orthographic,
                                    .ortho_half_height = 80.0f,
                                    .near_plane = 1.0f,
                                    .far_plane = 300.0f,
                                    .viewport = { 0.5f, 0.0f, 0.5f, 1.0f } });
    return m_scene.prepare();
}

//------------------------------------------------------------------------------
void SplitViews::draw(Frame const& p_frame)
{
    // The pillar turns, the World moves once, then each camera draws its half.
    // draw() would update and render together, and there are two cameras.
    m_pillar.rotate(0.6f * p_frame.elapsed, { 0.0f, 1.0f, 0.0f });
    m_scene.update(p_frame);
    m_scene.render(m_eye);
    m_map.render(m_top);
}

} // namespace examples
