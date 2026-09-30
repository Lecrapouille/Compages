// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/World/Controllers/CameraInput.hpp"
#include "Compages/World/Controllers/FlyController.hpp"
#include "Compages/World/Controllers/OrbitController.hpp"
#include "Compages/World/World.hpp"

// ****************************************************************************
//! \file
//! \brief Camera controls, as behaviors added to the camera entity.
//!
//! \code
//! m_scene.camera().position(0, 2, 6).add<compages::world::Orbit>();   // look at 0,0,0
//! \endcode
//!
//! They start from wherever the entity already is, so placing the camera
//! first and adding the control after keeps the placement.
// ****************************************************************************

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
namespace compages::world
{



// ****************************************************************************
//! \brief Turn around a target with the right mouse button, zoom with the
//! wheel.
//!
//! \code
//! scene.camera().add<compages::world::Orbit>(compages::core::Vector3f{ 0, 1, 0 });
//! \endcode
// ****************************************************************************
struct Orbit : Behavior
{
    //! \brief The tuning: the target, the sensitivities, the limits.
    OrbitController controller;

    Orbit() = default;
    explicit Orbit(compages::core::Vector3f p_target) { controller.target = p_target; }

    //! \brief Seed distance, zoom limits and yaw/pitch from the camera pose.
    void start() override;

    //! \brief Apply orbit input; optional \c spin when the user is not looking.
    //! \param[in] p_dt Seconds since the previous frame.
    void update(float p_dt) override;

    //! \brief Radians a second the camera turns by itself while the user
    //! does not turn it, like a turntable. Zero keeps it still.
    float spin = 0.0f;
};

// ****************************************************************************
//! \brief Fly freely: the right mouse button looks around, W A S D move, E
//! and Q go up and down, Shift runs.
//!
//! \code
//! scene.camera().position(0, 2, 5).add<compages::world::Fly>();
//! \endcode
// ****************************************************************************
struct Fly : Behavior
{
    //! \brief The tuning: the speeds, the sensitivity, the limits.
    FlyController controller;

    //! \brief Seed yaw/pitch from the camera's current forward (-Z).
    void start() override;

    //! \brief Apply fly look and movement from keyboard and mouse input.
    //! \param[in] p_dt Seconds since the previous frame.
    void update(float p_dt) override;
};

} // namespace compages::world
