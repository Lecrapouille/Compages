// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Spatial/TransformSystem.hpp"

#include "Compages/Core/Transformation.hpp"
#include "Compages/World/Spatial/SpatialGraph.hpp"
#include "Compages/World/Spatial/TransformStore.hpp"

namespace compages::world
{

namespace
{

const compages::core::Matrix44f IDENTITY(compages::core::matrix::Identity);

void updateNode(SpatialGraph const& p_graph,
                TransformStore& p_transforms,
                NodeId p_node,
                compages::core::Matrix44f const& p_parent_world,
                bool p_parent_was_dirty)
{
    const EntityId entity = p_graph.entityOf(p_node);
    compages::core::Matrix44f world_matrix = p_parent_world;
    const bool has_transform = entity.valid() && p_transforms.has(entity);
    const bool node_dirty = has_transform && p_transforms.isDirty(entity);
    const bool dirty = p_parent_was_dirty || node_dirty;

    if (has_transform)
    {
        if (dirty)
        {
            world_matrix = p_parent_world * p_transforms.localMatrix(entity);
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

void TransformSystem::update(SpatialGraph const& p_graph,
                             TransformStore& p_transforms) const
{
    p_graph.forEachRoot([&](NodeId p_root) {
        updateNode(p_graph, p_transforms, p_root, IDENTITY, false);
    });
}

} // namespace compages::world
