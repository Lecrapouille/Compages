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

#include "Compages/GPU/Core/DirtyRange.hpp"

//------------------------------------------------------------------------------
TEST(TestDirtyRange, StartsWithNothingToSend)
{
    compages::gpu::DirtyRange range;
    ASSERT_TRUE(range.empty());
    ASSERT_EQ(range.count(), 0u);
}

//------------------------------------------------------------------------------
// The bug this type was written to make impossible. Its ancestor recorded one
// changed element as start == end == pos, and the code sending the bytes computed
// the length as end - start, so a single change sent nothing at all and the
// screen never showed it.
//------------------------------------------------------------------------------
TEST(TestDirtyRange, OneChangedElementIsOneElementToSend)
{
    compages::gpu::DirtyRange range;
    range.add(5u);

    ASSERT_FALSE(range.empty());
    ASSERT_EQ(range.begin(), 5u);
    ASSERT_EQ(range.end(), 6u);
    ASSERT_EQ(range.count(), 1u);
}

//------------------------------------------------------------------------------
// Element zero is the case where an emptiness marked by a pair of zeroes could
// be mistaken for a change, or the other way round.
//------------------------------------------------------------------------------
TEST(TestDirtyRange, TellsAChangedFirstElementFromNoChangeAtAll)
{
    compages::gpu::DirtyRange range;
    range.add(0u);

    ASSERT_FALSE(range.empty());
    ASSERT_EQ(range.begin(), 0u);
    ASSERT_EQ(range.count(), 1u);
}

//------------------------------------------------------------------------------
// Adding to an empty range must not stretch it back to zero: the first change
// defines both bounds, it does not widen them.
//------------------------------------------------------------------------------
TEST(TestDirtyRange, DoesNotStretchBackToTheFrontOnItsFirstChange)
{
    compages::gpu::DirtyRange range;
    range.add(100u);

    ASSERT_EQ(range.begin(), 100u);
    ASSERT_EQ(range.count(), 1u);
}

//------------------------------------------------------------------------------
TEST(TestDirtyRange, GrowsToCoverEveryChange)
{
    compages::gpu::DirtyRange range;
    range.add(1u);
    range.add(5u);

    ASSERT_EQ(range.begin(), 1u);
    ASSERT_EQ(range.end(), 6u);
    ASSERT_EQ(range.count(), 5u);

    range.add(0u);
    ASSERT_EQ(range.begin(), 0u);
    ASSERT_EQ(range.end(), 6u);
    ASSERT_EQ(range.count(), 6u);
}

//------------------------------------------------------------------------------
// Elements between two changes are sent although they did not change. One write
// of the whole span costs far less than one write per element, which is the trade
// this type is making.
//------------------------------------------------------------------------------
TEST(TestDirtyRange, SendsTheUnchangedElementsBetweenTwoChanges)
{
    compages::gpu::DirtyRange range;
    range.add(0u);
    range.add(999u);

    ASSERT_EQ(range.count(), 1000u);
}

//------------------------------------------------------------------------------
TEST(TestDirtyRange, RecordsARunOfChanges)
{
    compages::gpu::DirtyRange range;
    range.add(10u, 4u);

    ASSERT_EQ(range.begin(), 10u);
    ASSERT_EQ(range.end(), 14u);
    ASSERT_EQ(range.count(), 4u);
}

//------------------------------------------------------------------------------
// A run of nothing is what an edit of an empty selection looks like, and it must
// not turn into a change at that index.
//------------------------------------------------------------------------------
TEST(TestDirtyRange, IgnoresARunOfNoElements)
{
    compages::gpu::DirtyRange range;
    range.add(10u, 0u);
    ASSERT_TRUE(range.empty());

    range.add(3u);
    range.add(8u, 0u);
    ASSERT_EQ(range.begin(), 3u);
    ASSERT_EQ(range.count(), 1u);
}

//------------------------------------------------------------------------------
TEST(TestDirtyRange, CoversAWholeContainer)
{
    compages::gpu::DirtyRange range;
    range.addAll(7u);

    ASSERT_EQ(range.begin(), 0u);
    ASSERT_EQ(range.end(), 7u);
    ASSERT_EQ(range.count(), 7u);
}

//------------------------------------------------------------------------------
TEST(TestDirtyRange, CoversNothingOfAnEmptyContainer)
{
    compages::gpu::DirtyRange range;
    range.add(2u);
    range.addAll(0u);

    ASSERT_TRUE(range.empty());
}

//------------------------------------------------------------------------------
TEST(TestDirtyRange, ForgetsEverythingOnceSent)
{
    compages::gpu::DirtyRange range;
    range.add(1u, 9u);
    range.clear();

    ASSERT_TRUE(range.empty());
    ASSERT_EQ(range.count(), 0u);
}

//------------------------------------------------------------------------------
// After the container shrinks, a range still naming elements that are gone would
// read past the end of the vector on the way to the device.
//------------------------------------------------------------------------------
TEST(TestDirtyRange, DropsWhatNoLongerExistsAfterAShrink)
{
    compages::gpu::DirtyRange range;
    range.add(2u, 8u);
    ASSERT_EQ(range.end(), 10u);

    range.clampTo(5u);
    ASSERT_EQ(range.begin(), 2u);
    ASSERT_EQ(range.end(), 5u);
    ASSERT_EQ(range.count(), 3u);
}

//------------------------------------------------------------------------------
TEST(TestDirtyRange, DropsEverythingWhenTheShrinkGoesPastTheChange)
{
    compages::gpu::DirtyRange range;
    range.add(8u, 2u);

    range.clampTo(4u);
    ASSERT_TRUE(range.empty());
}

//------------------------------------------------------------------------------
TEST(TestDirtyRange, LeavesARangeThatStillFitsAlone)
{
    compages::gpu::DirtyRange range;
    range.add(1u, 2u);

    range.clampTo(100u);
    ASSERT_EQ(range.begin(), 1u);
    ASSERT_EQ(range.end(), 3u);
}
