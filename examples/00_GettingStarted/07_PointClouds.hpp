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

namespace examples
{

struct PointVertex
{
    compages::core::Vector3f position;
};

// ****************************************************************************
//! \brief A dense sphere of points: a struct with one field, and the Points
//! primitive in the render state of the drawable.
// ****************************************************************************
class PointSphere: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "07_PointClouds";
    }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    //! \brief One PointVertex per point, drawn as points rather than triangles.
    compages::gpu::Drawable m_points;
};

} // namespace examples
