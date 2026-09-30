//=====================================================================
// Compages: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributedin the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#include "main.hpp"
#define protected public
#define private public
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"


#pragma GCC diagnostic pop
#undef protected
#undef private


template <typename T, size_t r, size_t c>
static void check_matrix(compages::core::Matrix<T, r, c> const& a, compages::core::Matrix<T, r, c> const& b)
{
    for (size_t i = 0_z; i < r * c; ++i)
    {
        ASSERT_EQ(true, std::abs(a.m_data[i] - b.m_data[i]) < 0.0001f);
    }
}

//--------------------------------------------------------------------------
TEST(TestQuaternions, testCreator)
{
    {
        compages::core::Quatf q;

        ASSERT_EQ(1.0f, q.a);
        ASSERT_EQ(0.0f, q.b);
        ASSERT_EQ(0.0f, q.c);
        ASSERT_EQ(0.0f, q.d);

        ASSERT_EQ(1.0f, q[0]);
        ASSERT_EQ(0.0f, q[1]);
        ASSERT_EQ(0.0f, q[2]);
        ASSERT_EQ(0.0f, q[3]);

        ASSERT_EQ(q.data, q.ptr());
        ASSERT_EQ(1.0f, q.ptr()[0]);
        ASSERT_EQ(0.0f, q.ptr()[1]);
        ASSERT_EQ(0.0f, q.ptr()[2]);
        ASSERT_EQ(0.0f, q.ptr()[3]);
    }

    {
        compages::core::Quatf q(1.0f, 0.0f, 0.0f, 0.0f);

        ASSERT_EQ(1.0f, q[0]);
        ASSERT_EQ(0.0f, q[1]);
        ASSERT_EQ(0.0f, q[2]);
        ASSERT_EQ(0.0f, q[3]);
    }

    {
        compages::core::Quatf q(1.0f, compages::core::Vector3f(0.0f, 0.0f, 0.0f));

        ASSERT_EQ(1.0f, q[0]);
        ASSERT_EQ(0.0f, q[1]);
        ASSERT_EQ(0.0f, q[2]);
        ASSERT_EQ(0.0f, q[3]);
    }

    {
        compages::core::Quatf q(0.0f, 0.0f, 0.0f);

        ASSERT_EQ(1.0f, q[0]);
        ASSERT_EQ(0.0f, q[1]);
        ASSERT_EQ(0.0f, q[2]);
        ASSERT_EQ(0.0f, q[3]);
    }

    {
        compages::core::Quatf q(1.0f, 2.0f, 3.0f, 4.0f);
        compages::core::Quatf q1(q);

        ASSERT_EQ(1.0f, q1[0]);
        ASSERT_EQ(2.0f, q1[1]);
        ASSERT_EQ(3.0f, q1[2]);
        ASSERT_EQ(4.0f, q1[3]);
    }

    {
        compages::core::Quatf q(1.0f, 2.0f, 3.0f, 4.0f);
        compages::core::Quatf q1 = q;

        ASSERT_EQ(1.0f, q1[0]);
        ASSERT_EQ(2.0f, q1[1]);
        ASSERT_EQ(3.0f, q1[2]);
        ASSERT_EQ(4.0f, q1[3]);
    }
}

//--------------------------------------------------------------------------
TEST(TestQuaternions, testDisplay)
{
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    std::cout << compages::core::Quatf{} << std::endl;
    ASSERT_STREQ(buffer.str().c_str(), "Quat(1, (0i, 0j, 0k))\n");
    std::cout.rdbuf(old);
}

//--------------------------------------------------------------------------
TEST(TestQuaternions, testangles)
{
    {
        compages::core::Quatf q;

        compages::core::Vector3f v = q.axis();
        ASSERT_EQ(0.0f, v.x);
        ASSERT_EQ(0.0f, v.y);
        ASSERT_EQ(1.0f, v.z);
        ASSERT_EQ(units::angle::degree_t(0.0f).to<int>(), q.angle().to<int>());
    }

    {
        compages::core::Quatf q = compages::core::Quatf::fromAngleAxis(units::angle::degree_t(0.0f),
                                       compages::core::Vector3f(0, 0, 1));
        ASSERT_EQ(1.0f, q[0]);
        ASSERT_EQ(0.0f, q[1]);
        ASSERT_EQ(0.0f, q[2]);
        ASSERT_EQ(0.0f, q[3]);
    }

    {
        compages::core::Quatf q(units::angle::degree_t(0.0f), compages::core::Vector3f(0, 0, 1));
        ASSERT_EQ(1.0f, q[0]);
        ASSERT_EQ(0.0f, q[1]);
        ASSERT_EQ(0.0f, q[2]);
        ASSERT_EQ(0.0f, q[3]);
    }
}

