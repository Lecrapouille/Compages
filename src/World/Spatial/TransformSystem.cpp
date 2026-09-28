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

#include "Compages/World/Spatial/TransformSystem.hpp"

#include "Compages/Core/Transformation.hpp"
#include "Compages/World/Spatial/SpatialGraph.hpp"
#include "Compages/World/Spatial/TransformStore.hpp"

namespace compages::world
{

namespace
{

const Matrix44f IDENTITY(compages::matrix::Identity);

//------------------------------------------------------------------------------
void updateNode(SpatialGraph const& p_graph,
                TransformStore& p_transforms,
                NodeId p_node,
                Matrix44f const& p_parent_world,
                bool p_parent_was_dirty)
{
    const EntityId entity = p_graph.entityOf(p_node);
    Matrix44f world_matrix = p_parent_world;
    const bool has_transform = entity.valid() && p_transforms.has(entity);
    const bool node_dirty = has_transform && p_transforms.isDirty(entity);
    const bool dirty = p_parent_was_dirty || node_dirty;

    if (has_transform)
    {
        if (dirty)
        {
            // CPU rows are shader columns, so what a mathematician writes as
            // ParentWorld * Local becomes Local * ParentWorld in this storage.
            world_matrix = p_transforms.localMatrix(entity) * p_parent_world;
            p_transforms.setWorld(entity, world_matrix);
            p_transforms.markClean(entity);
        }
        else
        {
            world_matrix = p_transforms.world(entity);
        }
    }

    NodeId child = p_graph.firstChild(p_node);
    while (child.valid())
    {
        const NodeId next = p_graph.nextSibling(child);
        updateNode(p_graph, p_transforms, child, world_matrix, dirty);
        child = next;
    }
}

} // namespace

//------------------------------------------------------------------------------
void TransformSystem::update(SpatialGraph const& p_graph,
                             TransformStore& p_transforms) const
{
    p_graph.forEachRoot([&](NodeId p_root) {
        updateNode(p_graph, p_transforms, p_root, IDENTITY, false);
    });
}

} // namespace compages::world
