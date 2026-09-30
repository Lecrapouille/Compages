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

#include "Common/Example.hpp"

#include <cstddef>
#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief The Glumpy spiral: ellipses on the CPU, colour from a 1D ramp.
//!
//! 13_Galaxy integrates gravity. This one does not. Each body sits on an
//! ellipse whose tilt depends on its radius, which is a density wave, and the
//! CPU advances that angle. The shader never sees a temperature in kelvin. It
//! is a coordinate into a blackbody ramp, and the point itself is a sprite:
//! \code
//! m_points["temperature"] = coordinate;   // 0 at 1000 K, 1 at 10000 K
//! m_points["colormap"] = m_colormap;      // one row of texels
//! m_points["sprite"] = m_sprite;
//! m_points["position"] = positions;       // rewritten each frame
//! m_points.draw();                        // flushes what was written
//! \endcode
//!
//! The model is Ingo Berg's, in the form Nicolas Rougier published with
//! Glumpy. The ramp is John Walker's spectrum rendering (public domain).
//! https://github.com/glumpy/glumpy/blob/master/examples/galaxy.py
// ****************************************************************************
class SpiralGalaxy: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "14_SpiralGalaxy";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    //! \brief One body of the density wave. Positions stay in parsecs; the
    //! shader receives them divided by the galaxy radius.
    struct Body
    {
        float theta = 0.0f;
        float velocity = 0.0f;
        float angle = 0.0f;
        float semi_major = 0.0f;
        float semi_minor = 0.0f;
        float size = 0.0f;
        float kind = 0.0f;
        float temperature = 0.0f;
        float brightness = 0.0f;
        float x = 0.0f;
        float y = 0.0f;
    };

    void seed();
    void step();
    void flush(float p_scale);

    [[nodiscard]] compages::Status makeColormap();
    [[nodiscard]] compages::Status makeSprite();

    std::vector<Body> m_bodies;
    std::vector<compages::core::Vector2f> m_positions;
    std::vector<float> m_sizes;
    //! \brief Index of the first hydrogen cloud. The cores follow, one each.
    std::size_t m_clouds = 0u;
    std::size_t m_cloud_count = 0u;

    compages::gpu::Texture m_colormap;
    compages::gpu::Texture m_sprite;
    compages::gpu::Drawable m_points;
};

} // namespace examples
