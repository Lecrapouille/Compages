// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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
