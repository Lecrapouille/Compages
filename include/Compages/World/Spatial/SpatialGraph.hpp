// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/Core/Result.hpp"
#include "Compages/World/EntityId.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace compages::world
{

class TransformStore;

// ****************************************************************************
//! \brief Names one node inside a SpatialGraph. Distinct from an EntityId.
//!
//! An EntityId is an identity of the World. A NodeId is the position of that
//! identity inside the spatial hierarchy. They are separate because an EntityId
//! may exist without being spatial: an event, a UI object, a piece of data. The
//! same entity may not appear twice in a graph; but a graph is not the only
//! kind of relation an entity can have.
//!
//! \code
//! compages::world::NodeId node = graph.attach(entity);
//! if (node.valid()) { graph.parent(node); }
//! \endcode
// ****************************************************************************
class NodeId
{
public:

    static constexpr std::uint32_t MAX_COUNT = 0xFFFFu;

    constexpr NodeId() = default;

    constexpr NodeId(std::uint16_t p_index, std::uint16_t p_generation)
        : m_bits((static_cast<std::uint32_t>(p_generation) << 16u) |
                 static_cast<std::uint32_t>(p_index))
    {
    }

    [[nodiscard]] constexpr bool valid() const
    {
        return m_bits != 0u;
    }
    [[nodiscard]] constexpr explicit operator bool() const
    {
        return valid();
    }
    [[nodiscard]] constexpr std::uint16_t index() const
    {
        return static_cast<std::uint16_t>(m_bits & 0xFFFFu);
    }
    [[nodiscard]] constexpr std::uint16_t generation() const
    {
        return static_cast<std::uint16_t>(m_bits >> 16u);
    }
    [[nodiscard]] constexpr std::uint32_t bits() const
    {
        return m_bits;
    }

    [[nodiscard]] constexpr bool operator==(NodeId const& p_other) const
    {
        return m_bits == p_other.m_bits;
    }
    [[nodiscard]] constexpr bool operator!=(NodeId const& p_other) const
    {
        return m_bits != p_other.m_bits;
    }
    [[nodiscard]] constexpr auto operator<=>(NodeId const& p_other) const
    {
        return m_bits <=> p_other.m_bits;
    }

private:

    std::uint32_t m_bits = 0u;
};

// ****************************************************************************
//! \brief What to keep when a node is reparented.
//!
//! The two families of behaviour a scene graph needs, made explicit at the
//! call site instead of guessed from a global flag.
//!
//! \code
//! world.setParent(child, newParent,
//! compages::world::ReparentPolicy::KeepWorld);
//! \endcode
// ****************************************************************************
enum class ReparentPolicy
{
    //! \brief Keep the local TRS the way it is. The world pose then changes to
    //! be that of the new parent times the child's local. This is what a robot
    //! demo attaching a limb to a body wants: the limb keeps its shoulder
    //! offset and swings with the body.
    KeepLocal,
    //! \brief Recompute the local TRS so the world pose stays the way it was.
    //! This is what an editor dragging an object between parents wants: the
    //! object does not jump.
    KeepWorld,
};

// ****************************************************************************
//! \brief The spatial hierarchy of a World.
//!
//! Nodes are opaque records: parent, first child, next and previous sibling,
//! plus the EntityId they represent. Nothing else. The transform of an entity
//! lives in the \c TransformStore; the graph only says who is attached to whom.
//!
//! An EntityId has at most one node in a given graph. Whether it has one at all
//! is a decision of the World, not of the graph.
//!
//! Invariants the graph enforces:
//! - no cycle: \c setParent refuses to make a node an ancestor of itself;
//! - a live parent or no parent: destroying a parent destroys its subtree;
//! - stale handles are caught: destroying a node bumps its generation, so a
//!   leftover NodeId no longer names anything.
//!
//! \code
//! compages::world::SpatialGraph& g = world.spatial();
//! compages::world::NodeId root = g.attach(world.create("Root"));
//! g.setParent(g.attach(world.create("Child")), root);
//! \endcode
// ****************************************************************************
class SpatialGraph: private compages::core::NotCopiable
{
public:

    SpatialGraph() = default;
    SpatialGraph(SpatialGraph&&) = default;
    SpatialGraph& operator=(SpatialGraph&&) = default;

    // ------------------------------------------------------------------------
    //! \brief Create a node for an EntityId. Returns the existing one if any.
    // ------------------------------------------------------------------------
    [[nodiscard]] NodeId attach(EntityId p_entity);

    // ------------------------------------------------------------------------
    //! \brief Destroy a node and every one below it. A stale NodeId is ignored.
    //!
    //! The Entities themselves are not destroyed: whether a spatial removal
    //! also destroys the identity is a World-level decision. The graph only
    //! detaches and clears its own records.
    // ------------------------------------------------------------------------
    void destroy(NodeId p_node);

    // ------------------------------------------------------------------------
    //! \brief Drop the node associated with an EntityId, if any. The EntityId
    //! itself is left alone.
    // ------------------------------------------------------------------------
    void detach(EntityId p_entity);

    // ------------------------------------------------------------------------
    //! \brief Is this NodeId still alive?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool alive(NodeId p_node) const;

    // ------------------------------------------------------------------------
    //! \brief The NodeId of an EntityId, or an empty NodeId when the entity is
    //! not spatial in this graph.
    // ------------------------------------------------------------------------
    [[nodiscard]] NodeId nodeOf(EntityId p_entity) const;

    // ------------------------------------------------------------------------
    //! \brief The EntityId a node represents.
    // ------------------------------------------------------------------------
    [[nodiscard]] EntityId entityOf(NodeId p_node) const;

    // ------------------------------------------------------------------------
    //! \brief Change (or clear) the parent of a node.
    //!
    //! Refuses to make a cycle: a node cannot become its own ancestor, which
    //! is how a world-matrix walk would never finish.
    //!
    //! \param[in] p_child the node being reparented. Must be alive.
    //! \param[in] p_parent the new parent, or an empty NodeId to unparent.
    //! \param[in] p_policy whether to keep the local TRS or the world pose.
    //! \param[in,out] p_transforms the transform store. Required when the
    //! policy is \c KeepWorld, so the child's local TRS can be recomputed. May
    //! be null when the policy is \c KeepLocal.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status
    setParent(NodeId p_child,
              NodeId p_parent,
              ReparentPolicy p_policy = ReparentPolicy::KeepLocal,
              TransformStore* p_transforms = nullptr);

    // ------------------------------------------------------------------------
    //! \brief The parent of a node, or an empty NodeId at the top.
    // ------------------------------------------------------------------------
    [[nodiscard]] NodeId parent(NodeId p_node) const;

    // ------------------------------------------------------------------------
    //! \brief The first child, in insertion order, or an empty NodeId.
    // ------------------------------------------------------------------------
    [[nodiscard]] NodeId firstChild(NodeId p_node) const;

    // ------------------------------------------------------------------------
    //! \brief The next sibling, or an empty NodeId at the end of the list.
    // ------------------------------------------------------------------------
    [[nodiscard]] NodeId nextSibling(NodeId p_node) const;

    // ------------------------------------------------------------------------
    //! \brief How many nodes hang below this one, itself included.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t descendantCount(NodeId p_node) const;

    // ------------------------------------------------------------------------
    //! \brief Walk a subtree parents-before-children.
    //!
    //! \c TransformSystem needs this: a child's world matrix depends on its
    //! parent's, so parents must be visited first.
    //!
    //! \tparam F a callable receiving a NodeId by value.
    // ------------------------------------------------------------------------
    template <typename F>
    void forEachDfs(NodeId p_root, F&& p_callback) const
    {
        if (!alive(p_root))
        {
            return;
        }
        dfs(p_root, std::forward<F>(p_callback));
    }

    // ------------------------------------------------------------------------
    //! \brief Walk every root of the graph.
    // ------------------------------------------------------------------------
    template <typename F>
    void forEachRoot(F&& p_callback) const
    {
        for (std::size_t i = 0u; i < m_nodes.size(); ++i)
        {
            if (!slotAlive(i))
            {
                continue;
            }
            if (m_nodes[i].parent.valid())
            {
                continue;
            }
            p_callback(NodeId(static_cast<std::uint16_t>(i), m_generation[i]));
        }
    }

    // ------------------------------------------------------------------------
    //! \brief How many nodes exist right now.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t size() const
    {
        return m_living;
    }

    // ------------------------------------------------------------------------
    //! \brief Number of node slots ever used, alive or free.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t capacity() const
    {
        return m_nodes.size();
    }

    // ------------------------------------------------------------------------
    //! \brief Would setting \c p_parent as the parent of \c p_child make a
    //! cycle? False when either handle is stale or a cycle would exist.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool wouldCycle(NodeId p_child, NodeId p_parent) const;

private:

    struct Node
    {
        EntityId entity;
        NodeId parent;
        NodeId first_child;
        NodeId next_sibling;
        NodeId prev_sibling;
    };

    void ensureEntitySparse(std::size_t p_index);
    [[nodiscard]] bool slotAlive(std::size_t p_index) const;
    std::uint16_t bumpGeneration(std::size_t p_index);
    void unlink(NodeId p_node);
    void link(NodeId p_child, NodeId p_parent);
    void destroySlot(std::size_t p_index);

    template <typename F>
    void dfs(NodeId p_node, F& p_callback) const
    {
        p_callback(p_node);
        NodeId child = m_nodes[p_node.index()].first_child;
        while (child.valid())
        {
            const NodeId next = m_nodes[child.index()].next_sibling;
            dfs(child, p_callback);
            child = next;
        }
    }

    static constexpr std::uint32_t NO_NODE = 0u;

    //! \brief All the nodes, indexed by \c NodeId::index(). Slot 0 is a live
    //! slot: it is generation 0 that means "empty".
    std::vector<Node> m_nodes;
    //! \brief Current generation of each slot. Zero when the slot is free.
    std::vector<std::uint16_t> m_generation;
    //! \brief Whether a slot is currently on the free list.
    std::vector<std::uint8_t> m_slot_free;
    //! \brief Free node slots.
    std::vector<std::uint16_t> m_free;
    //! \brief Sparse array \c EntityId::index() -> \c NodeId::bits(). Zero
    //! means "no node for this entity".
    std::vector<std::uint32_t> m_entity_to_node;
    std::size_t m_living = 0u;
};

} // namespace compages::world
