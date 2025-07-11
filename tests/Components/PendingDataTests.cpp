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
#include "Common/Pending.hpp"
#undef protected
#undef private

static constexpr size_t npos = static_cast<size_t>(-1); // FIXME Pending::npos

//--------------------------------------------------------------------------
TEST(TestPending, TestEmptyConstructor)
{
    Pending pd;

    ASSERT_EQ(npos, pd.m_pending_start);
    ASSERT_EQ(npos, pd.m_pending_end);
    ASSERT_EQ(false, pd.isPending());

    size_t pos_start;
    size_t pos_end;
    pd.getPending(pos_start, pos_end);
    ASSERT_EQ(npos, pos_start);
    ASSERT_EQ(npos, pos_end);
    ASSERT_EQ(npos, pd.getPending().first);
    ASSERT_EQ(npos, pd.getPending().second);

    pd.clearPending();
    ASSERT_EQ(false, pd.isPending());
    ASSERT_EQ(npos, pd.m_pending_start);
    ASSERT_EQ(npos, pd.m_pending_end);

    pd.clearPending(0u);
    ASSERT_EQ(false, pd.isPending());
    ASSERT_EQ(npos, pd.m_pending_start);
    ASSERT_EQ(npos, pd.m_pending_end);

    pd.clearPending(10u);
    ASSERT_EQ(true, pd.isPending());
    ASSERT_EQ(0u, pd.m_pending_start);
    ASSERT_EQ(10u, pd.m_pending_end);
    ASSERT_EQ(0u, pd.getPending().first);
    ASSERT_EQ(10u, pd.getPending().second);
}

//--------------------------------------------------------------------------
TEST(TestPending, TestConstructor)
{
    Pending pd(10u);

    ASSERT_EQ(0u, pd.m_pending_start);
    ASSERT_EQ(10u, pd.m_pending_end);
    ASSERT_EQ(true, pd.isPending());

    size_t pos_start;
    size_t pos_end;
    pd.getPending(pos_start, pos_end);
    ASSERT_EQ(0u, pos_start);
    ASSERT_EQ(10u, pos_end);
    ASSERT_EQ(0u, pd.getPending().first);
    ASSERT_EQ(10u, pd.getPending().second);

    pd.clearPending();
    ASSERT_EQ(false, pd.isPending());
    ASSERT_EQ(npos, pd.m_pending_start);
    ASSERT_EQ(npos, pd.m_pending_end);

    pd.clearPending(0u);
    ASSERT_EQ(false, pd.isPending());
    ASSERT_EQ(npos, pd.m_pending_start);
    ASSERT_EQ(npos, pd.m_pending_end);

    pd.clearPending(10u);
    ASSERT_EQ(true, pd.isPending());
    ASSERT_EQ(0u, pd.m_pending_start);
    ASSERT_EQ(10u, pd.m_pending_end);
}

//--------------------------------------------------------------------------
TEST(TestPending, TestPending)
{
    Pending pd;

    pd.setPending(0u);
    ASSERT_EQ(true, pd.isPending());
    ASSERT_EQ(0u, pd.m_pending_start);
    ASSERT_EQ(0u, pd.m_pending_end);
    ASSERT_EQ(0u, pd.getPending().first);
    ASSERT_EQ(0u, pd.getPending().second);

    pd.setPending(1u);
    ASSERT_EQ(true, pd.isPending());
    ASSERT_EQ(0u, pd.m_pending_start);
    ASSERT_EQ(1u, pd.m_pending_end);
    ASSERT_EQ(0u, pd.getPending().first);
    ASSERT_EQ(1u, pd.getPending().second);

    pd.clearPending();
    pd.setPending(3u);
    ASSERT_EQ(true, pd.isPending());
    ASSERT_EQ(3u, pd.m_pending_start);
    ASSERT_EQ(3u, pd.m_pending_end);
    ASSERT_EQ(3u, pd.getPending().first);
    ASSERT_EQ(3u, pd.getPending().second);

    pd.setPending(1u);
    ASSERT_EQ(true, pd.isPending());
    ASSERT_EQ(1u, pd.m_pending_start);
    ASSERT_EQ(3u, pd.m_pending_end);
    ASSERT_EQ(1u, pd.getPending().first);
    ASSERT_EQ(3u, pd.getPending().second);

    pd.setPending(5u);
    ASSERT_EQ(true, pd.isPending());
    ASSERT_EQ(1u, pd.m_pending_start);
    ASSERT_EQ(5u, pd.m_pending_end);
    ASSERT_EQ(1u, pd.getPending().first);
    ASSERT_EQ(5u, pd.getPending().second);

    pd.setPending(0u, 8u);
    ASSERT_EQ(true, pd.isPending());
    ASSERT_EQ(0u, pd.m_pending_start);
    ASSERT_EQ(8u, pd.m_pending_end);
    ASSERT_EQ(0u, pd.getPending().first);
    ASSERT_EQ(8u, pd.getPending().second);
}
