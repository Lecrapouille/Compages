// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Vector.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace examples::doom
{

//! \brief The side of one character of the map, in metres, the height of the
//! walls, and the size of a crate.
constexpr float CELL = 3.0f;
constexpr float WALL_HEIGHT = 3.6f;
constexpr float CRATE = 2.4f;
constexpr float PI = 3.14159265f;

//! \brief The bodies: how high the eye is, how wide the player is, and the
//! size of the enemies, what the shots are tested against. The robot of
//! RobotExpressive.glb is about four metres tall: it is scaled down.
constexpr float EYE = 1.6f;
constexpr float PLAYER_RADIUS = 0.45f;
constexpr float SOLDIER_RADIUS = 0.35f;
constexpr float SOLDIER_HEIGHT = 1.8f;
constexpr float ROBOT_SCALE = 0.42f;
constexpr float ROBOT_RADIUS = 0.45f;
constexpr float ROBOT_HEIGHT = 1.8f;

// ****************************************************************************
//! \brief What a level is made of: its grid, and what it looks like.
//!
//! Each character of \c rows is a cell of CELL metres: rows go along +Z,
//! columns along +X. '#' a wall, 'C' a crate, 'T' a torch, 'B' an explosive
//! barrel, 'A' shells, 'H' a medical kit, 'M' a soldier, 'R' a robot, 'S'
//! where the player starts and 'E' the way out.
// ****************************************************************************
struct LevelData
{
    char const* title;
    char const* wall;
    char const* floor;
    Vector3f wall_tint;
    Vector3f fog;
    std::vector<std::string> rows;
};

//! \brief The two levels of the game.
[[nodiscard]] std::array<LevelData, 2u> const& levels();

// ****************************************************************************
//! \brief Where a ray met a wall, a crate, the floor or the ceiling.
// ****************************************************************************
struct Hit
{
    Vector3f point;
    //! \brief Out of the surface met, toward where the ray came from.
    Vector3f normal;
    float distance;
};

// ****************************************************************************
//! \brief The grid of a level, and the questions the game asks it: can one
//! stand here, can one see there, where does a shot land.
// ****************************************************************************
class Map
{
public:

    void load(std::vector<std::string> p_rows) { m_rows = std::move(p_rows); }
    [[nodiscard]] std::vector<std::string> const& rows() const { return m_rows; }

    //! \brief The character at a cell, '#' outside the map.
    [[nodiscard]] char at(long p_column, long p_row) const;

    //! \brief The middle of a cell, on the floor.
    [[nodiscard]] static Vector3f center(std::size_t p_column, std::size_t p_row)
    {
        return Vector3f(float(p_column) * CELL, 0.0f, float(p_row) * CELL);
    }

    //! \brief Is this point of the floor inside a wall or a crate?
    [[nodiscard]] bool solid(float p_x, float p_z) const;

    //! \brief Move a body of radius \c p_radius by \c p_step, sliding along
    //! what it meets rather than stopping dead.
    [[nodiscard]] Vector3f slide(Vector3f p_from, Vector3f p_step, float p_radius) const;

    //! \brief Nothing solid between two points of the floor?
    [[nodiscard]] bool clearLine(Vector3f p_from, Vector3f p_to) const;

    //! \brief The first wall, crate, floor or ceiling a ray meets before
    //! \c p_max metres. \c p_direction is of length one.
    [[nodiscard]] std::optional<Hit> cast(Vector3f p_origin, Vector3f p_direction,
                                          float p_max) const;

private:

    //! \brief Inside a wall, or inside the box of a crate.
    [[nodiscard]] bool blocks(Vector3f const& p_point) const;

    std::vector<std::string> m_rows;
};

} // namespace examples::doom
