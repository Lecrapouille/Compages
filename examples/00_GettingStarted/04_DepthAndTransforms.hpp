// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/ColoredCube.hpp"
#include "Common/Example.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A solid in three dimensions: indices, depth, culling and matrices.
//!
//! Everything the previous four examples left out arrives at once, because these
//! four things are what turn a flat picture into a solid and they are useless one
//! at a time.
//!
//! Indices, so that a corner shared by several triangles is stored once and named
//! several times. Depth testing, so that a face nearer the eye covers one further
//! away whatever order they were drawn in. Back face culling, so that half the
//! triangles of a closed shape are dropped before they cost anything. And three
//! matrices, so that a shape described once can be placed, looked at, and
//! projected.
//!
//! The depth test and the culling live in the render state of the drawable:
//! \code
//! m_cube.depthTest().cull(compages::gpu::CullMode::Back);
//! \endcode
//! They are not switched on before the draw and forgotten afterwards: two
//! drawables wanting different ones cannot leave the device in a state the
//! other did not ask for.
//!
//! The cube itself, twenty four corners and thirty six indices, is built by
//! createCube() in Common/ColoredCube.hpp, shared with the 05 examples.
// ****************************************************************************
class IndexedCube: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "04_DepthAndTransforms";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    compages::gpu::Drawable m_cube;
};

} // namespace examples
