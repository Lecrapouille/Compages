// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPUContext.hpp"

#include "Compages/GPU/GPU.hpp"

#include <numeric>
#include <vector>

using namespace tests;

// ****************************************************************************
//! \brief A live device, on an invisible context.
// ****************************************************************************
class GrowingBufferTest: public GPUTest
{
protected:

    void SetUp() override
    {
        GPUTest::SetUp();
        auto ready = compages::gpu::init(GPUContext::procAddress());
        ASSERT_TRUE(bool(ready)) << ready.error();
    }

    void TearDown() override
    {
        compages::gpu::shutdown();
        GPUTest::TearDown();
    }

    //! \brief What the device really holds, for the first p_count elements.
    static std::vector<int> onDevice(compages::gpu::Buffer<int> const& p_array,
                                     std::size_t p_count)
    {
        std::vector<int> got(p_count);
        auto read = p_array.read(got);
        EXPECT_TRUE(bool(read)) << read.error();
        return got;
    }
};

// Filling the array must be possible before a window exists, so nothing before
// update() may touch the device.
TEST_F(GrowingBufferTest, ReservesNothingUntilAskedToSend)
{
    compages::gpu::Buffer<int> array;
    array.resize(100u);
    array.set(0u, 42);

    ASSERT_EQ(compages::gpu::liveBuffers(), 0u);
    ASSERT_FALSE(array.valid());
    ASSERT_TRUE(array.dirty());

    ASSERT_TRUE(bool(array.upload()));
    ASSERT_EQ(compages::gpu::liveBuffers(), 1u);
    ASSERT_TRUE(array.valid());
}

// The test the previous layer would have failed twice over. Changing a single
// element sent zero bytes, because the length was computed as end - start on a
// range recording start == end for one element. And when it did send something,
// it read from the front of the container while writing at the offset of the
// change, so the value landed on the wrong element.
TEST_F(GrowingBufferTest, SendsOneChangedElementToTheRightPlace)
{
    compages::gpu::Buffer<int> array;
    array.resize(8u);
    for (std::size_t i = 0u; i < 8u; ++i)
    {
        array.set(i, static_cast<int>(i));
    }
    ASSERT_TRUE(bool(array.upload()));
    ASSERT_FALSE(array.dirty());

    array.set(3u, 99);
    ASSERT_EQ(array.pending().count(), 1u);
    ASSERT_EQ(array.pending().begin(), 3u);

    ASSERT_TRUE(bool(array.upload()));

    const std::vector<int> expected{ 0, 1, 2, 99, 4, 5, 6, 7 };
    ASSERT_EQ(onDevice(array, 8u), expected);
}

// The last element is where an off by one in the length shows up.
TEST_F(GrowingBufferTest, SendsTheLastElement)
{
    compages::gpu::Buffer<int> array;
    array.resize(4u);
    ASSERT_TRUE(bool(array.upload()));

    array.set(3u, 7);
    ASSERT_TRUE(bool(array.upload()));

    const std::vector<int> expected{ 0, 0, 0, 7 };
    ASSERT_EQ(onDevice(array, 4u), expected);
}

TEST_F(GrowingBufferTest, SendsARunOfChangesInOneWrite)
{
    compages::gpu::Buffer<int> array;
    array.resize(10u);
    ASSERT_TRUE(bool(array.upload()));

    for (int& value : array.modify(4u, 3u))
    {
        value = 5;
    }
    ASSERT_EQ(array.pending().begin(), 4u);
    ASSERT_EQ(array.pending().count(), 3u);

    ASSERT_TRUE(bool(array.upload()));
    const std::vector<int> expected{ 0, 0, 0, 0, 5, 5, 5, 0, 0, 0 };
    ASSERT_EQ(onDevice(array, 10u), expected);
}

// Two far apart changes are sent as one run, unchanged elements included. What
// matters is that the unchanged ones arrive unchanged.
TEST_F(GrowingBufferTest, KeepsTheUntouchedElementsWhenSendingASpan)
{
    compages::gpu::Buffer<int> array;
    array.resize(6u);
    for (std::size_t i = 0u; i < 6u; ++i)
    {
        array.set(i, static_cast<int>(i) + 1);
    }
    ASSERT_TRUE(bool(array.upload()));

    array.set(1u, 100);
    array.set(4u, 400);
    ASSERT_EQ(array.pending().count(), 4u);

    ASSERT_TRUE(bool(array.upload()));
    const std::vector<int> expected{ 1, 100, 3, 4, 400, 6 };
    ASSERT_EQ(onDevice(array, 6u), expected);
}

