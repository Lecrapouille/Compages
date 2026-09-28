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

#include "Compages/GPU/Core/Pool.hpp"

#include <memory>
#include <vector>

namespace
{

//! \brief Stands in for a resource record: a value plus something owned, so that
//! releasing a slot can be checked to really let go of it.
struct Thing
{
    int value = 0;
    std::shared_ptr<int> owned;
};

using ThingPool = compages::gpu::Pool<Thing, struct ThingTag>;
using ThingHandle = ThingPool::Handle;

} // namespace

//------------------------------------------------------------------------------
TEST(Pool, StartsEmpty)
{
    ThingPool pool;

    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_EQ(pool.slots(), 0u);
}

//------------------------------------------------------------------------------
TEST(Pool, HandsOutAHandleNamingWhatWasStored)
{
    ThingPool pool;

    auto added = pool.add(Thing{ 42, nullptr });
    ASSERT_TRUE(bool(added)) << added.error();

    const ThingHandle handle = added.take();
    ASSERT_TRUE(pool.valid(handle));
    ASSERT_NE(pool.get(handle), nullptr);
    ASSERT_EQ(pool.get(handle)->value, 42);
    ASSERT_EQ(pool.size(), 1u);
}

//------------------------------------------------------------------------------
// The first slot is number zero with a reuse count of one, never zero: a live
// handle must never look like an empty one.
//------------------------------------------------------------------------------
TEST(Pool, NeverHandsOutAHandleThatLooksEmpty)
{
    ThingPool pool;

    const ThingHandle handle = pool.add(Thing{ 1, nullptr }).take();

    ASSERT_TRUE(handle.valid());
    ASSERT_EQ(handle.index(), 0u);
    ASSERT_EQ(handle.generation(), 1u);
}

//------------------------------------------------------------------------------
TEST(Pool, RejectsAnEmptyHandle)
{
    ThingPool pool;

    ASSERT_FALSE(pool.valid(ThingHandle{}));
    ASSERT_EQ(pool.get(ThingHandle{}), nullptr);
    ASSERT_FALSE(pool.remove(ThingHandle{}));
}

//------------------------------------------------------------------------------
// A handle made up out of thin air must not be believed, otherwise a stray
// integer becomes a way to read the pool out of bounds.
//------------------------------------------------------------------------------
TEST(Pool, RejectsAHandleToASlotThatDoesNotExist)
{
    ThingPool pool;
    (void)pool.add(Thing{ 1, nullptr });

    ASSERT_FALSE(pool.valid(ThingHandle(500u, 1u)));
    ASSERT_EQ(pool.get(ThingHandle(500u, 1u)), nullptr);
}

//------------------------------------------------------------------------------
TEST(Pool, ForgetsAReleasedResource)
{
    ThingPool pool;
    const ThingHandle handle = pool.add(Thing{ 7, nullptr }).take();

    ASSERT_TRUE(pool.remove(handle));

    ASSERT_FALSE(pool.valid(handle));
    ASSERT_EQ(pool.get(handle), nullptr);
    ASSERT_TRUE(pool.empty());
}

//------------------------------------------------------------------------------
// Releasing twice is a bug in the caller. It is reported rather than corrupting
// the free list, which would hand the same slot to two owners.
//------------------------------------------------------------------------------
TEST(Pool, RefusesToReleaseTwice)
{
    ThingPool pool;
    const ThingHandle handle = pool.add(Thing{ 7, nullptr }).take();

    ASSERT_TRUE(pool.remove(handle));
    ASSERT_FALSE(pool.remove(handle));
    ASSERT_EQ(pool.slots(), 1u);
}

//------------------------------------------------------------------------------
// A released slot must let go of whatever the record owned, right away and not at
// the next reuse, otherwise a texture stays in memory long after being deleted.
//------------------------------------------------------------------------------
TEST(Pool, LetsGoOfWhatAReleasedRecordOwned)
{
    ThingPool pool;
    auto owned = std::make_shared<int>(1);
    const ThingHandle handle = pool.add(Thing{ 1, owned }).take();
    ASSERT_EQ(owned.use_count(), 2);

    pool.remove(handle);

    ASSERT_EQ(owned.use_count(), 1);
}

//------------------------------------------------------------------------------
// This is the point of the whole design: the slot comes back, but the old handle
// does not come back with it.
//------------------------------------------------------------------------------
TEST(Pool, ReusesTheSlotButNotTheHandle)
{
    ThingPool pool;
    const ThingHandle first = pool.add(Thing{ 1, nullptr }).take();
    pool.remove(first);

    const ThingHandle second = pool.add(Thing{ 2, nullptr }).take();

    ASSERT_EQ(second.index(), first.index());
    ASSERT_NE(second.generation(), first.generation());
    ASSERT_FALSE(pool.valid(first));
    ASSERT_TRUE(pool.valid(second));
    ASSERT_EQ(pool.get(second)->value, 2);
    // One slot served both, so nothing grew.
    ASSERT_EQ(pool.slots(), 1u);
}

