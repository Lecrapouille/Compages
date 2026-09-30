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

#include "Compages/Core/Ray.hpp"



//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
TEST(Ray, MissesABoxBesideIt)
{
    const compages::core::Ray ray = compages::core::Ray::fromPoints(compages::core::Vector3f(0.0f, 0.0f, 10.0f),
                                    compages::core::Vector3f(0.0f, 0.0f, 0.0f));
    const compages::core::AABB box = compages::core::AABB::fromCenterExtent(compages::core::Vector3f(20.0f, 0.0f, 0.0f),
                                            compages::core::Vector3f(1.0f, 1.0f, 1.0f));
    ASSERT_FALSE(intersect(ray, box).has_value());
}

//------------------------------------------------------------------------------
TEST(Ray, ReportsZeroWhenItStartsInside)
{
    const compages::core::Ray ray{ compages::core::Vector3f(0.0f, 0.0f, 0.0f), compages::core::Vector3f(0.0f, 0.0f, -1.0f) };
    const compages::core::AABB box = compages::core::AABB::fromCenterExtent(compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                                            compages::core::Vector3f(2.0f, 2.0f, 2.0f));
    const auto t = intersect(ray, box);
    ASSERT_TRUE(t.has_value());
    ASSERT_NEAR(*t, 0.0f, 1.0e-5f);
}

//------------------------------------------------------------------------------
TEST(Ray, IgnoresAnEmptyBox)
{
    const compages::core::Ray ray{ compages::core::Vector3f(0.0f, 0.0f, 0.0f), compages::core::Vector3f(0.0f, 0.0f, -1.0f) };
    ASSERT_FALSE(intersect(ray, compages::core::AABB{}).has_value());
}
