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
using ThingHandle = compages::gpu::Handle<struct ThingTag>;
using OtherHandle = compages::gpu::Handle<struct OtherTag>;
} // namespace

//------------------------------------------------------------------------------
// A handle must cost no more than the integer it wraps, otherwise storing one per
// scene node stops being free.
//------------------------------------------------------------------------------
TEST(Handle, IsJustAnInteger)
{
    ASSERT_EQ(sizeof(ThingHandle), sizeof(std::uint32_t));
    ASSERT_TRUE(std::is_trivially_copyable_v<ThingHandle>);
}

//------------------------------------------------------------------------------
// A freshly declared handle names nothing, so forgetting to assign one is caught
// rather than pointing at slot zero.
//------------------------------------------------------------------------------
TEST(Handle, IsEmptyByDefault)
{
    ThingHandle handle;
    ASSERT_FALSE(handle.valid());
    ASSERT_FALSE(bool(handle));
    ASSERT_EQ(handle.bits(), 0u);
}

//------------------------------------------------------------------------------
TEST(Handle, RemembersItsSlotAndReuseCount)
{
    const ThingHandle handle(42u, 7u);

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
    const ThingHandle handle(0u, 1u);

    ASSERT_TRUE(handle.valid());
    ASSERT_EQ(handle.index(), 0u);
    ASSERT_EQ(handle.generation(), 1u);
}

//------------------------------------------------------------------------------
TEST(Handle, HoldsTheWholeRangeOfSlotsAndCounts)
{
    const ThingHandle handle(0xFFFFu, 0xFFFFu);

    ASSERT_EQ(handle.index(), 0xFFFFu);
    ASSERT_EQ(handle.generation(), 0xFFFFu);
    ASSERT_EQ(ThingHandle::MAX_COUNT, 0xFFFFu);
}

//------------------------------------------------------------------------------
// Same slot, different reuse count, means a different resource. This is the whole
// mechanism that turns using a released resource into a detectable mistake.
//------------------------------------------------------------------------------
TEST(Handle, DiffersFromAnOlderUseOfTheSameSlot)
{
    const ThingHandle before(5u, 1u);
    const ThingHandle after(5u, 2u);

    ASSERT_FALSE(before == after);
    ASSERT_TRUE(before != after);
    ASSERT_TRUE(before < after);
}

//------------------------------------------------------------------------------
TEST(Handle, ComparesEqualToItsOwnCopy)
{
    const ThingHandle handle(11u, 3u);
    const ThingHandle copy = handle;

    ASSERT_TRUE(handle == copy);
    ASSERT_EQ(handle.bits(), copy.bits());
}

//------------------------------------------------------------------------------
TEST(Handle, WorksAsAKeyOfAHashMap)
{
    std::unordered_map<ThingHandle, int> map;
    map[ThingHandle(1u, 1u)] = 10;
    map[ThingHandle(1u, 2u)] = 20;
    map[ThingHandle(2u, 1u)] = 30;

    ASSERT_EQ(map.size(), 3u);
    ASSERT_EQ(map[ThingHandle(1u, 1u)], 10);
    ASSERT_EQ(map[ThingHandle(1u, 2u)], 20);
    ASSERT_EQ(map[ThingHandle(2u, 1u)], 30);
}

//------------------------------------------------------------------------------
// Two kinds of resource are two unrelated types, so passing a texture where a
// buffer is expected does not compile. That cannot be tested at runtime, only
// stated: the assertions below are what the compiler enforces.
//------------------------------------------------------------------------------
TEST(Handle, TellsTheKindOfResourceApart)
{
    ASSERT_FALSE((std::is_same_v<ThingHandle, OtherHandle>));
    ASSERT_FALSE((std::is_convertible_v<ThingHandle, OtherHandle>));
}

//------------------------------------------------------------------------------
// Handles are compared and hashed at every draw call, so this must all be usable
// where the compiler can fold it away.
//------------------------------------------------------------------------------
TEST(Handle, IsUsableAtCompileTime)
{
    static_assert(!ThingHandle().valid());
    static_assert(ThingHandle(3u, 4u).valid());
    static_assert(ThingHandle(3u, 4u).index() == 3u);
    static_assert(ThingHandle(3u, 4u).generation() == 4u);
    static_assert(ThingHandle(3u, 4u) == ThingHandle(3u, 4u));
    ASSERT_TRUE(true);
}