//------------------------------------------------------------------------------
TEST(Pool, KeepsTheOtherResourcesWhereTheyWere)
{
    ThingPool pool;
    const ThingHandle a = pool.add(Thing{ 1, nullptr }).take();
    const ThingHandle b = pool.add(Thing{ 2, nullptr }).take();
    const ThingHandle c = pool.add(Thing{ 3, nullptr }).take();

    pool.remove(b);

    ASSERT_TRUE(pool.valid(a));
    ASSERT_TRUE(pool.valid(c));
    ASSERT_EQ(pool.get(a)->value, 1);
    ASSERT_EQ(pool.get(c)->value, 3);
    ASSERT_EQ(pool.size(), 2u);
}

//------------------------------------------------------------------------------
// The freed slots must all come back, otherwise a program creating and releasing
// resources every frame grows without bound.
//------------------------------------------------------------------------------
TEST(Pool, GrowsNoFurtherThanTheMostResourcesEverAlive)
{
    ThingPool pool;

    for (int round = 0; round < 100; ++round)
    {
        ThingHandle handles[4];
        for (int i = 0; i < 4; ++i)
        {
            handles[i] = pool.add(Thing{ i, nullptr }).take();
        }
        for (auto& handle : handles)
        {
            pool.remove(handle);
        }
    }

    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.slots(), 4u);
}

//------------------------------------------------------------------------------
TEST(Pool, VisitsEveryLiveResourceAndNoOther)
{
    ThingPool pool;
    const ThingHandle a = pool.add(Thing{ 1, nullptr }).take();
    const ThingHandle b = pool.add(Thing{ 2, nullptr }).take();
    const ThingHandle c = pool.add(Thing{ 3, nullptr }).take();
    pool.remove(b);

    std::vector<int> seen;
    std::vector<ThingHandle> handles;
    pool.forEach([&](ThingHandle p_handle, Thing& p_thing) {
        seen.emplace_back(p_thing.value);
        handles.emplace_back(p_handle);
    });

    ASSERT_THAT(seen, ElementsAre(1, 3));
    ASSERT_THAT(handles, ElementsAre(a, c));
}

//------------------------------------------------------------------------------
// The handle given to the visitor must be usable, since shutdown uses it to
// release what it walks over.
//------------------------------------------------------------------------------
TEST(Pool, GivesTheVisitorUsableHandles)
{
    ThingPool pool;
    (void)pool.add(Thing{ 1, nullptr });
    (void)pool.add(Thing{ 2, nullptr });

    std::vector<ThingHandle> handles;
    pool.forEach([&](ThingHandle p_handle, Thing&) {
        handles.emplace_back(p_handle);
    });

    for (auto const& handle : handles)
    {
        ASSERT_TRUE(pool.valid(handle));
        ASSERT_TRUE(pool.remove(handle));
    }
    ASSERT_TRUE(pool.empty());
}

//------------------------------------------------------------------------------
TEST(Pool, DropsEverythingWhenCleared)
{
    ThingPool pool;
    auto owned = std::make_shared<int>(1);
    const ThingHandle handle = pool.add(Thing{ 1, owned }).take();

    pool.clear();

    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.slots(), 0u);
    ASSERT_FALSE(pool.valid(handle));
    ASSERT_EQ(owned.use_count(), 1);
}

//------------------------------------------------------------------------------
// A reuse count is 16 bits, so a slot recycled often enough eventually wraps
// around. It must skip zero when it does, or a live handle would suddenly read as
// empty.
//------------------------------------------------------------------------------
TEST(Pool, SkipsZeroWhenTheReuseCountWrapsAround)
{
    ThingPool pool;

    ThingHandle handle;
    for (std::uint32_t round = 0u; round <= 0xFFFFu; ++round)
    {
        handle = pool.add(Thing{ 1, nullptr }).take();
        ASSERT_TRUE(handle.valid()) << "wrapped to an empty handle at round "
                                    << round;
        ASSERT_NE(handle.generation(), 0u);
        pool.remove(handle);
    }

    // Still the same single slot, having been through the whole range.
    ASSERT_EQ(pool.slots(), 1u);
}

//------------------------------------------------------------------------------
// A pool that cannot grow any further says so, rather than handing out a handle
// whose slot number does not fit.
//------------------------------------------------------------------------------
TEST(Pool, ReportsBeingFullInsteadOfOverflowing)
{
    compages::gpu::Pool<int, struct SmallTag> pool;

    for (std::uint32_t i = 0u; i < compages::gpu::Handle<struct SmallTag>::MAX_COUNT; ++i)
    {
        auto added = pool.add(static_cast<int>(i));
        ASSERT_TRUE(bool(added)) << "failed at " << i << ": " << added.error();
    }

    auto overflow = pool.add(0);
    ASSERT_FALSE(bool(overflow));
    ASSERT_THAT(overflow.error(), HasSubstr("too many live resources"));
}
