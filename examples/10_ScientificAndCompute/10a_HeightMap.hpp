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
//! \brief A surface the CPU recomputes, a hundred thousand vertices at a time.
//!
//! The height of every point is a function of its place and of time, decided
//! on the CPU each frame. The vertices are changed where they are, and the
//! whole array travels at the next draw because the whole array changed:
//! \code
//! for (Vertex& v : m_surface.vertices<Vertex>())
//! {
//!     v.position.y = heightAt(v.position.x, v.position.z, time);
//! }
//! m_surface.draw();
//! \endcode
//!
//! The triangles naming those points never move: the indices are given once
//! and stay on the GPU. 02_DynamicGeometry changed one vertex; this changes all
//! of them. Only the size of what is sent differs, not the code.
// ****************************************************************************
class HeightMap: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "10a_HeightMap";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    struct Vertex
    {
        compages::core::Vector3f position;
        compages::core::Vector3f normal;
    };

    compages::gpu::Drawable m_surface;
};

} // namespace examples
