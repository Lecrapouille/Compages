// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/World.hpp"
#include "Compages/World/Entity.hpp"

#include "Compages/World/Controllers/ViewFrame.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace compages::world
{

const std::string World::s_empty_name{};

Entity World::entity(std::string p_name)
{
    return Entity(*this, create(std::move(p_name)));
}

Entity World::entity(EntityId p_id)
{
    return Entity(*this, p_id);
}

EntityId World::create(std::string p_name)
{
    // EnTT indexes its entities with 20 bits. Fail loudly past that, in
    // release builds too (an assert is compiled out by NDEBUG).
    if (m_living >= EntityId::MAX_COUNT)
    {
        throw std::length_error("compages::world::World: too many living "
                                "entities (limit is " +
                                std::to_string(EntityId::MAX_COUNT) + ")");
    }
    EntityId entity(m_registry.create());
    ++m_living;
    (void)m_graph.attach(entity);
    m_transforms.allocate(entity);
    if (!p_name.empty())
    {
        m_registry.emplace<Name>(entity.native(), Name{ std::move(p_name) });
    }
    return entity;
}

void World::destroy(EntityId p_entity)
{
    if (!alive(p_entity))
    {
        return;
    }

    // List the subtree first, then destroy it children first. No recursion,
    // so a deep chain cannot overflow the stack.
    std::vector<EntityId> subtree;
    const NodeId node = m_graph.nodeOf(p_entity);
    if (node.valid())
    {
        m_graph.forEachDfs(node, [&](NodeId p_each)
                           { subtree.push_back(m_graph.entityOf(p_each)); });
    }
    else
    {
        subtree.push_back(p_entity);
    }
    // onDestroy() of the routines while the whole subtree is still alive.
    for (auto it = subtree.rbegin(); it != subtree.rend(); ++it)
    {
        m_routines.release(*it);
    }
    if (node.valid())
    {
        m_graph.destroy(node);
    }
    for (auto it = subtree.rbegin(); it != subtree.rend(); ++it)
    {
        // An onDestroy() may already have destroyed one of them.
        if (!m_registry.valid(it->native()))
        {
            continue;
        }
        m_transforms.release(*it);
        m_registry.destroy(it->native());
        --m_living;
    }
}

bool World::alive(EntityId p_entity) const
{
    return p_entity.valid() && m_registry.valid(p_entity.native());
}

std::string const& World::name(EntityId p_entity) const
{
    if (!alive(p_entity))
    {
        return s_empty_name;
    }
    if (auto const* value = m_registry.try_get<Name>(p_entity.native()))
    {
        return value->value;
    }
    return s_empty_name;
}

void World::setName(EntityId p_entity, std::string p_name)
{
    if (!alive(p_entity))
    {
        return;
    }
    if (p_name.empty())
    {
        m_registry.remove<Name>(p_entity.native());
    }
    else
    {
        m_registry.emplace_or_replace<Name>(p_entity.native(),
                                            Name{ std::move(p_name) });
    }
}

void World::setEnabled(EntityId p_entity, bool p_enabled)
{
    if (!alive(p_entity))
    {
        return;
    }
    if (p_enabled)
    {
        m_registry.remove<Disabled>(p_entity.native());
    }
    else
    {
        m_registry.emplace_or_replace<Disabled>(p_entity.native());
    }
}

bool World::enabled(EntityId p_entity) const
{
    if (!alive(p_entity))
    {
        return false;
    }
    return !m_registry.all_of<Disabled>(p_entity.native());
}

bool World::enabledInHierarchy(EntityId p_entity) const
{
    EntityId walk = p_entity;
    while (walk.valid())
    {
        if (!enabled(walk))
        {
            return false;
        }
        walk = parent(walk);
    }
    return true;
}

Status
World::setParent(EntityId p_child, EntityId p_parent, ReparentPolicy p_policy)
{
    if (!alive(p_child))
    {
        return failure(
            "setParent was given a child entity that no longer exists");
    }
    if (p_parent.valid() && !alive(p_parent))
    {
        return failure(
            "setParent was given a parent entity that no longer exists");
    }
    if (p_child == p_parent)
    {
        return failure("an entity cannot be its own parent");
    }

    const NodeId child = m_graph.nodeOf(p_child);
    const NodeId parent =
        p_parent.valid() ? m_graph.nodeOf(p_parent) : NodeId{};
    if (!child.valid())
    {
        return failure("the child entity is not part of the spatial graph");
    }
    if (p_parent.valid() && !parent.valid())
    {
        return failure("the parent entity is not part of the spatial graph");
    }

    return m_graph.setParent(child, parent, p_policy, &m_transforms);
}

EntityId World::parent(EntityId p_entity) const
{
    const NodeId node = m_graph.nodeOf(p_entity);
    if (!node.valid())
    {
        return {};
    }
    return m_graph.entityOf(m_graph.parent(node));
}

EntityId World::firstChild(EntityId p_entity) const
{
    const NodeId node = m_graph.nodeOf(p_entity);
    if (!node.valid())
    {
        return {};
    }
    return m_graph.entityOf(m_graph.firstChild(node));
}

EntityId World::nextSibling(EntityId p_entity) const
{
    const NodeId node = m_graph.nodeOf(p_entity);
    if (!node.valid())
    {
        return {};
    }
    return m_graph.entityOf(m_graph.nextSibling(node));
}

EntityId World::find(EntityId p_root, std::string_view p_path) const
{
    if (!alive(p_root))
    {
        return {};
    }

    EntityId current = p_root;
    std::size_t begin = 0u;
    while (begin < p_path.size())
    {
        if (p_path[begin] == '/')
        {
            ++begin;
            continue;
        }
        const std::size_t slash = p_path.find('/', begin);
        const std::string_view part = p_path.substr(
            begin,
            (slash == std::string_view::npos) ? std::string_view::npos
                                              : (slash - begin));

        EntityId child = firstChild(current);
        EntityId found;
        while (child.valid())
        {
            if (std::string_view(name(child)) == part)
            {
                found = child;
                break;
            }
            child = nextSibling(child);
        }
        if (!found.valid())
        {
            return {};
        }
        current = found;
        if (slash == std::string_view::npos)
        {
            break;
        }
        begin = slash + 1u;
    }
    return current;
}

std::size_t World::descendantCount(EntityId p_entity) const
{
    return m_graph.descendantCount(m_graph.nodeOf(p_entity));
}

LocalTransformView World::transform(EntityId p_entity)
{
    assert(alive(p_entity) && "World::transform on a stale entity");
    // Marking the entity dirty is enough: the update pass propagates dirt to
    // descendants as it walks. That is why we do not walk the subtree here.
    m_transforms.markDirty(p_entity);
    return m_transforms.localMutable(p_entity);
}

LocalTransform World::transform(EntityId p_entity) const
{
    assert(alive(p_entity) && "World::transform on a stale entity");
    return m_transforms.local(p_entity);
}

compages::core::Matrix44f const& World::worldMatrix(EntityId p_entity) const
{
    assert(alive(p_entity) && "World::worldMatrix on a stale entity");
    return m_transforms.world(p_entity);
}

void World::update()
{
    m_kinematic_system.update(m_registry, m_transforms);
    m_transform_system.update(m_graph, m_transforms);
}

World::~World()
{
    m_routines.clear();
}

void World::step(compages::core::Frame const& p_frame)
{
    m_frame = p_frame;
    // A headless step has no input: do not keep the keys and the mouse of a
    // previous update(ViewFrame), or routines would see them held forever.
    m_input = Input{};
    m_routines.update(m_frame.elapsed);
}

void World::step(ViewFrame const& p_view)
{
    m_frame = p_view;
    m_input = p_view.input;
    m_routines.update(m_frame.elapsed);
}

void World::endStep()
{
    m_routines.lateUpdate(m_frame.elapsed);
    update();
}

void World::update(compages::core::Frame const& p_frame)
{
    step(p_frame);
    update();
    endStep();
}

void World::update(ViewFrame const& p_view)
{
    step(p_view);
    update();
    endStep();
}

Entity World::lookup(std::string_view p_path)
{
    while (!p_path.empty() && (p_path.front() == '/'))
    {
        p_path.remove_prefix(1u);
    }
    const std::size_t slash = p_path.find('/');
    const std::string_view first = p_path.substr(0u, slash);

    EntityId root;
    for (auto [native, name] : m_registry.view<Name>().each())
    {
        const EntityId candidate(native);
        if ((name.value == first) && !parent(candidate).valid())
        {
            root = candidate;
            break;
        }
    }
    if (!root.valid() || (slash == std::string_view::npos))
    {
        return Entity(*this, root);
    }
    return Entity(*this, find(root, p_path.substr(slash + 1u)));
}

} // namespace compages::world
