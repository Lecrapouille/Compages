// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include <array>

namespace examples
{

// ****************************************************************************
//! \brief Every way of writing vertices, side by side: nine small triangles,
//! one per syntax. What they draw does not matter; the code does.
//!
//! Giving the vertices:
//! \code
//! d["position"] = { {-1, -1}, {1, -1}, {0, 1} };     // by name
//! d.setVertices<Vertex>({ {...}, {...}, {...} });    // between braces
//! d.setVertices(corners);                            // any container
//! d.emplace_back(Vertex{ position, color });         // one more
//! d.borrowVertices(buffer);                          // ours, no copy
//! \endcode
//!
//! Changing them, then draw() sends what changed:
//! \code
//! d["color"].set(0u, red);                        // one value, by name
//! d["position"] = positions;                      // a whole attribute
//! d.editVertex<Vertex>(2u).position = p;          // one vertex
//! for (Vertex& v : d.editVertices<Vertex>(0u, 2u)) ...   // a run
//! for (Vertex& v : d.editVertices<Vertex>()) ...  // all of them
//! buffer[1].position = p; buffer.upload();        // a borrowed Buffer
//! \endcode
//!
//! The panel lists which triangle uses which syntax.
// ****************************************************************************
class VertexSyntax: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "01f_VertexSyntax";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    struct Vertex
    {
        compages::Vector2f position;
        compages::Vector3f color;
    };

    //! \brief The Buffers we own, read by the last two triangles. Declared
    //! first, so that they are destroyed after the drawables.
    compages::gpu::VertexBuffer<Vertex> m_static;
    compages::gpu::VertexBuffer<Vertex> m_dynamic;

    compages::gpu::Drawable m_by_name;
    compages::gpu::Drawable m_whole_attribute;
    compages::gpu::Drawable m_braces;
    compages::gpu::Drawable m_edit_one;
    compages::gpu::Drawable m_edit_run;
    compages::gpu::Drawable m_edit_all;
    compages::gpu::Drawable m_grown;
    compages::gpu::Drawable m_borrow_static;
    compages::gpu::Drawable m_borrow_dynamic;

    //! \brief The nine, in the order of the grid.
    std::array<compages::gpu::Drawable*, 9u> m_all{
        &m_by_name,  &m_whole_attribute, &m_braces,
        &m_edit_one, &m_edit_run,        &m_edit_all,
        &m_grown,    &m_borrow_static,   &m_borrow_dynamic
    };
};

} // namespace examples
