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

#include <vector>

namespace compages::world
{

namespace
{

const compages::core::Matrix44f IDENTITY(compages::core::matrix::Identity);

//! \brief A node waiting to be visited, with what its parent left for it.
struct Pending
{
    NodeId node;
    compages::core::Matrix44f parent_world;
    bool parent_was_dirty;
};

} // namespace

void TransformSystem::update(SpatialGraph const& p_graph,
                             TransformStore& p_transforms) const
{
    // An explicit stack instead of recursion: a very deep chain cannot
    // overflow the call stack. Parents are still visited before children.
    std::vector<Pending> stack;
    p_graph.forEachRoot([&](NodeId p_root) {
        stack.push_back(Pending{ p_root, IDENTITY, false });
        while (!stack.empty())
        {
            const Pending pending = stack.back();
            stack.pop_back();

            const EntityId entity = p_graph.entityOf(pending.node);
            compages::core::Matrix44f world_matrix = pending.parent_world;
            const bool has_transform =
                entity.valid() && p_transforms.has(entity);
            const bool dirty = pending.parent_was_dirty ||
                               (has_transform && p_transforms.isDirty(entity));

            if (has_transform)
            {
                if (dirty)
                {
                    world_matrix =
                        pending.parent_world * p_transforms.localMatrix(entity);
                    p_transforms.setWorld(entity, world_matrix);
                    p_transforms.markClean(entity);
                }
                else
                {
                    world_matrix = p_transforms.world(entity);
                }
            }

            for (NodeId child = p_graph.firstChild(pending.node);
                 child.valid();
                 child = p_graph.nextSibling(child))
            {
                stack.push_back(Pending{ child, world_matrix, dirty });
            }
        }
    });
}

} // namespace compages::world
