//=====================================================================
// OpenGLCppWrapper: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
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
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#include "main.hpp"
#define protected public
#define private public
#include "OpenGL/Buffers/PendingContainer.hpp"
#undef protected
#undef private

static constexpr size_t npos = static_cast<size_t>(-1); // FIXME Pending::npos

//--------------------------------------------------------------------------
TEST(PendingContainerTests, TestConstructors)
{
    size_t pos_start;
    size_t pos_end;

    // --- Test 1
    PendingContainer<int> pc0;
    ASSERT_EQ(false, pc0.isPending());
    pc0.getPending(pos_start, pos_end);
    ASSERT_EQ(npos, pos_start);
    ASSERT_EQ(npos, pos_end);
    ASSERT_EQ(0u, pc0.m_container.size());
    ASSERT_EQ(0u, pc0.m_container.capacity());
    ASSERT_EQ(0u, pc0.size());
    ASSERT_EQ(0u, pc0.capacity());
    pc0.resize(0u);
    ASSERT_EQ(false, pc0.isPending());
    pc0.getPending(pos_start, pos_end);
    ASSERT_EQ(npos, pos_start);
    ASSERT_EQ(npos, pos_end);

    // --- Test 2
    PendingContainer<int> pc1(10u);
    ASSERT_EQ(false, pc1.isPending());
    ASSERT_EQ(npos, pc1.getPending().first);
    ASSERT_EQ(npos, pc1.getPending().second);
    ASSERT_EQ(0u, pc1.m_container.size());
    ASSERT_EQ(10u, pc1.m_container.capacity());
    ASSERT_EQ(0u, pc1.size());
    ASSERT_EQ(10u, pc1.capacity());
    pc1.reserve(20u);
    ASSERT_EQ(false, pc1.isPending());
    ASSERT_EQ(0u, pc1.size());
    ASSERT_EQ(20u, pc1.capacity());
    pc1.resize(10u);
    ASSERT_EQ(10u, pc1.size());
    ASSERT_EQ(20u, pc1.capacity());
    ASSERT_EQ(true, pc1.isPending());
    ASSERT_EQ(0u, pc1.getPending().first);
    ASSERT_EQ(10u, pc1.getPending().second);

    // --- Test 3
    PendingContainer<int> pc2(10u, 42);
    ASSERT_EQ(true, pc2.isPending());
    ASSERT_EQ(0u, pc2.getPending().first);
    ASSERT_EQ(10u, pc2.getPending().second);
    ASSERT_EQ(10u, pc2.m_container.size());
    ASSERT_EQ(10u, pc2.m_container.capacity());
    ASSERT_EQ(10u, pc2.size());
    ASSERT_EQ(10u, pc2.capacity());
    for (auto it : pc2.m_container)
    {
        ASSERT_EQ(42, it);
    }
    pc2.reserve(20u);
    ASSERT_EQ(10u, pc2.size());
    ASSERT_EQ(20u, pc2.capacity());
    pc2.resize(2u);
    ASSERT_EQ(2u, pc2.size());
    ASSERT_EQ(20u, pc2.capacity());
    for (auto it : pc2.m_container)
    {
        ASSERT_EQ(42, it);
    }
    ASSERT_EQ(true, pc2.isPending());
    ASSERT_EQ(0u, pc2.getPending().first);
    ASSERT_EQ(2u, pc2.getPending().second);
    pc2.resize(5u);
    ASSERT_EQ(5u, pc2.size());
    ASSERT_EQ(20u, pc2.capacity());
    ASSERT_EQ(true, pc2.isPending());
    ASSERT_EQ(0u, pc2.getPending().first);
    ASSERT_EQ(5u, pc2.getPending().second);
    size_t i = pc2.size();
    while (i--)
    {
        if (i < 2u)
        {
            ASSERT_EQ(42, pc2.m_container[i]);
        }
        else
        {
            ASSERT_EQ(0, pc2.m_container[i]);
        }
    }

    // --- Test 4
    PendingContainer<int> pc3(pc1);
    ASSERT_EQ(true, pc3.isPending());
    ASSERT_EQ(0, pc3.getPending().first);
    ASSERT_EQ(pc1.size(), pc3.getPending().second);
    ASSERT_EQ(pc1.getPending().first, pc3.getPending().first);
    ASSERT_EQ(pc1.getPending().second, pc3.getPending().second);
    ASSERT_EQ(10u, pc3.m_container.size());
    ASSERT_EQ(20u, pc3.m_container.capacity());
    ASSERT_EQ(10u, pc3.size());
    ASSERT_EQ(20u, pc3.capacity());
    ASSERT_EQ(pc1.size(), pc3.size());
    ASSERT_EQ(pc1.capacity(), pc3.capacity());
    pc3.reserve(5u);
    ASSERT_EQ(10u, pc3.size());
    ASSERT_EQ(20u, pc3.capacity());
    ASSERT_EQ(true, pc3.isPending());
    ASSERT_EQ(0u, pc3.getPending().first);
    ASSERT_EQ(10u, pc3.getPending().second);
    pc3.resize(10u);
    ASSERT_EQ(10u, pc3.size());
    ASSERT_EQ(20u, pc3.capacity());
    ASSERT_EQ(true, pc3.isPending());
    ASSERT_EQ(0u, pc3.getPending().first);
    ASSERT_EQ(10u, pc3.getPending().second);

    // --- Test 5
    PendingContainer<int> pc4(pc2);
    ASSERT_EQ(true, pc4.isPending());
    ASSERT_EQ(0u, pc4.getPending().first);
    ASSERT_EQ(5u, pc4.getPending().second);
    ASSERT_EQ(pc2.getPending().first, pc4.getPending().first);
    ASSERT_EQ(pc2.getPending().second, pc4.getPending().second);
    ASSERT_EQ(5u, pc4.m_container.size());
    ASSERT_EQ(20u, pc4.m_container.capacity());
    ASSERT_EQ(5u, pc4.size());
    ASSERT_EQ(20u, pc4.capacity());
    ASSERT_EQ(pc2.size(), pc4.size());
    ASSERT_EQ(pc2.capacity(), pc4.capacity());
    i = pc4.size();
    while (i--)
    {
        if (i < 2u)
        {
            ASSERT_EQ(42, pc4.m_container[i]);
        }
        else
        {
            ASSERT_EQ(0, pc4.m_container[i]);
        }
    }

    pc4.reserve(0u);
    ASSERT_EQ(5u, pc4.size());
    ASSERT_EQ(20u, pc4.capacity());

    pc4.resize(0u);
    ASSERT_EQ(true, pc4.isPending());
    ASSERT_EQ(0u, pc4.getPending().first);
    ASSERT_EQ(5u, pc4.getPending().second);

    // --- Test 6
    PendingContainer<int> pc5({ 66, 42, 55 });
    ASSERT_EQ(true, pc5.isPending());
    ASSERT_EQ(0u, pc5.getPending().first);
    ASSERT_EQ(3u, pc5.getPending().second);
    ASSERT_EQ(3u, pc5.m_container.size());
    ASSERT_EQ(3u, pc5.m_container.capacity());
    ASSERT_EQ(3u, pc5.size());
    ASSERT_EQ(3u, pc5.capacity());
    ASSERT_EQ(66, pc5.m_container[0]);
    ASSERT_EQ(42, pc5.m_container[1]);
    ASSERT_EQ(55, pc5.m_container[2]);
    pc5.reserve(10u);
    ASSERT_EQ(3u, pc5.size());
    ASSERT_EQ(10u, pc5.capacity());

    pc5.clearPending();
    ASSERT_EQ(false, pc5.isPending());
    pc5.append({ 56, 57, 58, 59, 60 });
    ASSERT_EQ(true, pc5.isPending());
    ASSERT_EQ(3u, pc5.getPending().first);
    ASSERT_EQ(8u, pc5.getPending().second);
    pc5.resize(2u);
    ASSERT_EQ(true, pc5.isPending());
    ASSERT_EQ(0u, pc5.getPending().first);
    ASSERT_EQ(2u, pc5.getPending().second);
    pc5.resize(0u);
    ASSERT_EQ(true, pc5.isPending());
    ASSERT_EQ(0u, pc5.getPending().first);
    ASSERT_EQ(2u, pc5.getPending().second);
    pc5.resize(10u);
    ASSERT_EQ(true, pc5.isPending());
    ASSERT_EQ(0u, pc5.getPending().first);
    ASSERT_EQ(10u, pc5.getPending().second);
    int a = pc5.get(0);
    ASSERT_EQ(true, pc5.isPending());
    ASSERT_EQ(0, pc5.getPending().first);
    ASSERT_EQ(10u, pc5.getPending().second);
    pc5.set(0) = a;
    ASSERT_EQ(true, pc5.isPending());
    ASSERT_EQ(0u, pc5.getPending().first);
    ASSERT_EQ(10u, pc5.getPending().second);

    ASSERT_EQ(10u, pc5.m_container.capacity());
    ASSERT_EQ(10u, pc5.m_container.size());

    pc5.set(20) = a;
    ASSERT_EQ(21u, pc5.m_container.size());
    ASSERT_EQ(21u, pc5.m_container.capacity());
    ASSERT_EQ(true, pc5.isPending());
    ASSERT_EQ(0u, pc5.getPending().first);
    ASSERT_EQ(21u, pc5.getPending().second);

    pc5.append(std::vector<int>({ 1, 2, 3 }));
    ASSERT_EQ(true, pc5.isPending());
    ASSERT_EQ(0u, pc5.getPending().first);
    ASSERT_EQ(24u, pc5.getPending().second);
    ASSERT_EQ(24u, pc5.size());
    ASSERT_EQ(42u, pc5.capacity());

    pc5.append(4);
    ASSERT_EQ(true, pc5.isPending());
    ASSERT_EQ(0u, pc5.getPending().first);
    ASSERT_EQ(25u, pc5.getPending().second);
    ASSERT_EQ(25u, pc5.size());
    ASSERT_EQ(42u, pc5.capacity());

    pc5.append(PendingContainer<int>({ 5 }));
    ASSERT_EQ(true, pc5.isPending());
    ASSERT_EQ(0u, pc5.getPending().first);
    ASSERT_EQ(26u, pc5.getPending().second);
    ASSERT_EQ(26u, pc5.size());
    ASSERT_EQ(42u, pc5.capacity());

    pc5.clear();
    ASSERT_EQ(26u, pc5.size());
    ASSERT_EQ(42u, pc5.capacity());
    ASSERT_EQ(false, pc5.isPending());
    ASSERT_EQ(npos, pc5.getPending().first);
    ASSERT_EQ(npos, pc5.getPending().second);
}

