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

namespace compages::world
{

class SpatialGraph;
class TransformStore;

// ****************************************************************************
//! \brief Turns local TRS values into up-to-date world matrices.
//!
//! Walks the graph parents-before-children and recomputes the world matrix of
//! every node whose local pose changed or whose ancestor's pose changed. Nodes
//! that stay dirty because their ancestor changed are marked dirty in the same
//! pass, so the walk itself is what makes the descendant propagation work: no
//! separate up-front sweep of the graph.
//!
//! The composition is the standard TRS one used by Three.js and Unity:
//! \code
//! WorldMatrix(root)  = LocalMatrix(root)
//! WorldMatrix(child) = WorldMatrix(parent) * LocalMatrix(child)
//! \endcode
//! Scale is included in \c localMatrix() and therefore inherited by
//! children through this multiplication.
//!
//! The system is stateless. Two Worlds can share one for as long as one is
//! running \c update() at a time.
//!
//! \code
//! compages::world::TransformSystem sys;
//! sys.update(world.spatial(), world.transforms());
//! \endcode
// ****************************************************************************
class TransformSystem
{
public:

    // ------------------------------------------------------------------------
    //! \brief Recompute every dirty world matrix.
    //!
    //! \param[in] p_graph the spatial graph to walk.
    //! \param[in,out] p_transforms the store to read locals from and write
    //! world matrices into.
    // ------------------------------------------------------------------------
    void update(SpatialGraph const& p_graph, TransformStore& p_transforms) const;
};

} // namespace compages::world
