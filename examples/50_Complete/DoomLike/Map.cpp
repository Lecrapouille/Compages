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

#include "50_Complete/DoomLike/Map.hpp"

#include <cmath>

namespace examples::doom
{

//------------------------------------------------------------------------------
std::array<LevelData, 2u> const& levels()
{
    static const std::array<LevelData, 2u> data{ {
        { "The Crypt", "rocks.png", "mud.png", compages::core::Vector3f(0.9f, 0.85f, 0.8f),
          compages::core::Vector3f(0.02f, 0.02f, 0.035f),
          { "###########",
            "#S..#..A..#",
            "#...#..M..#",
            "#.T.#.B.T.#",
            "#....R....#",
            "####.###.##",
            "#T..H..#.B#",
            "#..C.M.#.R#",
            "#A.....T..#",
            "#####E#####" } },
        { "The Depot", "rocks.png", "path.png", compages::core::Vector3f(0.55f, 0.65f, 0.9f),
          compages::core::Vector3f(0.015f, 0.025f, 0.04f),
          { "#############",
            "#S....#..A..#",
            "#.CC..#..M..#",
            "#..B..T..R..#",
            "###.#####.###",
            "#T..M...#..H#",
            "#...R...C.M.#",
            "#B.###..#...#",
            "#.M.T#..B...#",
            "######.###.##",
            "#E..R..T.AM.#",
            "#############" } },
    } };
    return data;
}

//------------------------------------------------------------------------------
char Map::at(long p_column, long p_row) const
{
    if ((p_row < 0) || (p_column < 0) || (std::size_t(p_row) >= m_rows.size()) ||
        (std::size_t(p_column) >= m_rows[std::size_t(p_row)].size()))
    {
        return '#';
    }
    return m_rows[std::size_t(p_row)][std::size_t(p_column)];
}

//------------------------------------------------------------------------------
bool Map::solid(float p_x, float p_z) const
{
    return blocks(compages::core::Vector3f(p_x, 0.5f, p_z));
}

//------------------------------------------------------------------------------
bool Map::blocks(compages::core::Vector3f const& p_point) const
{
    const long column = long(std::floor((p_point.x / CELL) + 0.5f));
    const long row = long(std::floor((p_point.z / CELL) + 0.5f));
    const char cell = at(column, row);
    if (cell == '#')
    {
        return true;
    }
    if (cell != 'C')
    {
        return false;
    }
    // A crate fills less than its cell, and is lower than the ceiling.
    const compages::core::Vector3f middle = center(std::size_t(column), std::size_t(row));
    return (std::abs(p_point.x - middle.x) < CRATE * 0.5f) &&
           (std::abs(p_point.z - middle.z) < CRATE * 0.5f) && (p_point.y < CRATE);
}

//------------------------------------------------------------------------------
compages::core::Vector3f Map::slide(compages::core::Vector3f p_from, compages::core::Vector3f p_step, float p_radius) const
{
    // One axis at a time: blocked along one, the body still slides along the
    // other, which is what makes a wall feel like a wall rather than glue.
    auto blocked = [this, p_radius](compages::core::Vector3f const& p) {
        return solid(p.x - p_radius, p.z - p_radius) || solid(p.x + p_radius, p.z - p_radius) ||
               solid(p.x - p_radius, p.z + p_radius) || solid(p.x + p_radius, p.z + p_radius);
    };
    compages::core::Vector3f position = p_from;
    const compages::core::Vector3f along_x(position.x + p_step.x, position.y, position.z);
    if (!blocked(along_x))
    {
        position = along_x;
    }
    const compages::core::Vector3f along_z(position.x, position.y, position.z + p_step.z);
    if (!blocked(along_z))
    {
        position = along_z;
    }
    return position;
}

//------------------------------------------------------------------------------
bool Map::clearLine(compages::core::Vector3f p_from, compages::core::Vector3f p_to) const
{
    const compages::core::Vector3f d = p_to - p_from;
    const float length = std::sqrt((d.x * d.x) + (d.z * d.z));
    for (float s = 0.0f; s < length; s += 0.3f)
    {
        const compages::core::Vector3f p = p_from + (d * (s / length));
        if (solid(p.x, p.z))
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
std::optional<Hit> Map::cast(compages::core::Vector3f p_origin, compages::core::Vector3f p_direction, float p_max) const
{
    // Small steps: plenty for a corridor three metres wide, and the step
    // before the one inside tells which face was crossed.
    constexpr float STEP = 0.03f;
    compages::core::Vector3f previous = p_origin;
    for (float s = STEP; s <= p_max; s += STEP)
    {
        const compages::core::Vector3f p = p_origin + (p_direction * s);
        if (p.y <= 0.0f)
        {
            return Hit{ compages::core::Vector3f(p.x, 0.0f, p.z), compages::core::Vector3f(0.0f, 1.0f, 0.0f), s };
        }
        if (p.y >= WALL_HEIGHT)
        {
            return Hit{ compages::core::Vector3f(p.x, WALL_HEIGHT, p.z), compages::core::Vector3f(0.0f, -1.0f, 0.0f), s };
        }
        if (blocks(p))
        {
            // The one coordinate that, put back, leaves the solid.
            compages::core::Vector3f normal(0.0f, 1.0f, 0.0f);
            if (!blocks(compages::core::Vector3f(previous.x, p.y, p.z)))
            {
                normal = compages::core::Vector3f((previous.x > p.x) ? 1.0f : -1.0f, 0.0f, 0.0f);
            }
            else if (!blocks(compages::core::Vector3f(p.x, p.y, previous.z)))
            {
                normal = compages::core::Vector3f(0.0f, 0.0f, (previous.z > p.z) ? 1.0f : -1.0f);
            }
            return Hit{ previous, normal, s - STEP };
        }
        previous = p;
    }
    return std::nullopt;
}

} // namespace examples::doom
