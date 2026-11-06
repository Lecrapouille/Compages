// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief An animation that has a formula needs neither the CPU nor a compute
//! shader: the vertex shader computes it from the time.
//!
//! 13_Galaxy integrates gravity: each step depends on the one before, so the
//! state must be stored and updated (a compute shader and a PingPong). Here
//! each body rides a fixed ellipse, and its angle is theta + spin * years. Its
//! position is a function of the time only. The vertices are sent once and
//! never change; each frame only one number is written:
//! \code
//! m_points.setVertices(bodies);       // once: the ellipse of each body
//! m_points["years"] = m_years;        // each frame: the time
//! m_points.draw();
//! \endcode
//!
//! The model is Ingo Berg's density wave, in the form Nicolas Rougier
//! published with Glumpy:
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

    //! \brief One body, as the vertex shader reads it: its ellipse, not its
    //! position. Lengths are in parsecs, angles in degrees.
    struct Body
    {
        float semi_major;
        float semi_minor;
        //! \brief Tilt of the ellipse. Grows with the radius: the arms.
        float tilt;
        //! \brief Angle on the ellipse at year zero.
        float theta;
        float size;
        float kind;
        //! \brief 0 at 1000 K, 1 at 10000 K: a coordinate in the colour ramp.
        float temperature;
        float brightness;
        //! \brief For a hydrogen cloud or its core: how much larger the
        //! ellipse of the other one of the pair is. Zero for the others.
        float partner;
    };

    static std::vector<Body> seed();
    [[nodiscard]] compages::Status makeColormap();
    [[nodiscard]] compages::Status makeSprite();

    //! \brief Time of the galaxy, wrapped on one turn to keep float precise.
    float m_years = 0.0f;

    compages::gpu::Texture m_colormap;
    compages::gpu::Texture m_sprite;
    compages::gpu::Drawable m_points;
};

} // namespace examples
