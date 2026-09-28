// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"
#define protected public
#define private public
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include "Compages/Core/Quaternion.hpp"
#pragma GCC diagnostic pop
#undef protected
#undef private

template <typename T, size_t r, size_t c>
static void check_matrix(Matrix<T, r, c> const& a, Matrix<T, r, c> const& b)
{
    for (size_t i = 0_z; i < r * c; ++i)
    {
        ASSERT_EQ(true, std::abs(a.m_data[i] - b.m_data[i]) < 0.0001f);
    }
}

TEST(TestQuaternions, testCreator)
{
    {
        Quatf q;

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
        Quatf q(1.0f, 0.0f, 0.0f, 0.0f);

        ASSERT_EQ(1.0f, q[0]);
        ASSERT_EQ(0.0f, q[1]);
        ASSERT_EQ(0.0f, q[2]);
        ASSERT_EQ(0.0f, q[3]);
    }

    {
        Quatf q(1.0f, Vector3f(0.0f, 0.0f, 0.0f));

        ASSERT_EQ(1.0f, q[0]);
        ASSERT_EQ(0.0f, q[1]);
        ASSERT_EQ(0.0f, q[2]);
        ASSERT_EQ(0.0f, q[3]);
    }

    {
        Quatf q(0.0f, 0.0f, 0.0f);

        ASSERT_EQ(1.0f, q[0]);
        ASSERT_EQ(0.0f, q[1]);
        ASSERT_EQ(0.0f, q[2]);
        ASSERT_EQ(0.0f, q[3]);
    }

    {
        Quatf q(1.0f, 2.0f, 3.0f, 4.0f);
        Quatf q1(q);

        ASSERT_EQ(1.0f, q1[0]);
        ASSERT_EQ(2.0f, q1[1]);
        ASSERT_EQ(3.0f, q1[2]);
        ASSERT_EQ(4.0f, q1[3]);
    }

    {
        Quatf q(1.0f, 2.0f, 3.0f, 4.0f);
        Quatf q1 = q;

        ASSERT_EQ(1.0f, q1[0]);
        ASSERT_EQ(2.0f, q1[1]);
        ASSERT_EQ(3.0f, q1[2]);
        ASSERT_EQ(4.0f, q1[3]);
    }
}

TEST(TestQuaternions, testDisplay)
{
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    std::cout << Quatf{} << std::endl;
    ASSERT_STREQ(buffer.str().c_str(), "Quat(1, (0i, 0j, 0k))\n");
    std::cout.rdbuf(old);
}

TEST(TestQuaternions, testangles)
{
    {
        Quatf q;

        Vector3f v = q.axis();
        ASSERT_EQ(0.0f, v.x);
        ASSERT_EQ(0.0f, v.y);
        ASSERT_EQ(1.0f, v.z);
        ASSERT_EQ(units::angle::degree_t(0.0f).to<int>(), q.angle().to<int>());
    }

    {
        Quatf q = Quatf::fromAngleAxis(units::angle::degree_t(0.0f),
                                       Vector3f(0, 0, 1));
        ASSERT_EQ(1.0f, q[0]);
        ASSERT_EQ(0.0f, q[1]);
        ASSERT_EQ(0.0f, q[2]);
        ASSERT_EQ(0.0f, q[3]);
    }

    {
        Quatf q(units::angle::degree_t(0.0f), Vector3f(0, 0, 1));
        ASSERT_EQ(1.0f, q[0]);
        ASSERT_EQ(0.0f, q[1]);
        ASSERT_EQ(0.0f, q[2]);
        ASSERT_EQ(0.0f, q[3]);
    }
}

TEST(TestQuaternions, testmatrix)
{
    {
        Quatf q;
        Matrix44f m = q.toMatrix();
        check_matrix(Matrix44f(compages::matrix::Identity), m);
    }

    {
        Quatf q = Quatf::fromAngleAxis(units::angle::degree_t(45.0f),
                                       Vector3f(0, 1, 0));
        Matrix44f m = q.toMatrix();
        check_matrix(Matrix44f({ 0.707107f,
                                 0.0f,
                                 -0.707107f,
                                 0.0f,
                                 0.0f,
                                 1.0f,
                                 0.0f,
                                 0.0f,
                                 0.707107f,
                                 0.0f,
                                 0.707107f,
                                 0.0f,
                                 0.0f,
                                 0.0f,
                                 0.0f,
                                 1.0f }),
                     m);
    }
}

TEST(TestQuaternions, testoperators)
{
    {
        Quatf q1(1.0f, 2.0f, 3.0f, 4.0f);
        Quatf q2(5.0f, 6.0f, 7.0f, 8.0f);
        Quatf q = q1 + q2;

        ASSERT_EQ(6.0f, q[0]);
        ASSERT_EQ(8.0f, q[1]);
        ASSERT_EQ(10.0f, q[2]);
        ASSERT_EQ(12.0f, q[3]);
    }

    {
        Quatf q(1.0f, 2.0f, 3.0f, 4.0f);
        float k = 3.0f;
        Quatf q1 = q * k;
        Quatf q2 = k * q;

        ASSERT_EQ(3.0f, q1[0]);
        ASSERT_EQ(6.0f, q1[1]);
        ASSERT_EQ(9.0f, q1[2]);
        ASSERT_EQ(12.0f, q1[3]);

        ASSERT_EQ(3.0f, q2[0]);
        ASSERT_EQ(6.0f, q2[1]);
        ASSERT_EQ(9.0f, q2[2]);
        ASSERT_EQ(12.0f, q2[3]);

        Quatf q3 = q / k;
        ASSERT_EQ(1.0f / k, q3[0]);
        ASSERT_EQ(2.0f / k, q3[1]);
        ASSERT_EQ(3.0f / k, q3[2]);
        ASSERT_EQ(4.0f / k, q3[3]);
    }

    {
        Quatf q(1.0f, 2.0f, 3.0f, 4.0f);
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
        Quatf q(1.0f, 2.0f, 3.0f, 4.0f);
        Quatf q1(5.0f, 6.0f, 7.0f, 8.0f);
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

TEST(TestQuaternions, testoperations)
{
    {
        Quatf q;

        ASSERT_EQ(1.0f, q.norm());
        ASSERT_EQ(1.0f, q[0]);
        ASSERT_EQ(0.0f, q[1]);
        ASSERT_EQ(0.0f, q[2]);
        ASSERT_EQ(0.0f, q[3]);
    }

    {
        Quatf q(1.0f, 2.0f, 3.0f, 4.0f);
        Quatf q1 = q.conjugate();
        ASSERT_EQ(1.0f, q1[0]);
        ASSERT_EQ(-2.0f, q1[1]);
        ASSERT_EQ(-3.0f, q1[2]);
        ASSERT_EQ(-4.0f, q1[3]);
    }

    {
        Quatf q(1.0f, 2.0f, 3.0f, 4.0f);
        Quatf q1 = -q;
        ASSERT_EQ(-1.0f, q1[0]);
        ASSERT_EQ(-2.0f, q1[1]);
        ASSERT_EQ(-3.0f, q1[2]);
        ASSERT_EQ(-4.0f, q1[3]);
    }
}
