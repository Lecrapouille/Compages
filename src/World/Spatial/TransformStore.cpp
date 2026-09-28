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

#include "Compages/World/Spatial/TransformStore.hpp"

namespace compages::world
{

namespace
{
const Matrix44f IDENTITY(compages::matrix::Identity);
const Vector3f ZERO_POSITION(0.0f, 0.0f, 0.0f);
const Vector3f UNIT_SCALE(1.0f, 1.0f, 1.0f);
} // namespace

//------------------------------------------------------------------------------
void TransformStore::ensureCapacity(std::size_t p_index)
{
    if (p_index >= m_flags.size())
    {
        m_position.resize(p_index + 1u);
        m_rotation.resize(p_index + 1u);
        m_scale.resize(p_index + 1u, UNIT_SCALE);
        m_world.resize(p_index + 1u, IDENTITY);
        m_flags.resize(p_index + 1u, 0u);
        m_generation.resize(p_index + 1u, 0u);
    }
}

//------------------------------------------------------------------------------
bool TransformStore::slotMatches(EntityId p_entity) const
{
    if (!p_entity.valid())
    {
        return false;
    }
    const std::size_t index = p_entity.index();
    if (index >= m_flags.size())
    {
        return false;
    }
    if (m_generation[index] != p_entity.generation())
    {
        return false;
    }
    return (m_flags[index] & FLAG_PRESENT) != 0u;
}

//------------------------------------------------------------------------------
void TransformStore::allocate(EntityId p_entity)
{
    assert(p_entity.valid() && "TransformStore::allocate on an empty entity");
    const std::size_t index = p_entity.index();
    ensureCapacity(index);

    if ((m_generation[index] != p_entity.generation()) ||
        ((m_flags[index] & FLAG_PRESENT) == 0u))
    {
        m_position[index] = ZERO_POSITION;
        m_rotation[index] = Quatf{};
        m_scale[index] = UNIT_SCALE;
        m_world[index] = IDENTITY;
    }

    m_flags[index] = static_cast<std::uint8_t>(FLAG_PRESENT | FLAG_DIRTY);
    m_generation[index] = p_entity.generation();
}

//------------------------------------------------------------------------------
void TransformStore::release(EntityId p_entity)
{
    if (!p_entity.valid())
    {
        return;
    }
    const std::size_t index = p_entity.index();
    if (index >= m_flags.size())
    {
        return;
    }
    if (m_generation[index] != p_entity.generation())
    {
        return;
    }
    m_flags[index] = 0u;
    m_generation[index] = 0u;
    m_position[index] = ZERO_POSITION;
    m_rotation[index] = Quatf{};
    m_scale[index] = UNIT_SCALE;
    m_world[index] = IDENTITY;
}

//------------------------------------------------------------------------------
bool TransformStore::has(EntityId p_entity) const
{
    return slotMatches(p_entity);
}

//------------------------------------------------------------------------------
LocalTransform TransformStore::local(EntityId p_entity) const
{
    assert(has(p_entity) && "TransformStore::local on an entity without one");
    const std::size_t index = p_entity.index();
    return LocalTransform{ m_position[index], m_rotation[index], m_scale[index] };
}

//------------------------------------------------------------------------------
LocalTransformView TransformStore::localMutable(EntityId p_entity)
{
    assert(has(p_entity) &&
           "TransformStore::localMutable on an entity without one");
    const std::size_t index = p_entity.index();
    return LocalTransformView(
        m_position[index], m_rotation[index], m_scale[index]);
}

//------------------------------------------------------------------------------
Vector3f const& TransformStore::position(EntityId p_entity) const
{
    assert(has(p_entity) && "TransformStore::position on an entity without one");
    return m_position[p_entity.index()];
}

//------------------------------------------------------------------------------
Quatf const& TransformStore::rotation(EntityId p_entity) const
{
    assert(has(p_entity) && "TransformStore::rotation on an entity without one");
    return m_rotation[p_entity.index()];
}

//------------------------------------------------------------------------------
Vector3f const& TransformStore::scale(EntityId p_entity) const
{
    assert(has(p_entity) && "TransformStore::scale on an entity without one");
    return m_scale[p_entity.index()];
}

//------------------------------------------------------------------------------
Matrix44f TransformStore::localMatrix(EntityId p_entity) const
{
    assert(has(p_entity) &&
           "TransformStore::localMatrix on an entity without one");
    const std::size_t index = p_entity.index();
    return composeLocalMatrix(
        m_position[index], m_rotation[index], m_scale[index]);
}

//------------------------------------------------------------------------------
Matrix44f const& TransformStore::world(EntityId p_entity) const
{
    assert(has(p_entity) && "TransformStore::world on an entity without one");
    return m_world[p_entity.index()];
}

//------------------------------------------------------------------------------
void TransformStore::setWorld(EntityId p_entity, Matrix44f const& p_matrix)
{
    assert(has(p_entity) && "TransformStore::setWorld on an entity without one");
    m_world[p_entity.index()] = p_matrix;
}

//------------------------------------------------------------------------------
void TransformStore::markDirty(EntityId p_entity)
{
    if (!has(p_entity))
    {
        return;
    }
    m_flags[p_entity.index()] |= FLAG_DIRTY;
}

//------------------------------------------------------------------------------
void TransformStore::markClean(EntityId p_entity)
{
    if (!has(p_entity))
    {
        return;
    }
    m_flags[p_entity.index()] &= static_cast<std::uint8_t>(~FLAG_DIRTY);
}

//------------------------------------------------------------------------------
bool TransformStore::isDirty(EntityId p_entity) const
{
    if (!has(p_entity))
    {
        return false;
    }
    return (m_flags[p_entity.index()] & FLAG_DIRTY) != 0u;
}

} // namespace compages::world
