//==============================================================================
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
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//==============================================================================

#include "main.hpp"

#include "Compages/GPU/Core/Handle.hpp"

#include <unordered_map>
#include <unordered_set>

namespace
{

struct ThingTag
{
};
struct OtherTag
{
};

} // namespace

//------------------------------------------------------------------------------
// A handle must cost no more than the integer it wraps, otherwise storing one per
// scene node stops being free.
//------------------------------------------------------------------------------
TEST(Handle, IsJustAnInteger)
{
    ASSERT_EQ(sizeof(compages::gpu::Handle<ThingTag>), sizeof(std::uint32_t));
    ASSERT_TRUE(std::is_trivially_copyable_v<compages::gpu::Handle<ThingTag>>);
}

//------------------------------------------------------------------------------
// A freshly declared handle names nothing, so forgetting to assign one is caught
// rather than pointing at slot zero.
//------------------------------------------------------------------------------
TEST(Handle, IsEmptyByDefault)
{
    compages::gpu::Handle<ThingTag> handle;
    ASSERT_FALSE(handle.valid());
    ASSERT_FALSE(bool(handle));
    ASSERT_EQ(handle.bits(), 0u);
}

//------------------------------------------------------------------------------
TEST(Handle, RemembersItsSlotAndReuseCount)
{
    const compages::gpu::Handle<ThingTag> handle(42u, 7u);

    ASSERT_TRUE(handle.valid());
    ASSERT_EQ(handle.index(), 42u);
    ASSERT_EQ(handle.generation(), 7u);
}

//------------------------------------------------------------------------------
// Slot zero is a perfectly ordinary slot: it is the reuse count, never the slot
// number, that tells an empty handle from a live one.
//------------------------------------------------------------------------------
TEST(Handle, SlotZeroIsNotEmpty)
{
    const compages::gpu::Handle<ThingTag> handle(0u, 1u);

    ASSERT_TRUE(handle.valid());
    ASSERT_EQ(handle.index(), 0u);
    ASSERT_EQ(handle.generation(), 1u);
}

//------------------------------------------------------------------------------
TEST(Handle, HoldsTheWholeRangeOfSlotsAndCounts)
{
    const compages::gpu::Handle<ThingTag> handle(0xFFFFu, 0xFFFFu);

    ASSERT_EQ(handle.index(), 0xFFFFu);
    ASSERT_EQ(handle.generation(), 0xFFFFu);
    ASSERT_EQ(compages::gpu::Handle<ThingTag>::MAX_COUNT, 0xFFFFu);
}

//------------------------------------------------------------------------------
// Same slot, different reuse count, means a different resource. This is the whole
// mechanism that turns using a released resource into a detectable mistake.
//------------------------------------------------------------------------------
TEST(Handle, DiffersFromAnOlderUseOfTheSameSlot)
{
    const compages::gpu::Handle<ThingTag> before(5u, 1u);
    const compages::gpu::Handle<ThingTag> after(5u, 2u);

    ASSERT_FALSE(before == after);
    ASSERT_TRUE(before != after);
    ASSERT_TRUE(before < after);
}

//------------------------------------------------------------------------------
TEST(Handle, ComparesEqualToItsOwnCopy)
{
    const compages::gpu::Handle<ThingTag> handle(11u, 3u);
    const compages::gpu::Handle<ThingTag> copy = handle;

    ASSERT_TRUE(handle == copy);
    ASSERT_EQ(handle.bits(), copy.bits());
}

//------------------------------------------------------------------------------
TEST(Handle, WorksAsAKeyOfAHashMap)
{
    std::unordered_map<compages::gpu::Handle<ThingTag>, int> map;
    map[compages::gpu::Handle<ThingTag>(1u, 1u)] = 10;
    map[compages::gpu::Handle<ThingTag>(1u, 2u)] = 20;
    map[compages::gpu::Handle<ThingTag>(2u, 1u)] = 30;

    ASSERT_EQ(map.size(), 3u);
    ASSERT_EQ(map[compages::gpu::Handle<ThingTag>(1u, 1u)], 10);
    ASSERT_EQ(map[compages::gpu::Handle<ThingTag>(1u, 2u)], 20);
    ASSERT_EQ(map[compages::gpu::Handle<ThingTag>(2u, 1u)], 30);
}

//------------------------------------------------------------------------------
// Two kinds of resource are two unrelated types, so passing a texture where a
// buffer is expected does not compile. That cannot be tested at runtime, only
// stated: the assertions below are what the compiler enforces.
//------------------------------------------------------------------------------
TEST(Handle, TellsTheKindOfResourceApart)
{
    ASSERT_FALSE((std::is_same_v<compages::gpu::Handle<ThingTag>, compages::gpu::Handle<OtherTag>>));
    ASSERT_FALSE((std::is_convertible_v<compages::gpu::Handle<ThingTag>, compages::gpu::Handle<OtherTag>>));
}

//------------------------------------------------------------------------------
// Handles are compared and hashed at every draw call, so this must all be usable
// where the compiler can fold it away.
//------------------------------------------------------------------------------
TEST(Handle, IsUsableAtCompileTime)
{
    static_assert(!compages::gpu::Handle<ThingTag>().valid());
    static_assert(compages::gpu::Handle<ThingTag>(3u, 4u).valid());
    static_assert(compages::gpu::Handle<ThingTag>(3u, 4u).index() == 3u);
    static_assert(compages::gpu::Handle<ThingTag>(3u, 4u).generation() == 4u);
    static_assert(compages::gpu::Handle<ThingTag>(3u, 4u) == compages::gpu::Handle<ThingTag>(3u, 4u));
    ASSERT_TRUE(true);
}
