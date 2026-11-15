// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The robot workshop of the tutorials (doc/tutorials/): one function
//! or class per step, each step adding one idea to the steps before it.
//!
//! | Step | Adds                                   | Demo            |
//! |------|----------------------------------------|-----------------|
//! | 1    | buildRoom(): a table, a part, a lamp   | 00a_Workshop    |
//! | 2    | buildArm(): joints and a routine       | 00b_RobotArm    |
//! | 3    | HotTable: values on a grid             | 00c_HotTable    |
//! | 4    | paintArm(): a look of your own         | 00d_PaintedArm  |
//! | 5    | RobotEye: what the robot sees          | 00e_RobotEye    |
// ****************************************************************************

#include "Compages/Fields/Field.hpp"
#include "Compages/Renderer/Render/RenderTarget.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

namespace examples::workshop
{

//! \brief The height of the top of the table, in metres.
inline constexpr float TABLE_TOP = 0.78f;

// ----------------------------------------------------------------------------
//! \brief What the room is made of, for the steps after it.
// ----------------------------------------------------------------------------
struct Room
{
    compages::world::Entity table;
    //! \brief The blue part on the table: what the robot works on.
    compages::world::Entity part;
};

// ----------------------------------------------------------------------------
//! \brief Step 1: the floor, a table, a blue part, a lamp, the sun, a camera.
// ----------------------------------------------------------------------------
Room buildRoom(compages::renderer::Scene& p_scene);

// ----------------------------------------------------------------------------
//! \brief The joints of the arm, from its base to its tool.
// ----------------------------------------------------------------------------
struct Arm
{
    compages::world::Entity base;     //!< turns round the vertical
    compages::world::Entity shoulder; //!< tilts the upper arm
    compages::world::Entity elbow;    //!< bends the forearm
    compages::world::Entity tool;     //!< the hot tip, at the end
    //! \brief The boxes and cylinders of the arm, for a look of their own.
    std::vector<compages::world::Entity> links;
};

// ----------------------------------------------------------------------------
//! \brief Step 2: an arm of three joints on the table, moved by a routine.
// ----------------------------------------------------------------------------
Arm buildArm(compages::renderer::Scene& p_scene);

// ****************************************************************************
//! \brief Step 3: the temperature of the top of the table, a Field on a
//! Grid2D. The tool heats it where it touches; the heat spreads and cools.
// ****************************************************************************
class HotTable
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
//! \brief Step 4: yellow and black stripes on the arm, from the hook
//! "surface" of the PBR shader. The lights, shadows and fog stay.
// ----------------------------------------------------------------------------
[[nodiscard]] compages::Status paintArm(compages::renderer::Scene& p_scene, Arm const& p_arm);

// ****************************************************************************
//! \brief Step 5: a camera on the tool. It is drawn in a corner of the
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
