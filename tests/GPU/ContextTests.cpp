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
TEST(Context, GrantsTheVersionTheBackendNeeds)
{
    GPUContext context;
    if (!context.ready())
    {
        GTEST_SKIP() << context.error();
    }

    // 4.5 core for GL45, 4.1 core for GL41 (macOS), ES 3.0 for GLES3.
#if defined(COMPAGES_GPU_BACKEND_GLES3)
    constexpr int needed[2] = { 3, 0 };
#elif defined(COMPAGES_GPU_BACKEND_GL41)
    constexpr int needed[2] = { 4, 1 };
#else
    constexpr int needed[2] = { 4, 5 };
#endif
    auto const [major, minor] = context.version();
    ASSERT_GE(major, needed[0]);
    if (major == needed[0])
    {
        ASSERT_GE(minor, needed[1]);
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
