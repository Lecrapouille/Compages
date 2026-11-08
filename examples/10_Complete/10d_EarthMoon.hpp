// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Core/Units.hpp"
#include "Compages/Core/Vector.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

#include <vector>

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
//!   near plane can be half a metre and the far one 130 astronomical units
//!   without far faces flickering;
//! - the floating origin: a float has 24 bits, so near the Moon two
//!   positions in a float are 32 m apart. The simulation keeps its positions
//!   in double, and each frame writes into the World their offsets from the
//!   rocket, which stay small:
//! \code
//! m_earth.position(compages::toMetres(EARTH - rocket));   // the rocket is at 0
//! m_moon.position(compages::toMetres(MOON - rocket));
//! \endcode
//! Untick "Floating origin" in Try it, follow the rocket to the Moon and
//! zoom in: the rocket and the camera jump by tens of metres.
//!
//! The Sun and the seven other planets are there too, at their real sizes
//! and distances (Neptune is 4.5 billion km away), where they are on the
//! chosen date (circular orbits). "Look at" moves the floating origin to
//! the chosen world: the origin goes where the camera looks.
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

    //! \brief The Sun or a planet other than the Earth: what is fixed, and
    //! where it is in the simulation.
    struct Body
    {
        char const* name;
        //! \brief Picture in external/Compages-data/planets/, or none (the Sun).
        char const* file;
        //! \brief Mean radius.
        units::length::meter_t radius;
        //! \brief Mean distance to the Sun (0 for the Sun).
        units::length::meter_t distance;
        //! \brief Mean longitude on 1 January 2000 at noon (J2000).
        units::angle::degree_t longitude;
        //! \brief Time of one orbit.
        units::time::day_t period;
        //! \brief Direction of its north pole in the sky (IAU): right
        //! ascension and declination. What tilts Saturn's rings.
        units::angle::degree_t pole_ra;
        units::angle::degree_t pole_dec;
        //! \brief Placed and tilted; the sphere (and the rings) hang from it.
        compages::world::Entity pivot{};
        //! \brief Its centre in the simulation, in metres, in double.
        compages::Vector3m at = compages::Vector3m(units::length::meter_t(0.0));
    };

private:

    //! \brief Move the rocket along its path, then write the double positions
    //! into the World, from the floating origin or from the centre of the
    //! Earth.
    //! \param[in] p_s Where on the path, 0 on the pad, 1 near the Moon.
    void place(double p_s);

    //! \brief What the caption of the camera says this frame.
    //! \return The altitude, the distance to the Moon, what the camera looks
    //! at and from how far, and where the origin is.
    [[nodiscard]] std::string status() const;

    //! \brief Where the Sun and the planets are at the date m_date.
    void movePlanets();

    //! \brief Point the camera at a thing, and frame it.
    //! \param[in] p_focus Its number in FOCUS_NAMES (clamped).
    void lookAt(std::size_t p_focus);

    //! \brief A thing to look at.
    struct Focus
    {
        //! \brief Its centre in the simulation, in metres, in double.
        compages::Vector3m centre;
        //! \brief Its radius: what the camera frames.
        units::length::meter_t radius;
    };

    //! \brief Where a thing to look at is, and how large.
    //! \param[in] p_focus Its number in FOCUS_NAMES.
    //! \return Its centre and its radius.
    [[nodiscard]] Focus focus(std::size_t p_focus) const;

    //! \brief A position of the simulation as the World stores it.
    //! \param[in] p_position In metres, in double.
    //! \return Its offset from the floating origin m_origin, in float metres.
    [[nodiscard]] compages::Vector3f fromOrigin(compages::Vector3m const& p_position) const
    {
        return compages::toMetres(p_position - m_origin);
    }

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_earth;
    compages::world::Entity m_moon;
    compages::world::Entity m_rocket;
    compages::world::Entity m_camera;
    compages::world::Entity m_sunlight;
    std::vector<Body> m_bodies;

    //! \brief Where the rocket is on its path, 0 on the pad, 1 near the Moon.
    double m_progress = 0.0;
    float m_mission_seconds = 90.0f;
    bool m_running = true;
    bool m_floating_origin = true;
    bool m_orbits = true;
    bool m_trajectory = true;
    //! \brief 0 the rocket, 1 the Earth, 2 the Moon, then m_bodies.
    std::size_t m_focus = 0u;
    //! \brief The date, as the time since 1 January 2000 at noon (J2000).
    units::time::day_t m_date{ 0.0 };
    //! \brief How fast the date runs, in days per second (a float for the
    //! slider): Mercury goes round in 88 days.
    float m_days_per_second = 5.0f;
    //! \brief What is written at the origin of the World this frame.
    compages::Vector3m m_origin = compages::Vector3m(units::length::meter_t(0.0));
    //! \brief Where the rocket is.
    compages::Vector3m m_position = compages::Vector3m(units::length::meter_t(0.0));
    //! \brief Where it goes: the tangent of its path.
    compages::Vector3m m_heading = compages::Vector3m(units::length::meter_t(0.0));
};

} // namespace examples
