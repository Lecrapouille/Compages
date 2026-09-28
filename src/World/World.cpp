// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Entity.hpp"
#include "Compages/World/World.hpp"

#include "Compages/World/Controllers/ViewFrame.hpp"

#include <algorithm>
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
    assert(m_living < EntityId::MAX_COUNT && "World entity limit reached");
    EntityId entity(m_registry.create());
    ++m_living;
    (void)m_graph.attach(entity);
    m_transforms.allocate(entity);
    if (!p_name.empty())
    {
        m_registry.emplace<Name>(
            entity.native(), Name{ std::move(p_name) });
    }
    return entity;
}

void World::destroy(EntityId p_entity)
{
    if (!alive(p_entity))
    {
        return;
    }
    destroyRecursive(p_entity);
}

void World::destroyRecursive(EntityId p_entity)
{
    // Walk children first so their components and transform slots are cleaned
    // up before their parent is.
    EntityId child = firstChild(p_entity);
    while (child.valid())
    {
        const EntityId next = nextSibling(child);
        destroyRecursive(child);
        child = next;
    }

    m_graph.detach(p_entity);
    m_transforms.release(p_entity);
    m_registry.destroy(p_entity.native());
    --m_living;
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
    if (auto const* value =
            m_registry.try_get<Name>(p_entity.native()))
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
        m_registry.emplace_or_replace<Name>(
            p_entity.native(), Name{ std::move(p_name) });
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

compages::Status World::setParent(EntityId p_child,
                             EntityId p_parent,
                             ReparentPolicy p_policy)
{
    if (!alive(p_child))
    {
        return compages::failure(
            "setParent was given a child entity that no longer exists");
    }
    if (p_parent.valid() && !alive(p_parent))
    {
        return compages::failure(
            "setParent was given a parent entity that no longer exists");
    }
    if (p_child == p_parent)
    {
        return compages::failure("an entity cannot be its own parent");
    }

    const NodeId child = m_graph.nodeOf(p_child);
    const NodeId parent = p_parent.valid() ? m_graph.nodeOf(p_parent) : NodeId{};
    if (!child.valid())
    {
        return compages::failure(
            "the child entity is not part of the spatial graph");
    }
    if (p_parent.valid() && !parent.valid())
    {
        return compages::failure(
            "the parent entity is not part of the spatial graph");
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
        const std::string_view part =
            p_path.substr(begin,
                          (slash == std::string_view::npos)
                              ? std::string_view::npos
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

Matrix44f const& World::worldMatrix(EntityId p_entity) const
{
    assert(alive(p_entity) && "World::worldMatrix on a stale entity");
    return m_transforms.world(p_entity);
}

void World::update()
{
    m_kinematic_system.update(m_registry, m_transforms);
    m_transform_system.update(m_graph, m_transforms);
}

void World::update(Frame const& p_frame)
{
    m_frame = p_frame;
    runBehaviors();
    update();
}

void World::update(ViewFrame const& p_view)
{
    m_frame = p_view;
    m_input = p_view.input;
    runBehaviors();
    update();
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

Behavior& World::addBehavior(EntityId p_entity, std::unique_ptr<Behavior> p_behavior)
{
    assert(alive(p_entity) && "a behavior added to a dead entity");
    p_behavior->m_world = this;
    p_behavior->m_entity = p_entity;
    Behaviors& behaviors = m_registry.get_or_emplace<Behaviors>(p_entity.native());
    behaviors.list.emplace_back(std::move(p_behavior));
    return *behaviors.list.back();
}

std::size_t World::behaviorCount(EntityId p_entity) const
{
    Behaviors const* behaviors = tryGet<Behaviors>(p_entity);
    return (behaviors == nullptr) ? 0u : behaviors->list.size();
}

void World::runBehaviors()
{
    // A behavior may create or destroy entities, or add behaviors: the list
    // of who runs this frame is taken first.
    struct Running
    {
        EntityId entity;
        Behavior* behavior;
    };
    std::vector<Running> running;
    for (auto [native, behaviors] : m_registry.view<Behaviors>().each())
    {
        if (!enabledInHierarchy(EntityId(native)))
        {
            continue;
        }
        for (std::unique_ptr<Behavior> const& behavior : behaviors.list)
        {
            running.emplace_back(Running{ EntityId(native), behavior.get() });
        }
    }
    for (Running const& run : running)
    {
        // Gone if an earlier behavior of this same frame destroyed it: the
        // pointer is only followed once found again in its entity.
        Behaviors const* owner = tryGet<Behaviors>(run.entity);
        if ((owner == nullptr) ||
            std::none_of(owner->list.begin(), owner->list.end(),
                         [&run](std::unique_ptr<Behavior> const& p_kept) {
                             return p_kept.get() == run.behavior;
                         }))
        {
            continue;
        }
        Behavior* behavior = run.behavior;
        if (!behavior->m_started)
        {
            behavior->m_started = true;
            behavior->start();
        }
        behavior->update(m_frame.elapsed);
    }
}

} // namespace compages::world