// Calling update() every frame must be free when nothing moved, since that is
// how it is meant to be used.
TEST_F(GrowingBufferTest, HasNothingToSendTwiceInARow)
{
    compages::gpu::Buffer<int> array;
    array.resize(4u);
    ASSERT_TRUE(bool(array.upload()));
    ASSERT_FALSE(array.dirty());

    ASSERT_TRUE(bool(array.upload()));
    ASSERT_FALSE(array.dirty());
    ASSERT_EQ(compages::gpu::liveBuffers(), 1u);
}

// An array nobody filled has nothing to reserve, and asking the device for zero
// bytes would be refused.
TEST_F(GrowingBufferTest, SendsNothingWhenEmpty)
{
    compages::gpu::Buffer<int> array;
    ASSERT_TRUE(bool(array.upload()));
    ASSERT_EQ(compages::gpu::liveBuffers(), 0u);
}

// Reading must not cost a write to the device. Everything already sent stays
// sent.
TEST_F(GrowingBufferTest, ReadingChangesNothing)
{
    compages::gpu::Buffer<int> array;
    array.resize(4u);
    array.set(2u, 8);
    ASSERT_TRUE(bool(array.upload()));

    compages::gpu::Buffer<int> const& reading = array;
    ASSERT_EQ(reading[2], 8);
    ASSERT_EQ(reading.elements()[2], 8);
    ASSERT_EQ(reading.size(), 4u);

    ASSERT_FALSE(array.dirty());
}