//--------------------------------------------------------------------------
TEST(PendingContainerTests, TestArithmeticsValid)
{
    PendingContainer<int> pc1({ 42, 5, 50, 10 });
    ASSERT_EQ(true, pc1.isPending());
    ASSERT_EQ(0u, pc1.getPending().first);
    ASSERT_EQ(4u, pc1.getPending().second);
    pc1.clearPending();
    ASSERT_EQ(false, pc1.isPending());
    ASSERT_EQ(107, pc1.sum());
    ASSERT_EQ(false, pc1.isPending());
    ASSERT_EQ(105000, pc1.prod());
    ASSERT_EQ(false, pc1.isPending());
    ASSERT_EQ(5, pc1.min());
    ASSERT_EQ(false, pc1.isPending());
    ASSERT_EQ(50, pc1.max());
    ASSERT_EQ(false, pc1.isPending());

    PendingContainer<int> pc2({ -42, -5, -50, -10 });
    ASSERT_EQ(true, pc2.isPending());
    pc2.clearPending();
    pc2.abs();
    ASSERT_EQ(std::vector<int>({ 42, 5, 50, 10 }), pc2.m_container);
    ASSERT_EQ(true, pc2.isPending());
    ASSERT_EQ(0u, pc2.getPending().first);
    ASSERT_EQ(4u, pc2.getPending().second);

    PendingContainer<float> pc3({ 4.0f, 9.0f, 16.0f, 25.0f });
    pc3.clearPending();
    pc3.sqrt();
    ASSERT_EQ(std::vector<float>({ 2.0f, 3.0f, 4.0f, 5.0f }), pc3.m_container);
    ASSERT_EQ(true, pc3.isPending());
    ASSERT_EQ(0u, pc3.getPending().first);
    ASSERT_EQ(4u, pc3.getPending().second);
    pc3.squared();
    ASSERT_EQ(std::vector<float>({ 4.0f, 9.0f, 16.0f, 25.0f }),
              pc3.m_container);
    pc3.cos();
    ASSERT_EQ(std::vector<float>({ std::cos(4.0f),
                                   std::cos(9.0f),
                                   std::cos(16.0f),
                                   std::cos(25.0f) }),
              pc3.m_container);
    pc3.clearPending();

    PendingContainer<float> pc4;
    pc4 = pc3;
    ASSERT_EQ(true, pc4.isPending());
    ASSERT_EQ(0u, pc4.getPending().first);
    ASSERT_EQ(4u, pc4.getPending().second);
    ASSERT_EQ(std::vector<float>({ std::cos(4.0f),
                                   std::cos(9.0f),
                                   std::cos(16.0f),
                                   std::cos(25.0f) }),
              pc4.m_container);

    pc4.clearPending();
    pc4 = std::vector<float>({ 2.0f, 3.0f, 4.0f, 5.0f });
    ASSERT_EQ(true, pc4.isPending());
    ASSERT_EQ(0u, pc4.getPending().first);
    ASSERT_EQ(4u, pc4.getPending().second);
    pc4.sin();
    ASSERT_EQ(
        std::vector<float>(
            { std::sin(2.0f), std::sin(3.0f), std::sin(4.0f), std::sin(5.0f) }),
        pc4.m_container);
    ASSERT_EQ(true, pc4.isPending());
    ASSERT_EQ(0u, pc4.getPending().first);
    ASSERT_EQ(4u, pc4.getPending().second);

    pc4.clear();
    ASSERT_EQ(false, pc4.isPending());
    pc4.clearPending();
    pc4 = { 2.0f, 3.0f, 4.0f, 5.0f };
    ASSERT_EQ(true, pc4.isPending());
    ASSERT_EQ(0u, pc4.getPending().first);
    ASSERT_EQ(4u, pc4.getPending().second);
    ASSERT_EQ(std::vector<float>({ 2.0f, 3.0f, 4.0f, 5.0f }), pc4.m_container);

    pc4.clearPending();
    pc4 *= 2.0f;
    ASSERT_EQ(true, pc4.isPending());
    ASSERT_EQ(0u, pc4.getPending().first);
    ASSERT_EQ(4u, pc4.getPending().second);
    ASSERT_EQ(std::vector<float>({ 4.0f, 6.0f, 8.0f, 10.0f }), pc4.m_container);

    pc4 += 2.0f;
    ASSERT_EQ(std::vector<float>({ 6.0f, 8.0f, 10.0f, 12.0f }),
              pc4.m_container);
    pc4 -= 2.0f;
    ASSERT_EQ(std::vector<float>({ 4.0f, 6.0f, 8.0f, 10.0f }), pc4.m_container);
    pc4 /= 2.0f;
    ASSERT_EQ(std::vector<float>({ 2.0f, 3.0f, 4.0f, 5.0f }), pc4.m_container);

    std::cout << pc4 << std::endl;
    float* a = pc4.to_array();
    float const* ca = pc4.to_array();
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, ca);
    ASSERT_EQ(2.0f, a[0]);
    ASSERT_EQ(2.0f, ca[0]);
    ASSERT_EQ(3.0f, a[1]);
    ASSERT_EQ(3.0f, ca[1]);
    ASSERT_EQ(4.0f, a[2]);
    ASSERT_EQ(4.0f, ca[2]);
    ASSERT_EQ(5.0f, a[3]);
    ASSERT_EQ(5.0f, ca[3]);

    PendingContainer<int> pc0;
    int* a0 = pc0.to_array();
    int const* ca0 = pc0.to_array();
    ASSERT_EQ(nullptr, a0);
    ASSERT_EQ(nullptr, ca0);

    PendingContainer<int> const pc00;
    int const* ca00 = pc00.to_array();
    ASSERT_EQ(nullptr, ca00);
}

