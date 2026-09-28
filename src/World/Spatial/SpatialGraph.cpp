// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Spatial/SpatialGraph.hpp"

#include "Compages/Core/Transformation.hpp"
#include "Compages/World/Spatial/TransformStore.hpp"

#include <cassert>

namespace compages::world
{

void SpatialGraph::ensureEntitySparse(std::size_t p_index)
{
    if (p_index >= m_entity_to_node.size())
    {
        m_entity_to_node.resize(p_index + 1u, NO_NODE);
    }
}

bool SpatialGraph::slotAlive(std::size_t p_index) const
{
    if (p_index >= m_nodes.size())
    {
        return false;
    }
    if (m_slot_free[p_index] != 0u)
    {
        return false;
    }
    return m_generation[p_index] != 0u;
}

std::uint16_t SpatialGraph::bumpGeneration(std::size_t p_index)
{
    std::uint16_t next = static_cast<std::uint16_t>(m_generation[p_index] + 1u);
    if (next == 0u)
    {
        next = 1u;
    }
    m_generation[p_index] = next;
    return next;
}

bool SpatialGraph::alive(NodeId p_node) const
{
    if (!p_node.valid())
    {
        return false;
    }
    const std::size_t index = p_node.index();
    if (!slotAlive(index))
    {
        return false;
    }
    return m_generation[index] == p_node.generation();
}

NodeId SpatialGraph::attach(EntityId p_entity)
{
    assert(p_entity.valid() && "SpatialGraph::attach on an empty entity");
    ensureEntitySparse(p_entity.index());

    // One node per entity: return the existing one instead of making a
    // parallel record.
    if (const std::uint32_t existing = m_entity_to_node[p_entity.index()];
        existing != NO_NODE)
    {
        const std::uint16_t index = static_cast<std::uint16_t>(existing & 0xFFFFu);
        const std::uint16_t generation =
            static_cast<std::uint16_t>(existing >> 16u);
        if ((index < m_nodes.size()) && slotAlive(index) &&
            (m_generation[index] == generation) &&
            (m_nodes[index].entity == p_entity))
        {
            return NodeId(index, generation);
        }
    }

    std::uint16_t slot = 0u;
    if (!m_free.empty())
    {
        slot = m_free.back();
        m_free.pop_back();
        m_slot_free[slot] = 0u;
        m_nodes[slot] = Node{};
    }
    else
    {
        slot = static_cast<std::uint16_t>(m_nodes.size());
        m_nodes.emplace_back();
        m_generation.emplace_back(1u);
        m_slot_free.emplace_back(0u);
    }

    m_nodes[slot].entity = p_entity;
    m_entity_to_node[p_entity.index()] =
        (static_cast<std::uint32_t>(m_generation[slot]) << 16u) |
        static_cast<std::uint32_t>(slot);
    ++m_living;
    return NodeId(slot, m_generation[slot]);
}

void SpatialGraph::destroySlot(std::size_t p_index)
{
    // Destroy children first, then this slot itself. Order matters: after we
    // clear the parent's first_child, we still need to walk the children to
    // clean up their sparse entries.
    NodeId child = m_nodes[p_index].first_child;
    while (child.valid())
    {
        const NodeId next = m_nodes[child.index()].next_sibling;
        destroySlot(child.index());
        child = next;
    }

    // Detach from parent's sibling list.
    const NodeId self(static_cast<std::uint16_t>(p_index),
                      m_generation[p_index]);
    unlink(self);

    // Clear the sparse mapping only if it still points at us: a slot reused by
    // another entity in between must not be dropped by our destruction.
    const EntityId entity = m_nodes[p_index].entity;
    if (entity.valid() && (entity.index() < m_entity_to_node.size()))
    {
        const std::uint32_t stored = m_entity_to_node[entity.index()];
        const std::uint32_t self_bits =
            (static_cast<std::uint32_t>(m_generation[p_index]) << 16u) |
            static_cast<std::uint32_t>(p_index);
        if (stored == self_bits)
        {
            m_entity_to_node[entity.index()] = NO_NODE;
        }
    }

    m_nodes[p_index] = Node{};
    bumpGeneration(p_index);
    m_slot_free[p_index] = 1u;
    m_free.emplace_back(static_cast<std::uint16_t>(p_index));
    --m_living;
}

void SpatialGraph::destroy(NodeId p_node)
{
    if (!alive(p_node))
    {
        return;
    }
    destroySlot(p_node.index());
}

void SpatialGraph::detach(EntityId p_entity)
{
    if (!p_entity.valid() || (p_entity.index() >= m_entity_to_node.size()))
    {
        return;
    }
    const std::uint32_t bits = m_entity_to_node[p_entity.index()];
    if (bits == NO_NODE)
    {
        return;
    }
    const std::uint16_t index = static_cast<std::uint16_t>(bits & 0xFFFFu);
    const std::uint16_t generation = static_cast<std::uint16_t>(bits >> 16u);
    destroy(NodeId(index, generation));
}

NodeId SpatialGraph::nodeOf(EntityId p_entity) const
{
    if (!p_entity.valid() || (p_entity.index() >= m_entity_to_node.size()))
    {
        return {};
    }
    const std::uint32_t bits = m_entity_to_node[p_entity.index()];
    if (bits == NO_NODE)
    {
        return {};
    }
    const std::uint16_t index = static_cast<std::uint16_t>(bits & 0xFFFFu);
    const std::uint16_t generation = static_cast<std::uint16_t>(bits >> 16u);
    return NodeId(index, generation);
}

EntityId SpatialGraph::entityOf(NodeId p_node) const
{
    if (!alive(p_node))
    {
        return {};
    }
    return m_nodes[p_node.index()].entity;
}

void SpatialGraph::unlink(NodeId p_node)
{
    Node& node = m_nodes[p_node.index()];
    if (node.parent.valid() && alive(node.parent))
    {
        Node& parent = m_nodes[node.parent.index()];
        if (parent.first_child == p_node)
        {
            parent.first_child = node.next_sibling;
        }
    }
    if (node.prev_sibling.valid() && alive(node.prev_sibling))
    {
        m_nodes[node.prev_sibling.index()].next_sibling = node.next_sibling;
    }
    if (node.next_sibling.valid() && alive(node.next_sibling))
    {
        m_nodes[node.next_sibling.index()].prev_sibling = node.prev_sibling;
    }
    node.parent = {};
    node.prev_sibling = {};
    node.next_sibling = {};
}

void SpatialGraph::link(NodeId p_child, NodeId p_parent)
{
    Node& child = m_nodes[p_child.index()];
    Node& parent = m_nodes[p_parent.index()];
    child.parent = p_parent;
    child.next_sibling = parent.first_child;
    child.prev_sibling = {};
    if (parent.first_child.valid())
    {
        m_nodes[parent.first_child.index()].prev_sibling = p_child;
    }
    parent.first_child = p_child;
}

bool SpatialGraph::wouldCycle(NodeId p_child, NodeId p_parent) const
{
    if (!alive(p_child))
    {
        return false;
    }
    if (!p_parent.valid())
    {
        return false;
    }
    if (!alive(p_parent))
    {
        return false;
    }
    if (p_child == p_parent)
    {
        return true;
    }
    NodeId walk = p_parent;
    while (walk.valid())
    {
        if (walk == p_child)
        {
            return true;
        }
        walk = m_nodes[walk.index()].parent;
    }
    return false;
}

compages::Status SpatialGraph::setParent(NodeId p_child,
                                    NodeId p_parent,
                                    ReparentPolicy p_policy,
                                    TransformStore* p_transforms)
{
    if (!alive(p_child))
    {
        return compages::failure(
            "setParent was given a child node that no longer exists");
    }
    if (p_parent.valid() && !alive(p_parent))
    {
        return compages::failure(
            "setParent was given a parent node that no longer exists");
    }
    if (p_child == p_parent)
    {
        return compages::failure("a node cannot be its own parent");
    }
    if (wouldCycle(p_child, p_parent))
    {
        return compages::failure(
            "setParent would make a cycle: the requested parent is already a "
            "descendant of the child, which would make the world-matrix walk "
            "never finish");
    }

    // The world pose is a function of the whole ancestor chain, so we need it
    // now if we want to preserve it after the reparent. Capture it before we
    // touch the graph.
    const EntityId child_entity = m_nodes[p_child.index()].entity;
    Matrix44f preserved_world(compages::matrix::Identity);
    const bool need_world = (p_policy == ReparentPolicy::KeepWorld) &&
                            (p_transforms != nullptr) &&
                            p_transforms->has(child_entity);
    if (need_world)
    {
        preserved_world = p_transforms->world(child_entity);
    }

    unlink(p_child);
    if (p_parent.valid())
    {
        link(p_child, p_parent);
    }

    if (need_world)
    {
        // Choose the local TRS so that parentWorld * local == preservedWorld.
        // We do not have a decomposition helper handy, so we recompose by
        // treating the local as "the affine transform from parent to child",
        // pulled apart into T, R, S.
        Matrix44f parent_world(compages::matrix::Identity);
        if (p_parent.valid())
        {
            const EntityId parent_entity = m_nodes[p_parent.index()].entity;
            if (p_transforms->has(parent_entity))
            {
                parent_world = p_transforms->world(parent_entity);
            }
        }
        const Matrix44f parent_inverse = compages::matrix::inverse(parent_world);
        const Matrix44f new_local = preserved_world * parent_inverse;

        // Decompose. Rows of the CPU matrix are the columns of the shader; the
        // basis vectors sit in rows 0..2, and the translation in row 3.
        LocalTransformView local = p_transforms->localMutable(child_entity);
        local.position = Vector3f(new_local[3].x,
                                  new_local[3].y,
                                  new_local[3].z);
        const Vector3f col0(new_local[0].x, new_local[0].y, new_local[0].z);
        const Vector3f col1(new_local[1].x, new_local[1].y, new_local[1].z);
        const Vector3f col2(new_local[2].x, new_local[2].y, new_local[2].z);
        local.scale = Vector3f(compages::vector::norm(col0),
                               compages::vector::norm(col1),
                               compages::vector::norm(col2));
        Matrix44f rotation_only(compages::matrix::Identity);
        if (local.scale.x > 1.0e-6f)
        {
            const Vector3f n0 = col0 / local.scale.x;
            rotation_only[0] = Vector4f(n0.x, n0.y, n0.z, 0.0f);
        }
        if (local.scale.y > 1.0e-6f)
        {
            const Vector3f n1 = col1 / local.scale.y;
            rotation_only[1] = Vector4f(n1.x, n1.y, n1.z, 0.0f);
        }
        if (local.scale.z > 1.0e-6f)
        {
            const Vector3f n2 = col2 / local.scale.z;
            rotation_only[2] = Vector4f(n2.x, n2.y, n2.z, 0.0f);
        }
        local.rotation = Quatf::fromMatrix(rotation_only);
    }

    if (p_transforms != nullptr)
    {
        p_transforms->markDirty(child_entity);
    }
    return compages::success();
}

NodeId SpatialGraph::parent(NodeId p_node) const
{
    if (!alive(p_node))
    {
        return {};
    }
    return m_nodes[p_node.index()].parent;
}

NodeId SpatialGraph::firstChild(NodeId p_node) const
{
    if (!alive(p_node))
    {
        return {};
    }
    return m_nodes[p_node.index()].first_child;
}

NodeId SpatialGraph::nextSibling(NodeId p_node) const
{
    if (!alive(p_node))
    {
        return {};
    }
    return m_nodes[p_node.index()].next_sibling;
}

std::size_t SpatialGraph::descendantCount(NodeId p_node) const
{
    if (!alive(p_node))
    {
        return 0u;
    }
    std::size_t count = 1u;
    NodeId child = m_nodes[p_node.index()].first_child;
    while (child.valid())
    {
        count += descendantCount(child);
        child = m_nodes[child.index()].next_sibling;
    }
    return count;
}

} // namespace compages::world