// The case of the growing curve. Reserving memory on the device cannot be
// undone, so growing means a new, larger block, and everything has to be sent
// again for the new block to hold anything.
TEST_F(GrowingBufferTest, GrowsTheDeviceMemoryKeepingWhatWasThere)
{
    compages::gpu::Buffer<int> array;
    array.emplace_back(1);
    array.emplace_back(2);
    ASSERT_TRUE(bool(array.upload()));
    ASSERT_EQ(array.deviceCapacity(), 2u);

    for (int value = 3; value <= 9; ++value)
    {
        array.emplace_back(value);
    }
    ASSERT_TRUE(bool(array.upload()));

    ASSERT_EQ(array.size(), 9u);
    ASSERT_GE(array.deviceCapacity(), 9u);
    // One block at a time: the old one is released as the new one takes over.
    ASSERT_EQ(compages::gpu::liveBuffers(), 1u);

    const std::vector<int> expected{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    ASSERT_EQ(onDevice(array, 9u), expected);
}

// The curve that gains a point per frame, which is the demanding case: asking
// the device for a new block on every frame would mean sending the whole curve
// again every frame, turning a constant cost into one that grows without bound.
// Each new block is twice the last, so a thousand frames cost a handful of them.
TEST_F(GrowingBufferTest, AsksForNewMemoryOnlyAHandfulOfTimesWhileGrowing)
{
    compages::gpu::Buffer<int> array;
    compages::gpu::BufferHandle previous;
    std::size_t reallocations = 0u;

    for (int value = 0; value < 1000; ++value)
    {
        array.emplace_back(value);
        ASSERT_TRUE(bool(array.upload()));
        if (array.handle() != previous)
        {
            ++reallocations;
            previous = array.handle();
        }
    }

    ASSERT_EQ(array.size(), 1000u);
    ASSERT_LT(reallocations, 16u);
    ASSERT_EQ(compages::gpu::liveBuffers(), 1u);

    const std::vector<int> got = onDevice(array, 1000u);
    ASSERT_EQ(got.front(), 0);
    ASSERT_EQ(got.back(), 999);
}

// Saying the final count up front is what a caller does to avoid the growing
// altogether: the device memory is then reserved once.
TEST_F(GrowingBufferTest, ReservesWhatItWasToldToExpect)
{
    compages::gpu::Buffer<int> array;
    array.reserve(1000u);
    array.emplace_back(1);
    ASSERT_TRUE(bool(array.upload()));

    ASSERT_EQ(array.deviceCapacity(), 1000u);

    const compages::gpu::BufferHandle same = array.handle();
    for (int value = 0; value < 500; ++value)
    {
        array.emplace_back(value);
    }
    ASSERT_TRUE(bool(array.upload()));
    ASSERT_EQ(array.handle(), same);
}

// A draw call is told how many elements to read, so shrinking has nothing to
// send: what lies beyond the new end is never looked at.
TEST_F(GrowingBufferTest, SendsNothingWhenItShrinks)
{
    compages::gpu::Buffer<int> array;
    array.resize(8u);
    ASSERT_TRUE(bool(array.upload()));

    array.resize(3u);
    ASSERT_FALSE(array.dirty());
    ASSERT_EQ(array.size(), 3u);
    // The memory is kept, ready for the array to grow back into it.
    ASSERT_EQ(array.deviceCapacity(), 8u);
}

// A change waiting to be sent, then dropped by a shrink, must not be read from
// past the end of the container on its way to the device.
TEST_F(GrowingBufferTest, ForgetsAChangeThatTheShrinkRemoved)
{
    compages::gpu::Buffer<int> array;
    array.resize(8u);
    ASSERT_TRUE(bool(array.upload()));

    array.set(7u, 123);
    array.resize(4u);

    ASSERT_FALSE(array.dirty());
    ASSERT_TRUE(bool(array.upload()));
}

// A shrink followed by a change: only the part that still exists is sent.
TEST_F(GrowingBufferTest, KeepsTheSurvivingPartOfAChange)
{
    compages::gpu::Buffer<int> array;
    array.resize(8u);
    ASSERT_TRUE(bool(array.upload()));

    for (int& value : array.modify(2u, 6u))
    {
        value = 5;
    }
    array.resize(5u);

    ASSERT_TRUE(array.dirty());
    ASSERT_EQ(array.pending().begin(), 2u);
    ASSERT_EQ(array.pending().end(), 5u);

    ASSERT_TRUE(bool(array.upload()));
    const std::vector<int> expected{ 0, 0, 5, 5, 5 };
    ASSERT_EQ(onDevice(array, 5u), expected);
}

// The animated surface: everything moves every frame, so everything is sent.
TEST_F(GrowingBufferTest, SendsEverythingWhenEverythingMoves)
{
    compages::gpu::Buffer<int> array;
    array.resize(1000u);
    ASSERT_TRUE(bool(array.upload()));

    int next = 0;
    for (int& value : array.modify())
    {
        value = next++;
    }
    ASSERT_EQ(array.pending().count(), 1000u);
    ASSERT_TRUE(bool(array.upload()));

    const std::vector<int> got = onDevice(array, 1000u);
    ASSERT_EQ(got.front(), 0);
    ASSERT_EQ(got.back(), 999);
}

TEST_F(GrowingBufferTest, ReplacesItsWholeContents)
{
    compages::gpu::Buffer<int> array;
    array.resize(3u);
    ASSERT_TRUE(bool(array.upload()));

    const std::vector<int> fresh{ 7, 8, 9, 10 };
    array.assign(fresh);
    ASSERT_EQ(array.size(), 4u);
    ASSERT_TRUE(bool(array.upload()));

    ASSERT_EQ(onDevice(array, 4u), fresh);
}

// Built from a vector, with everything already waiting to be sent.
TEST_F(GrowingBufferTest, CanBeBuiltFromDataItAlreadyHas)
{
    std::vector<int> source(16u);
    std::iota(source.begin(), source.end(), 100);

    compages::gpu::Buffer<int> array{ std::span<const int>(source) };
    ASSERT_EQ(array.size(), 16u);
    ASSERT_TRUE(array.dirty());

    ASSERT_TRUE(bool(array.upload()));
    ASSERT_EQ(onDevice(array, 16u), source);
}

// Indices are the same mechanism with another use, which is the point of one
// class rather than one per kind of buffer.
TEST_F(GrowingBufferTest, HoldsIndicesJustAsWell)
{
    compages::gpu::Buffer<std::uint32_t> indices(compages::gpu::BufferKind::Index);
    const std::vector<std::uint32_t> triangles{ 0u, 1u, 2u, 2u, 3u, 0u };
    indices.assign(triangles);

    ASSERT_TRUE(bool(indices.upload()));
    ASSERT_EQ(indices.kind(), compages::gpu::BufferKind::Index);
    ASSERT_EQ(indices.kind(), compages::gpu::BufferKind::Index);

    auto read = indices.read();
    ASSERT_TRUE(bool(read)) << read.error();
    ASSERT_EQ(read.value(), triangles);
}

// The array owns its device memory, so leaving its scope must bring the count
// back to zero without anything to call by hand.
TEST_F(GrowingBufferTest, ReleasesItsMemoryWhenItGoesOutOfScope)
{
    {
        compages::gpu::Buffer<int> array;
        array.resize(4u);
        ASSERT_TRUE(bool(array.upload()));
        ASSERT_EQ(compages::gpu::liveBuffers(), 1u);
    }
    ASSERT_EQ(compages::gpu::liveBuffers(), 0u);
}

// Reaching an element in order to change it has to be assumed to change it, so
// it is sent. Callers who only read say so, and pay nothing.
TEST_F(GrowingBufferTest, AssumesAWritableReferenceIsWritten)
{
    compages::gpu::Buffer<int> array;
    array.resize(4u);
    ASSERT_TRUE(bool(array.upload()));

    array[2] = 5;
    ASSERT_TRUE(array.dirty());
    ASSERT_EQ(array.pending().begin(), 2u);
    ASSERT_EQ(array.pending().count(), 1u);

    ASSERT_TRUE(bool(array.upload()));
    const std::vector<int> expected{ 0, 0, 5, 0 };
    ASSERT_EQ(onDevice(array, 4u), expected);
}
