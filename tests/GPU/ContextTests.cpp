// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPUContext.hpp"

using namespace tests;

// The GL45 backend needs Direct State Access, introduced in OpenGL 4.5. If this
// test fails on a machine, no other GPU test can be trusted, so it acts as the
// canary for the whole GPU test suite.
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

// Creating and destroying the context repeatedly must stay possible: the
// examples gallery relies on it to switch demos without restarting.
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
