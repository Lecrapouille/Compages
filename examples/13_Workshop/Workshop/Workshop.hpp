// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The robot workshop of the tutorials (doc/tutorials/), on the side
//! of the Renderer: the room, the shapes of the arm, the hot plate, the
//! paint, the camera of the robot. The arm itself, its joints and its
//! routine, is in Arm.hpp and needs only the World.
//!
//! | Step | Adds                                         | Demo           |
//! |------|----------------------------------------------|----------------|
//! | 1    | buildRoom(), dressArm(): the scene drawn     | 13a_Workshop   |
//! | 2    | HotPlate: values on a grid                   | 13b_HotPlate   |
//! | 3    | paintArm(): a look of your own               | 13c_PaintedArm |
//! | 4    | RobotEye: what the robot sees                | 13d_RobotEye   |
// ****************************************************************************

#include "13_Workshop/Workshop/Arm.hpp"

#include "Compages/Fields/Field.hpp"
#include "Compages/Renderer/Render/RenderTarget.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

namespace examples::workshop
{

// ----------------------------------------------------------------------------
//! \brief What the room is made of, for the steps after it.
// ----------------------------------------------------------------------------
struct Room
{
    compages::world::Entity table;
    //! \brief The blue part on the table, beyond the reach of the arm:
    //! what its camera looks for (step 4).
    compages::world::Entity part;
};

// ----------------------------------------------------------------------------
//! \brief The floor, a table, a blue part, a lamp, the sun, a camera.
// ----------------------------------------------------------------------------
Room buildRoom(compages::renderer::Scene& p_scene);

// ----------------------------------------------------------------------------
//! \brief Give the joints of \c p_arm their shapes: a foot, two links, a
//! sphere at the elbow, a tip. Returns the two links, for a look of their own.
// ----------------------------------------------------------------------------
std::vector<compages::world::Entity> dressArm(compages::renderer::Scene& p_scene, Arm const& p_arm);

// ****************************************************************************
//! \brief Step 2: the temperature of a steel plate on the table, a Field on
//! a Grid2D. The tool heats it where it touches; the heat spreads and cools.
// ****************************************************************************
class HotPlate
{
public:

    //! \brief The field at room temperature, shown on the top of the table.
    void create(compages::renderer::Scene& p_scene);
    //! \brief \c p_seconds of heat: from \c p_tool if it touches, then spread.
    void step(float p_seconds, compages::Vector3f const& p_tool);
    //! \brief Run \c p_seconds of work before the first frame.
    void warmUp(compages::renderer::Scene& p_scene, Arm const& p_arm, float p_seconds);
    //! \brief Send the new values to the picture.
    [[nodiscard]] compages::Status show(compages::renderer::Scene& p_scene);

private:

    compages::Field<float> m_temperature;
    compages::Field<float> m_next;
    compages::renderer::FieldLook m_look{ .colors = compages::renderer::ColorMap::Inferno,
                                          .low = 20.0f,
                                          .high = 300.0f,
                                          .relief = 0.0f,
                                          .lit = true };
    compages::world::Entity m_shown;
};

// ----------------------------------------------------------------------------
//! \brief Step 3: orange paint worn down to the steel along the edges of
//! the links, from the hook "surface" of the PBR shader. The lights, the
//! shadows and the fog stay those of the Scene.
// ----------------------------------------------------------------------------
[[nodiscard]] compages::Status paintArm(compages::renderer::Scene& p_scene,
                                        std::vector<compages::world::Entity> const& p_links);

// ****************************************************************************
//! \brief Step 4: a camera on a mast of the arm. It is drawn in a corner of the
//! window, and into a picture of its own that the program reads: how much of
//! the blue part does the robot see?
// ****************************************************************************
class RobotEye
{
public:

    [[nodiscard]] compages::Status create(compages::renderer::Scene& p_scene, Arm const& p_arm);
    //! \brief Draw into the picture of the robot, and read it four times
    //! a second (\c p_elapsed: the seconds since the last frame).
    [[nodiscard]] compages::Status look(compages::renderer::Scene& p_scene, float p_elapsed);
    //! \brief Draw the view of the robot in its corner of the window.
    void show(compages::renderer::Scene& p_scene);
    //! \brief The share of the picture of the robot that is the blue part.
    [[nodiscard]] float seen() const
    {
        return m_seen;
    }

private:

    compages::world::Entity m_camera;
    compages::renderer::RenderTarget m_picture;
    float m_wait = 0.0f;
    float m_seen = 0.0f;
};

} // namespace examples::workshop