//--------------------------------------------------------------------------
TEST(TestQuaternions, testmatrix)
{
    {
        compages::core::Quatf q;
        compages::core::Matrix44f m = q.toMatrix();
        check_matrix(compages::core::Matrix44f(compages::core::matrix::Identity), m);
    }

    {
        compages::core::Quatf q = compages::core::Quatf::fromAngleAxis(units::angle::degree_t(45.0f),
                                       compages::core::Vector3f(0, 1, 0));
        compages::core::Matrix44f m = q.toMatrix();
        check_matrix(compages::core::Matrix44f({ 0.707107f,
                                 0.0f,
                                 0.707107f,
                                 0.0f,
                                 0.0f,
                                 1.0f,
                                 0.0f,
                                 0.0f,
                                 -0.707107f,
                                 0.0f,
                                 0.707107f,
                                 0.0f,
                                 0.0f,
                                 0.0f,
                                 0.0f,
                                 1.0f }),
                     m);
    }

    // toMatrix(), rotate() and fromMatrix() shall agree on the convention.
    {
        const units::angle::degree_t angle(30.0f);
        const compages::core::Vector3f axis(0.371391f, 0.557086f, 0.742781f);
        compages::core::Quatf q = compages::core::Quatf::fromAngleAxis(angle, axis);
        compages::core::Matrix44f m = q.toMatrix();
        check_matrix(compages::core::rotate(compages::core::Matrix44f(compages::core::matrix::Identity),
                                            angle,
                                            axis),
                     m);

        compages::core::Quatf back = compages::core::Quatf::fromMatrix(m);
        ASSERT_NEAR(q[0], back[0], 0.0001f);
        ASSERT_NEAR(q[1], back[1], 0.0001f);
        ASSERT_NEAR(q[2], back[2], 0.0001f);
        ASSERT_NEAR(q[3], back[3], 0.0001f);
    }
}

//--------------------------------------------------------------------------
TEST(TestQuaternions, testoperators)
{
    {
        compages::core::Quatf q1(1.0f, 2.0f, 3.0f, 4.0f);
        compages::core::Quatf q2(5.0f, 6.0f, 7.0f, 8.0f);
        compages::core::Quatf q = q1 + q2;

        ASSERT_EQ(6.0f, q[0]);
        ASSERT_EQ(8.0f, q[1]);
        ASSERT_EQ(10.0f, q[2]);
        ASSERT_EQ(12.0f, q[3]);
    }

    {
        compages::core::Quatf q(1.0f, 2.0f, 3.0f, 4.0f);
        float k = 3.0f;
        compages::core::Quatf q1 = q * k;
        compages::core::Quatf q2 = k * q;

        ASSERT_EQ(3.0f, q1[0]);
        ASSERT_EQ(6.0f, q1[1]);
        ASSERT_EQ(9.0f, q1[2]);
        ASSERT_EQ(12.0f, q1[3]);

        ASSERT_EQ(3.0f, q2[0]);
        ASSERT_EQ(6.0f, q2[1]);
        ASSERT_EQ(9.0f, q2[2]);
        ASSERT_EQ(12.0f, q2[3]);

        compages::core::Quatf q3 = q / k;
        ASSERT_EQ(1.0f / k, q3[0]);
        ASSERT_EQ(2.0f / k, q3[1]);
        ASSERT_EQ(3.0f / k, q3[2]);
        ASSERT_EQ(4.0f / k, q3[3]);
    }

    {
        compages::core::Quatf q(1.0f, 2.0f, 3.0f, 4.0f);
        q *= 3.0f;

        ASSERT_EQ(3.0f, q[0]);
        ASSERT_EQ(6.0f, q[1]);
        ASSERT_EQ(9.0f, q[2]);
        ASSERT_EQ(12.0f, q[3]);

        q /= 3.0f;

        ASSERT_EQ(1.0f, q[0]);
        ASSERT_EQ(2.0f, q[1]);
        ASSERT_EQ(3.0f, q[2]);
        ASSERT_EQ(4.0f, q[3]);
    }

    {
        compages::core::Quatf q(1.0f, 2.0f, 3.0f, 4.0f);
        compages::core::Quatf q1(5.0f, 6.0f, 7.0f, 8.0f);
        q += q1;

        ASSERT_EQ(6.0f, q[0]);
        ASSERT_EQ(8.0f, q[1]);
        ASSERT_EQ(10.0f, q[2]);
        ASSERT_EQ(12.0f, q[3]);

        q -= q1;

        ASSERT_EQ(1.0f, q[0]);
        ASSERT_EQ(2.0f, q[1]);
        ASSERT_EQ(3.0f, q[2]);
        ASSERT_EQ(4.0f, q[3]);
    }
}

//--------------------------------------------------------------------------
TEST(TestQuaternions, testoperations)
{
    {
        compages::core::Quatf q;

        ASSERT_EQ(1.0f, q.norm());
        ASSERT_EQ(1.0f, q[0]);
        ASSERT_EQ(0.0f, q[1]);
        ASSERT_EQ(0.0f, q[2]);
        ASSERT_EQ(0.0f, q[3]);
    }

    {
        compages::core::Quatf q(1.0f, 2.0f, 3.0f, 4.0f);
        compages::core::Quatf q1 = q.conjugate();
        ASSERT_EQ(1.0f, q1[0]);
        ASSERT_EQ(-2.0f, q1[1]);
        ASSERT_EQ(-3.0f, q1[2]);
        ASSERT_EQ(-4.0f, q1[3]);
    }

    {
        compages::core::Quatf q(1.0f, 2.0f, 3.0f, 4.0f);
        compages::core::Quatf q1 = -q;
        ASSERT_EQ(-1.0f, q1[0]);
        ASSERT_EQ(-2.0f, q1[1]);
        ASSERT_EQ(-3.0f, q1[2]);
        ASSERT_EQ(-4.0f, q1[3]);
    }
}
