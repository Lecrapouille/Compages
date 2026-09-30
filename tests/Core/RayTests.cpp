// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"

#include "Compages/Core/Ray.hpp"



TEST(Ray, HitsABoxInFront)
{
    const compages::core::Ray ray = compages::core::Ray::fromPoints(compages::core::Vector3f(0.0f, 0.0f, 10.0f),
                                    compages::core::Vector3f(0.0f, 0.0f, 0.0f));
    const compages::core::AABB box = compages::core::AABB::fromCenterExtent(compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                                            compages::core::Vector3f(1.0f, 1.0f, 1.0f));
    const auto t = intersect(ray, box);
    ASSERT_TRUE(t.has_value());
    ASSERT_NEAR(*t, 9.0f, 1.0e-4f);
}

TEST(Ray, MissesABoxBesideIt)
{
    const compages::core::Ray ray = compages::core::Ray::fromPoints(compages::core::Vector3f(0.0f, 0.0f, 10.0f),
                                    compages::core::Vector3f(0.0f, 0.0f, 0.0f));
    const compages::core::AABB box = compages::core::AABB::fromCenterExtent(compages::core::Vector3f(20.0f, 0.0f, 0.0f),
                                            compages::core::Vector3f(1.0f, 1.0f, 1.0f));
    ASSERT_FALSE(intersect(ray, box).has_value());
}

TEST(Ray, ReportsZeroWhenItStartsInside)
{
    const compages::core::Ray ray{ compages::core::Vector3f(0.0f, 0.0f, 0.0f), compages::core::Vector3f(0.0f, 0.0f, -1.0f) };
    const compages::core::AABB box = compages::core::AABB::fromCenterExtent(compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                                            compages::core::Vector3f(2.0f, 2.0f, 2.0f));
    const auto t = intersect(ray, box);
    ASSERT_TRUE(t.has_value());
    ASSERT_NEAR(*t, 0.0f, 1.0e-5f);
}

TEST(Ray, IgnoresAnEmptyBox)
{
    const compages::core::Ray ray{ compages::core::Vector3f(0.0f, 0.0f, 0.0f), compages::core::Vector3f(0.0f, 0.0f, -1.0f) };
    ASSERT_FALSE(intersect(ray, compages::core::AABB{}).has_value());
}
