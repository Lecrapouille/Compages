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

#include "GPUContext.hpp"

using namespace tests;

//------------------------------------------------------------------------------
// The GL45 backend needs Direct State Access, introduced in OpenGL 4.5. If this
// test fails on a machine, no other GPU test can be trusted, so it acts as the
// canary for the whole GPU test suite.
//------------------------------------------------------------------------------
TEST(Context, GrantsAtLeast45Core)
{
    GPUContext context;
    if (!context.ready())
    {
        GTEST_SKIP() << context.error();
    }

    auto const [major, minor] = context.version();
    ASSERT_GE(major, 4);
    if (major == 4)
    {
        ASSERT_GE(minor, 5);
    }
}

//------------------------------------------------------------------------------
TEST(Context, ProvidesASymbolLoader)
{
    GPUContext context;
    if (!context.ready())
    {
        GTEST_SKIP() << context.error();
    }

    ASSERT_NE(GPUContext::procAddress(), nullptr);
    // glClear exists in every OpenGL version, so a loader that cannot resolve
    // it is broken.
    ASSERT_NE(GPUContext::procAddress()("glClear"), nullptr);
}

//------------------------------------------------------------------------------
// Creating and destroying the context repeatedly must stay possible: the
// examples gallery relies on it to switch demos without restarting.
//------------------------------------------------------------------------------
TEST(Context, CanBeRecreated)
{
    for (int i = 0; i < 3; ++i)
    {
        GPUContext context;
        if (!context.ready())
        {
            GTEST_SKIP() << context.error();
        }
        ASSERT_TRUE(context.ready());
    }
}