//--------------------------------------------------------------------------
TEST(PendingContainerTests, TestArithmeticsInvalid)
{
    PendingContainer<int> pc0;
    try
    {
        pc0.sum();
        ASSERT_FALSE("Exception not caught");
    }
    catch (std::out_of_range&)
    {
    }

    try
    {
        pc0.prod();
        ASSERT_FALSE("Exception not caught");
    }
    catch (std::out_of_range&)
    {
    }

    try
    {
        pc0.min();
        ASSERT_FALSE("Exception not caught");
    }
    catch (std::out_of_range&)
    {
    }

    try
    {
        pc0.max();
        ASSERT_FALSE("Exception not caught");
    }
    catch (std::out_of_range&)
    {
    }
}

TEST(PendingContainerTests, TestNotExpandable)
{
    PendingContainer<int> pc0;
    ASSERT_EQ(true, pc0.m_can_expand);
    pc0.set_cannot_expand();
    ASSERT_EQ(false, pc0.m_can_expand);
    ASSERT_EQ(0u, pc0.size());

    try
    {
        pc0 = { 4, 6, 8, 10 };
        ASSERT_FALSE("Exception not caught");
    }
    catch (std::out_of_range&)
    {
        ASSERT_EQ(false, pc0.isPending());
        ASSERT_EQ(npos, pc0.getPending().first);
        ASSERT_EQ(npos, pc0.getPending().second);
        ASSERT_EQ(0u, pc0.size());
    }

    try
    {
        pc0.append({ 4, 6, 8, 10 });
        ASSERT_FALSE("Exception not caught");
    }
    catch (std::out_of_range&)
    {
        ASSERT_EQ(false, pc0.isPending());
        ASSERT_EQ(npos, pc0.getPending().first);
        ASSERT_EQ(npos, pc0.getPending().second);
        ASSERT_EQ(0u, pc0.size());
    }

    ASSERT_EQ(0u, pc0.capacity());
    try
    {
        pc0.reserve(10u);
        ASSERT_FALSE("Exception not caught");
    }
    catch (std::out_of_range&)
    {
        ASSERT_EQ(false, pc0.isPending());
        ASSERT_EQ(npos, pc0.getPending().first);
        ASSERT_EQ(npos, pc0.getPending().second);
        ASSERT_EQ(0u, pc0.size());
        ASSERT_EQ(0u, pc0.capacity());
    }

    try
    {
        pc0.resize(10u);
        ASSERT_FALSE("Exception not caught");
    }
    catch (std::out_of_range&)
    {
        ASSERT_EQ(false, pc0.isPending());
        ASSERT_EQ(npos, pc0.getPending().first);
        ASSERT_EQ(npos, pc0.getPending().second);
        ASSERT_EQ(0u, pc0.size());
        ASSERT_EQ(0u, pc0.capacity());
    }
}
