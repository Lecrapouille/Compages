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

// ****************************************************************************
//! \brief A terrain coloured by a 3D texture: six pictures stacked, from deep
//! water to snow, and the altitude picks where to read between them.
//!
//! A 3D texture is sampled with three coordinates. The first two say where on
//! a picture, the third says between which pictures, and the hardware blends
//! the two nearest. The altitude of each vertex becomes that third
//! coordinate, so the shore fades into fields and the rocks into snow without
//! any test in the shader:
//! \code
//! COMPAGES_TRY(m_layers.loadVolume({ "deep_water.png", ..., "snow.png" }));
//! m_terrain["layers"] = m_layers;         // a sampler3D in the shader
//! \endcode
// ****************************************************************************
class Terrain3D: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "10b_Terrain3D";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    struct Vertex
    {
        Vector3f position;
        //! \brief Where to read the stack of pictures: x and y on a picture,
        //! z between them.
        Vector3f layer_coord;
    };

    void makeTerrain(std::uint32_t p_side);

    //! \brief Declared before the drawable sampling it, so destroyed after.
    compages::gpu::Texture m_layers;
    compages::gpu::Drawable m_terrain;
};

} // namespace examples
