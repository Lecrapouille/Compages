// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Renderer/Scene/Scene.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A rocket from the Earth to the Moon, at the real scale: a metre
//! next to 384 400 km.
//!
//! Three things make such a scale work, and this demo shows each:
//! - the zoom of the wheel is logarithmic: each notch is the same part of
//!   the way (Shift: six notches), from the rocket to a view of both worlds;
//! - the Scene stores the depth reversed (PassDesc::reversed_depth), so the
//!   near plane can be half a metre and the far one two million kilometres
//!   without far faces flickering;
//! - the floating origin: a float has 24 bits, so near the Moon two
//!   positions in a float are 32 m apart. The simulation keeps its positions
//!   in double, and each frame writes into the World their offsets from the
//!   rocket, which stay small:
//! \code
//! m_earth.position(toFloat(EARTH - rocket));   // the rocket is at the origin
//! m_moon.position(toFloat(MOON - rocket));
//! \endcode
//! Untick "Floating origin" in Try it, follow the rocket to the Moon and
//! zoom in: the rocket and the camera jump by tens of metres.
// ****************************************************************************
class EarthMoon: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "10d_EarthMoon";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;
    [[nodiscard]] std::string hud() const override;

    //! \brief A position of the simulation, in metres, in double.
    struct Point
    {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    };

private:

    //! \brief Write the double positions into the World, from the floating
    //! origin or from the centre of the Earth.
    //! \param[in] p_s where on the path, 0 on the pad, 1 near the Moon.
    void place(double p_s);

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_earth;
    compages::world::Entity m_moon;
    compages::world::Entity m_rocket;
    compages::world::Entity m_camera;

    //! \brief Where the rocket is on its path, 0 on the pad, 1 near the Moon.
    double m_progress = 0.0;
    float m_mission_seconds = 90.0f;
    bool m_running = true;
    bool m_floating_origin = true;
    Point m_position;
    Point m_heading{ 1.0, 0.0, 0.0 };
};

} // namespace examples
