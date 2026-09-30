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
#include "Compages/Core/Vector.hpp"


#pragma GCC diagnostic pop
#undef protected
#undef private


//--------------------------------------------------------------------------
#define ASSERT_DOUBLES_EQUAL(a, b, c) ASSERT_EQ(true, compages::core::abs(a - b) < c)

//--------------------------------------------------------------------------
// Check union [0,1,2,3] and [x,y,z,w] and [r,g,b,a]. The parameters cannot be
// named a, b, c, d: the colour aliases of the union are themselves named b and
// a, and the preprocessor would substitute them.
#define ASSERT_NEAR_VECTOR4(vect, e0, e1, e2, e3, thresh) \
    ASSERT_NEAR(vect[0], e0, thresh);                     \
    ASSERT_NEAR(vect[1], e1, thresh);                     \
    ASSERT_NEAR(vect[2], e2, thresh);                     \
    ASSERT_NEAR(vect[3], e3, thresh);                     \
    ASSERT_NEAR(vect.x, e0, thresh);                      \
    ASSERT_NEAR(vect.y, e1, thresh);                      \
    ASSERT_NEAR(vect.z, e2, thresh);                      \
    ASSERT_NEAR(vect.w, e3, thresh);                      \
    ASSERT_NEAR(vect.r, e0, thresh);                      \
    ASSERT_NEAR(vect.g, e1, thresh);                      \
    ASSERT_NEAR(vect.b, e2, thresh);                      \
    ASSERT_NEAR(vect.a, e3, thresh)

//--------------------------------------------------------------------------
#define ASSERT_VECTOR4_NAN(vect)          \
    ASSERT_EQ(true, std::isnan(vect[0])); \
    ASSERT_EQ(true, std::isnan(vect[1])); \
    ASSERT_EQ(true, std::isnan(vect[2])); \
    ASSERT_EQ(true, std::isnan(vect[3])); \
    ASSERT_EQ(true, std::isnan(vect.x));  \
    ASSERT_EQ(true, std::isnan(vect.y));  \
    ASSERT_EQ(true, std::isnan(vect.z));  \
    ASSERT_EQ(true, std::isnan(vect.w));  \
    ASSERT_EQ(true, std::isnan(vect.r));  \
    ASSERT_EQ(true, std::isnan(vect.g));  \
    ASSERT_EQ(true, std::isnan(vect.b)); \
    ASSERT_EQ(true, std::isnan(vect.a))

//--------------------------------------------------------------------------
// Check union [0,1,2] and [x,y,z] and [r,g,b]
#define ASSERT_NEAR_VECTOR3(vect, e0, e1, e2, thresh) \
    ASSERT_NEAR(vect[0], e0, thresh);                 \
    ASSERT_NEAR(vect[1], e1, thresh);                 \
    ASSERT_NEAR(vect[2], e2, thresh);                 \
    ASSERT_NEAR(vect.x, e0, thresh);                  \
    ASSERT_NEAR(vect.y, e1, thresh);                  \
    ASSERT_NEAR(vect.z, e2, thresh);                  \
    ASSERT_NEAR(vect.r, e0, thresh);                  \
    ASSERT_NEAR(vect.g, e1, thresh);                  \
    ASSERT_NEAR(vect.b, e2, thresh)

//--------------------------------------------------------------------------
#define ASSERT_THAT_VECTOR3(vect, e0, e1, e2) \
    ASSERT_THAT(vect[0], e0);                 \
    ASSERT_THAT(vect[1], e1);                 \
    ASSERT_THAT(vect[2], e2);                 \
    ASSERT_THAT(vect.x, e0);                  \
    ASSERT_THAT(vect.y, e1);                  \
    ASSERT_THAT(vect.z, e2);                  \
    ASSERT_THAT(vect.r, e0);                  \
    ASSERT_THAT(vect.g, e1);                  \
    ASSERT_THAT(vect.b, e2)

//--------------------------------------------------------------------------
#define ASSERT_VECTOR3_NAN(vect)          \
    ASSERT_EQ(true, std::isnan(vect[0])); \
    ASSERT_EQ(true, std::isnan(vect[1])); \
    ASSERT_EQ(true, std::isnan(vect[2])); \
    ASSERT_EQ(true, std::isnan(vect.x));  \
    ASSERT_EQ(true, std::isnan(vect.y));  \
    ASSERT_EQ(true, std::isnan(vect.z));  \
    ASSERT_EQ(true, std::isnan(vect.r));  \
    ASSERT_EQ(true, std::isnan(vect.g));  \
    ASSERT_EQ(true, std::isnan(vect.b))

//--------------------------------------------------------------------------
#define ASSERT_NEAR_VECTOR3_INF(vect)     \
    ASSERT_EQ(true, std::isinf(vect[0])); \
    ASSERT_EQ(true, std::isinf(vect[1])); \
    ASSERT_EQ(true, std::isinf(vect[2])); \
    ASSERT_EQ(true, std::isinf(vect.x));  \
    ASSERT_EQ(true, std::isinf(vect.y));  \
    ASSERT_EQ(true, std::isinf(vect.z));  \
    ASSERT_EQ(true, std::isinf(vect.r));  \
    ASSERT_EQ(true, std::isinf(vect.g));  \
    ASSERT_EQ(true, std::isinf(vect.b))

//--------------------------------------------------------------------------
// Check union [0,1] and [x,y] and [u,v]
#define ASSERT_NEAR_VECTOR2(vect, a, b, thresh) \
    ASSERT_NEAR(vect[0], a, thresh);            \
    ASSERT_NEAR(vect[1], b, thresh);            \
    ASSERT_NEAR(vect.x, a, thresh);             \
    ASSERT_NEAR(vect.y, b, thresh);             \
    ASSERT_NEAR(vect.u, a, thresh);             \
    ASSERT_NEAR(vect.v, b, thresh)

//--------------------------------------------------------------------------
#define ASSERT_THAT_VECTOR2(vect, a, b) \
    ASSERT_THAT(vect[0], a);            \
    ASSERT_THAT(vect[1], b);            \
    ASSERT_THAT(vect.x, a);             \
    ASSERT_THAT(vect.y, b);             \
    ASSERT_THAT(vect.u, a);             \
    ASSERT_THAT(vect.v, b)

//--------------------------------------------------------------------------
#define ASSERT_VECTOR2_NAN(vect)          \
    ASSERT_EQ(true, std::isnan(vect[0])); \
    ASSERT_EQ(true, std::isnan(vect[1])); \
    ASSERT_EQ(true, std::isnan(vect.x));  \
    ASSERT_EQ(true, std::isnan(vect.y));  \
    ASSERT_EQ(true, std::isnan(vect.u));  \
    ASSERT_EQ(true, std::isnan(vect.v))

//--------------------------------------------------------------------------
TEST(TestVectors, testSizeof)
{
    ASSERT_EQ(2u * sizeof(float), sizeof(compages::core::Vector2f));
    ASSERT_EQ(2u * sizeof(double), sizeof(compages::core::Vector2g));
    ASSERT_EQ(2u * sizeof(int), sizeof(compages::core::Vector2i));

    ASSERT_EQ(3u * sizeof(float), sizeof(compages::core::Vector3f));
    ASSERT_EQ(3u * sizeof(double), sizeof(compages::core::Vector3g));
    ASSERT_EQ(3u * sizeof(int), sizeof(compages::core::Vector3i));

    ASSERT_EQ(4u * sizeof(float), sizeof(compages::core::Vector4f));
    ASSERT_EQ(4u * sizeof(double), sizeof(compages::core::Vector4g));
    ASSERT_EQ(4u * sizeof(int), sizeof(compages::core::Vector4i));
}

//--------------------------------------------------------------------------
TEST(TestVectors, testConstructorVec4)
{
    compages::core::Vector4f v1;
    compages::core::Vector4f v2(1.0f, 2.0f, 3.0f, 4.0f);
    compages::core::Vector4f v3(compages::core::Vector3f(1.0f, 2.0f, 3.0f));
    compages::core::Vector4f v4 = { 4.0f, 5.0f, 6.0f, 7.0f };
    compages::core::Vector4f v5(1, 2, 3, 4);
    compages::core::Vector4f v6 = { -4, 5, -6 };
    compages::core::Vector4f v7(compages::core::Vector2f(1, 2));
    compages::core::Vector4f v8(42);
    compages::core::Vector4f dummy = compages::core::Vector4f::DUMMY;

    // Check size
    ASSERT_EQ(4_z, v1.size());
    ASSERT_EQ(4_z, v2.size());
    ASSERT_EQ(4_z, v3.size());
    ASSERT_EQ(4_z, v4.size());
    ASSERT_EQ(4_z, v5.size());
    ASSERT_EQ(4_z, v6.size());
    ASSERT_EQ(4_z, v7.size());
    ASSERT_EQ(4_z, v8.size());
    ASSERT_EQ(4_z, dummy.size());

    // Check values passed to the constructor
    ASSERT_NEAR_VECTOR4(v2, 1.0f, 2.0f, 3.0f, 4.0f, 0.001f);
    ASSERT_NEAR_VECTOR4(v3, 1.0f, 2.0f, 3.0f, 0.0f, 0.001f);
    ASSERT_NEAR_VECTOR4(v4, 4.0f, 5.0f, 6.0f, 7.0f, 0.001f);
    ASSERT_NEAR_VECTOR4(v5, 1.0f, 2.0f, 3.0f, 4.0f, 0.001f);
    ASSERT_NEAR_VECTOR4(v6, -4.0f, 5.0f, -6.0f, 0.0f, 0.001f);
    ASSERT_NEAR_VECTOR4(v7, 1.0f, 2.0f, 0.0f, 0.0f, 0.001f);
    ASSERT_NEAR_VECTOR4(v8, 42.0f, 42.0f, 42.0f, 42.0f, 0.001f);
    ASSERT_VECTOR4_NAN(dummy);
}

//--------------------------------------------------------------------------
TEST(TestVectors, testConstructorVec3)
{
    compages::core::Vector3f v1;
    compages::core::Vector3f v2(1.0f, 2.0f, 3.0f);
    compages::core::Vector3f v3(1.0f, 2.0f);
    compages::core::Vector3f v4 = { 4.0f, 5.0f, 6.0f };
    compages::core::Vector3f v5(1, 2, 3);
    compages::core::Vector3f v6 = { -4, 5, -6 };
    compages::core::Vector3f v7(compages::core::Vector2f(1, 2));
    compages::core::Vector3f v8(42);
    compages::core::Vector3f dummy = compages::core::Vector3f::DUMMY;

    // Check size
    ASSERT_EQ(3_z, v1.size());
    ASSERT_EQ(3_z, v2.size());
    ASSERT_EQ(3_z, v3.size());
    ASSERT_EQ(3_z, v4.size());
    ASSERT_EQ(3_z, v5.size());
    ASSERT_EQ(3_z, v6.size());
    ASSERT_EQ(3_z, v7.size());
    ASSERT_EQ(3_z, v8.size());
    ASSERT_EQ(3_z, dummy.size());

    // Check values passed to the constructor
    ASSERT_NEAR_VECTOR3(v2, 1.0f, 2.0f, 3.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(v3, 1.0f, 2.0f, 0.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(v4, 4.0f, 5.0f, 6.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(v5, 1.0f, 2.0f, 3.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(v6, -4.0f, 5.0f, -6.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(v7, 1.0f, 2.0f, 0.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(v8, 42.0f, 42.0f, 42.0f, 0.001f);
    ASSERT_VECTOR3_NAN(dummy);
}

//--------------------------------------------------------------------------
TEST(TestVectors, testConstructorVec2)
{
    compages::core::Vector2f v1;
    compages::core::Vector2f v2(1.0f, 2.0f);
    compages::core::Vector2f v3(1.0f);
    compages::core::Vector2f v4 = { 4.0f, 5.0f, 6.0f };
    compages::core::Vector2f v5(1, 2);
    compages::core::Vector2f v6 = { -4, 5, -6 };
    compages::core::Vector2f v7(compages::core::Vector2f(1, 2));
    compages::core::Vector2f v8(42);
    compages::core::Vector2f dummy = compages::core::Vector2f::DUMMY;

    // Check size
    ASSERT_EQ(2_z, v1.size());
    ASSERT_EQ(2_z, v2.size());
    ASSERT_EQ(2_z, v3.size());
    ASSERT_EQ(2_z, v4.size());
    ASSERT_EQ(2_z, v5.size());
    ASSERT_EQ(2_z, v6.size());
    ASSERT_EQ(2_z, v7.size());
    ASSERT_EQ(2_z, v8.size());
    ASSERT_EQ(2_z, dummy.size());

    // Check values passed to the constructor
    ASSERT_NEAR_VECTOR2(v2, 1.0f, 2.0f, 0.001f);
    ASSERT_NEAR_VECTOR2(v3, 1.0f, 1.0f, 0.001f);
    ASSERT_NEAR_VECTOR2(v4, 4.0f, 5.0f, 0.001f);
    ASSERT_NEAR_VECTOR2(v5, 1.0f, 2.0f, 0.001f);
    ASSERT_NEAR_VECTOR2(v6, -4.0f, 5.0f, 0.001f);
    ASSERT_NEAR_VECTOR2(v7, 1.0f, 2.0f, 0.001f);
    ASSERT_NEAR_VECTOR2(v8, 42.0f, 42.0f, 0.001f);
    ASSERT_VECTOR2_NAN(dummy);
}

//--------------------------------------------------------------------------
TEST(TestVectors, testPredefined)
{
    ASSERT_VECTOR3_NAN(compages::core::Vector3f::DUMMY);
    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::POSITIVE_INFINITY,
                        compages::core::max<float>(),
                        compages::core::max<float>(),
                        compages::core::max<float>(),
                        0.001f);
    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::NEGATIVE_INFINITY,
                        -compages::core::max<float>(),
                        -compages::core::max<float>(),
                        -compages::core::max<float>(),
                        0.001f);

    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::ZERO, 0.0f, 0.0f, 0.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::ONE, 1.0f, 1.0f, 1.0f, 0.001f);

    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::UNIT_SCALE, 1.0f, 1.0f, 1.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(
        compages::core::Vector3f::NEGATIVE_UNIT_SCALE, -1.0f, -1.0f, -1.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::UNIT_X, 1.0f, 0.0f, 0.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::UNIT_Y, 0.0f, 1.0f, 0.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::UNIT_Z, 0.0f, 0.0f, 1.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::NEGATIVE_UNIT_X, -1.0f, 0.0f, 0.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::NEGATIVE_UNIT_Y, 0.0f, -1.0f, 0.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::NEGATIVE_UNIT_Z, 0.0f, 0.0f, -1.0f, 0.001f);

    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::LEFT, -1.0f, 0.0f, 0.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::RIGHT, 1.0f, 0.0f, 0.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::BACK, 0.0f, 0.0f, -1.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::FORWARD, 0.0f, 0.0f, 1.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::DOWN, 0.0f, -1.0f, 0.0f, 0.001f);
    ASSERT_NEAR_VECTOR3(compages::core::Vector3f::UP, 0.0f, 1.0f, 0.0f, 0.001f);
}

//--------------------------------------------------------------------------
TEST(TestVectors, testPrint)
{
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    std::cout << compages::core::Vector4f::ZERO << std::endl;
    std::cout.rdbuf(old);

    ASSERT_THAT(buffer.str().c_str(), HasSubstr("[0, 0, 0, 0]"));
}

//--------------------------------------------------------------------------
TEST(TestVectors, testSwap)
{
    compages::core::Vector3f v2(1.0f, 2.0f, 3.0f);
    compages::core::Vector3f v4 = { 4.0f, 5.0f, 6.0f };

    compages::core::vector::swap(v2, v4);
    ASSERT_NEAR_VECTOR3(v2, 4.0f, 5.0f, 6.0f, 0.001f);
    compages::core::vector::swap(v2, v4);
    ASSERT_NEAR_VECTOR3(v2, 1.0f, 2.0f, 3.0f, 0.001f);
    compages::core::vector::swap(v4, v2);
    ASSERT_NEAR_VECTOR3(v2, 4.0f, 5.0f, 6.0f, 0.001f);
    compages::core::vector::swap(v4, v2);
    ASSERT_NEAR_VECTOR3(v2, 1.0f, 2.0f, 3.0f, 0.001f);
}

//--------------------------------------------------------------------------
TEST(TestVectors, testComparaisons)
{
    compages::core::Vector3f one(1.0f);
    compages::core::Vector3f two(2.0f);

    // Operator <
    {
        compages::core::Vector3b A = (one < two);
        compages::core::Vector3b B = (two < one);
        compages::core::Vector3b C = (one < one);

        ASSERT_THAT_VECTOR3(A, true, true, true);
        ASSERT_THAT_VECTOR3(B, false, false, false);
        ASSERT_THAT_VECTOR3(C, false, false, false);
    }

    // Operator >
    {
        compages::core::Vector3b A = (one > two);
        compages::core::Vector3b B = (two > one);
        compages::core::Vector3b C = (one > one);

        ASSERT_THAT_VECTOR3(A, false, false, false);
        ASSERT_THAT_VECTOR3(B, true, true, true);
        ASSERT_THAT_VECTOR3(C, false, false, false);
    }

    // Operator <=
    {
        compages::core::Vector3b A = (one <= two);
        compages::core::Vector3b B = (two <= one);
        compages::core::Vector3b C = (one <= one);

        ASSERT_THAT_VECTOR3(A, true, true, true);
        ASSERT_THAT_VECTOR3(B, false, false, false);
        ASSERT_THAT_VECTOR3(C, true, true, true);
    }

    // Operator >=
    {
        compages::core::Vector3b A = (one >= two);
        compages::core::Vector3b B = (two >= one);
        compages::core::Vector3b C = (one >= one);

        ASSERT_THAT_VECTOR3(A, false, false, false);
        ASSERT_THAT_VECTOR3(B, true, true, true);
        ASSERT_THAT_VECTOR3(C, true, true, true);
    }

    // Operator ==
    {
        compages::core::Vector3b A = (one == two);
        compages::core::Vector3b B = (two == one);
        compages::core::Vector3b C = (one == one);

        ASSERT_THAT_VECTOR3(A, false, false, false);
        ASSERT_THAT_VECTOR3(B, false, false, false);
        ASSERT_THAT_VECTOR3(C, true, true, true);
    }

    // Operator !=
    {
        compages::core::Vector3b A = (one != two);
        compages::core::Vector3b B = (two != one);
        compages::core::Vector3b C = (one != one);

        ASSERT_THAT_VECTOR3(A, true, true, true);
        ASSERT_THAT_VECTOR3(B, true, true, true);
        ASSERT_THAT_VECTOR3(C, false, false, false);
    }

    // Operator !
    {
        compages::core::Vector3b A = (one != two);
        compages::core::Vector3b B = !A;
        compages::core::Vector3b C = !B;

        ASSERT_THAT_VECTOR3(A, true, true, true);
        ASSERT_THAT_VECTOR3(B, false, false, false);
        ASSERT_THAT_VECTOR3(C, true, true, true);
    }

    // Operator &
    {
        compages::core::Vector3b A(true);
        compages::core::Vector3b B(false);
        compages::core::Vector3b C = A & B;
        compages::core::Vector3b D = A | B;
        compages::core::Vector3b E = A ^ true;
        compages::core::Vector3b F = true ^ B;

        ASSERT_THAT_VECTOR3(A, true, true, true);
        ASSERT_THAT_VECTOR3(B, false, false, false);
        ASSERT_THAT_VECTOR3(C, false, false, false);
        ASSERT_THAT_VECTOR3(D, true, true, true);
        ASSERT_THAT_VECTOR3(E, false, false, false);
        ASSERT_THAT_VECTOR3(F, true, true, true);
    }

    // Operator -
    {
        compages::core::Vector3f A(compages::core::Vector3f::NEGATIVE_UNIT_SCALE);
        compages::core::Vector3f B = -A;

        ASSERT_THAT_VECTOR3(
            B, compages::core::one<float>(), compages::core::one<float>(), compages::core::one<float>());
    }
}

//--------------------------------------------------------------------------
TEST(TestVectors, testArithmetic)
{
    compages::core::Vector3f v2(1.0f, 2.0f, 3.0f);
    compages::core::Vector3f v3(1.0f, 2.0f);
    compages::core::Vector3f v5(1, 2, 3);
    const float c_scalar = -2.0f;
    float scalar = -2.0f;

    // Addition, substraction
    {
        compages::core::Vector3f r1 = v2 + v5 + v3;
        compages::core::Vector3f r2 = compages::core::Vector3f::ZERO + 4.0f;
        compages::core::Vector3f r3 = compages::core::Vector3f::UNIT_X - compages::core::Vector3f::UNIT_X;
        compages::core::Vector3f r4 = -compages::core::Vector3f::UNIT_X;
        compages::core::Vector3f r5 = +compages::core::Vector3f::UNIT_X;
        compages::core::Vector3f r6 = -v2;

        ASSERT_NEAR_VECTOR3(r1, 3.0f, 6.0f, 6.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r2, 4.0f, 4.0f, 4.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r3, 0.0f, 0.0f, 0.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r4, -1.0f, 0.0f, 0.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r5, +1.0f, 0.0f, 0.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r6, -1.0f, -2.0f, -3.0f, 0.001f);
    }

    // Multiplication
    {
        compages::core::Vector3f r1 = v5 * 2.0f;
        compages::core::Vector3f r2 = v5 * -2.0f;
        compages::core::Vector3f r3 = -2.0f * v5;
        compages::core::Vector3f r4 = c_scalar * v5;
        compages::core::Vector3f r5 = -v5 * 2.0f;
        compages::core::Vector3f r6 = -v5 * c_scalar;
        compages::core::Vector3f r7 = -v5 * scalar;

        ASSERT_NEAR_VECTOR3(r1, 2.0f, 4.0f, 6.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r2, -2.0f, -4.0f, -6.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r3, -2.0f, -4.0f, -6.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r4, -2.0f, -4.0f, -6.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r5, -2.0f, -4.0f, -6.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r6, 2.0f, 4.0f, 6.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r7, 2.0f, 4.0f, 6.0f, 0.001f);
    }

    // Division
    {
        compages::core::Vector3f r1 = v5 / 2.0f;
        compages::core::Vector3f r2 = v5 / -2.0f;
        compages::core::Vector3f r3 = -2.0f / v5;
        compages::core::Vector3f r4 = c_scalar / v5;
        compages::core::Vector3f r5 = scalar / v5;
        compages::core::Vector3f r6 = -v5 / 2.0f;
        compages::core::Vector3f r7 = -v5 / c_scalar;
        compages::core::Vector3f r8 = -v5 / scalar;

        ASSERT_NEAR_VECTOR3(r1, 0.5f, 1.0f, 3.0f / 2.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r2, -0.5f, -1.0f, -3.0f / 2.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r3, -2.0f, -1.0f, -2.0f / 3.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r4, -2.0f, -1.0f, -2.0f / 3.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r5, -2.0f, -1.0f, -2.0f / 3.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r6, -0.5f, -1.0f, -3.0f / 2.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r7, 0.5f, 1.0f, 3.0f / 2.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(r8, 0.5f, 1.0f, 3.0f / 2.0f, 0.001f);
    }

    // Self operation
    {
        compages::core::Vector3f v(v2);
        v += 1.0f;
        ASSERT_NEAR_VECTOR3(v, 2.0f, 3.0f, 4.0f, 0.001f);
        v += 1.0f;
        ASSERT_NEAR_VECTOR3(v, 3.0f, 4.0f, 5.0f, 0.001f);
        v -= 2.0f;
        ASSERT_NEAR_VECTOR3(v, 1.0f, 2.0f, 3.0f, 0.001f);
        v /= 2.0f;
        ASSERT_NEAR_VECTOR3(v, 1.0f / 2.0f, 2.0f / 2.0f, 3.0f / 2.0f, 0.001f);
        v *= 2.0f;
        ASSERT_NEAR_VECTOR3(v, 1.0f, 2.0f, 3.0f, 0.001f);
    }
}

//--------------------------------------------------------------------------
TEST(TestVectors, testOperations)
{
    compages::core::Vector3f v2(1.0f, 2.0f, 3.0f);
    compages::core::Vector3f v6 = { -4, 5, -6 };
    compages::core::Vector3f dummy = compages::core::Vector3f::DUMMY;

    // Min, max, clamp, abs
    {
        ASSERT_NEAR_VECTOR3(compages::core::vector::abs(v6), 4.0f, 5.0f, 6.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(compages::core::vector::min(v2, dummy), v2.x, v2.y, v2.z, 0.001f);
        ASSERT_NEAR_VECTOR3(compages::core::vector::max(v2, dummy), v2.x, v2.y, v2.z, 0.001f);
        ASSERT_NEAR_VECTOR3(
            compages::core::vector::min(v2, compages::core::Vector3f::UNIT_Z + 1.0f), 1.0f, 1.0f, 2.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(
            compages::core::vector::max(v2, compages::core::Vector3f::UNIT_Y + 2.0f), 2.0f, 3.0f, 3.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(
            compages::core::vector::clamp(v6, -5.0f, 3.0f), -4.0f, 3.0f, -5.0f, 0.001f);
        ASSERT_NEAR_VECTOR3(compages::core::vector::clamp(compages::core::vector::abs(v6), -3.0f, 5.0f),
                            4.0f,
                            5.0f,
                            5.0f,
                            0.001f);
    }

    // Middle Point
    {
        ASSERT_NEAR_VECTOR3(compages::core::vector::middle(compages::core::Vector3f::ZERO, compages::core::Vector3f::UNIT_Z),
                            0.0f,
                            0.0f,
                            0.5f,
                            0.001f);
        ASSERT_NEAR_VECTOR3(compages::core::vector::middle(compages::core::Vector3f::ZERO, compages::core::Vector3f::UNIT_Y),
                            0.0f,
                            0.5f,
                            0.0f,
                            0.001f);
        ASSERT_NEAR_VECTOR3(compages::core::vector::middle(compages::core::Vector3f::ZERO, compages::core::Vector3f::UNIT_X),
                            0.5f,
                            0.0f,
                            0.0f,
                            0.001f);
        ASSERT_NEAR_VECTOR3(compages::core::vector::middle(compages::core::Vector3f::UNIT_X, -compages::core::Vector3f::UNIT_X),
                            0.0f,
                            0.0f,
                            0.0f,
                            0.001f);
        ASSERT_NEAR_VECTOR3(compages::core::vector::middle(-compages::core::Vector3f::UNIT_X, compages::core::Vector3f::UNIT_X),
                            0.0f,
                            0.0f,
                            0.0f,
                            0.001f);
        ASSERT_NEAR_VECTOR3(-compages::core::vector::middle(compages::core::Vector3f::UNIT_X, compages::core::Vector3f::UNIT_X),
                            -1.0f,
                            0.0f,
                            0.0f,
                            0.001f);
    }

    // Add scaled
    {
        compages::core::Vector3f v1 = compages::core::Vector3f::UNIT_SCALE;
        v1.addScaled(compages::core::Vector3f::UNIT_X, 2.0f);
        compages::core::Vector3b b = (v1 == compages::core::Vector3f(3.0f, 1.0f, 1.0f));
        ASSERT_THAT_VECTOR3(b, true, true, true);
    }

    // Distance
    {
        compages::core::Vector3f v1 = compages::core::Vector3f::UNIT_SCALE * 2.0f;
        ASSERT_EQ(compages::core::sqrt(12.0f), compages::core::vector::distance(v1, compages::core::Vector3f::ZERO));
        v1 = compages::core::Vector3f::ZERO;
        ASSERT_EQ(0.0f, compages::core::vector::distance(v1, compages::core::Vector3f::ZERO));
    }

    // Norm
    {
        ASSERT_EQ(5.0f, compages::core::vector::norm(compages::core::Vector2i(-3, 4)));
        ASSERT_EQ(5.0f, compages::core::vector::magnitude(compages::core::Vector2i(-3, 4)));
        ASSERT_EQ(5.0f, compages::core::Vector2f(-3, 4).norm());
        ASSERT_EQ(5.0f, compages::core::Vector2f(-3, 4).norm());
        ASSERT_EQ(7.0f, compages::core::Vector3f(3, -2, 6).norm());
        ASSERT_EQ(5.0f, compages::core::sqrt(compages::core::vector::squaredMagnitude(compages::core::Vector2f(-3, 4))));
        ASSERT_EQ(1.0f, compages::core::vector::magnitude(compages::core::Vector3f::UNIT_X));
        ASSERT_EQ(compages::core::sqrt(3.0f), compages::core::vector::magnitude(compages::core::Vector3f::UNIT_SCALE));
        ASSERT_EQ(compages::core::sqrt(3.0f), compages::core::vector::magnitude(-compages::core::Vector3f::UNIT_SCALE));
        ASSERT_EQ(3.0f, compages::core::vector::squaredMagnitude(compages::core::Vector3f::UNIT_SCALE));
        ASSERT_EQ(0.0f, compages::core::vector::dot(compages::core::Vector3f::UNIT_X, compages::core::Vector3f::UNIT_Y));
        ASSERT_EQ(3.0f,
                  compages::core::vector::dot(compages::core::Vector3f::UNIT_SCALE, compages::core::Vector3f::UNIT_SCALE));
        ASSERT_EQ(24.0f, compages::core::vector::dot(compages::core::Vector2f(3.0f, 4.0f), compages::core::Vector2f(4.0f, 3.0f)));
    }

    // Component-wise product in place (must not read uninitialized storage)
    {
        compages::core::Vector3f a(1.0f, 2.0f, 3.0f);
        compages::core::Vector3f const b(4.0f, 5.0f, 6.0f);
        compages::core::vector::componentProductUpdate(a, b);
        ASSERT_NEAR_VECTOR3(a, 4.0f, 10.0f, 18.0f, 0.001f);
        ASSERT_EQ(&a, &compages::core::vector::componentProductUpdate(a, b));
    }

    // Vector4 from Vector2: z stays zero, w takes the scalar
    {
        compages::core::Vector4f v(compages::core::Vector2f(1.0f, 2.0f), 3.0f);
        ASSERT_NEAR(0.0f, v.z, 0.001f);
        ASSERT_NEAR(3.0f, v.w, 0.001f);
        ASSERT_NEAR(1.0f, v.x, 0.001f);
        ASSERT_NEAR(2.0f, v.y, 0.001f);
    }

    // Normalize
    {
        compages::core::Vector3f v3 = compages::core::vector::normalize(compages::core::Vector3f::UNIT_SCALE * 2.0f);
        compages::core::Vector3f v1 = compages::core::Vector3f::UNIT_SCALE / compages::core::sqrt(3.0f);
        compages::core::Vector3b b = (v1 == v3);
        ASSERT_THAT_VECTOR3(b, true, true, true);
    }

    // Inverse
    {
        compages::core::Vector3f v1 = compages::core::Vector3f::UNIT_SCALE;
        v1.invert();
        ASSERT_THAT_VECTOR3(v1,
                            -compages::core::one<float>(),
                            -compages::core::one<float>(),
                            -compages::core::one<float>());
    }

    // Perpendicular 2D
    {
        compages::core::Vector2f af = compages::core::Vector2f(2.0f, 4.0f);
        compages::core::Vector2f of = compages::core::vector::orthogonal(compages::core::Vector2f(2.0f, 4.0f));
        ASSERT_EQ(true, compages::core::vector::areOrthogonal(of, af));
        ASSERT_EQ(false, compages::core::vector::areOrthogonal(af, compages::core::Vector2f(3.0f, 2.0f)));

        compages::core::Vector2i ai = compages::core::Vector2i(2, 4);
        compages::core::Vector2i oi = compages::core::vector::orthogonal(compages::core::Vector2i(2, 4));
        ASSERT_EQ(true, compages::core::vector::areOrthogonal(oi, ai));
        ASSERT_EQ(false, compages::core::vector::areOrthogonal(ai, compages::core::Vector2i(3, 2)));

        compages::core::Vector2b b = compages::core::vector::compare(of, compages::core::Vector2f(-4.0f, 2.0f));
        ASSERT_THAT_VECTOR2(b, true, true);

        b = compages::core::vector::compare(oi, compages::core::Vector2i(-4, 2));
        ASSERT_THAT_VECTOR2(b, true, true);
    }

    // Perpendicular 3D
    {
        // TODO
    }

    // Colinear 2D
    { /*
         ASSERT_EQ(true, compages::core::vector::areCollinear(compages::core::Vector2f(3.0f, -2.0f),
         compages::core::Vector2f(-15.0f, 10.0f))); ASSERT_EQ(false,
         compages::core::vector::areCollinear(compages::core::Vector2f(6.0f, 4.0f), compages::core::Vector2f(4.0f, 2.0f)));

         ASSERT_EQ(true, compages::core::vector::areEquivalent(compages::core::Vector3f(1,0,0),
         compages::core::Vector3f(3,0,0))); ASSERT_EQ(false,
         compages::core::vector::areEquivalent(compages::core::Vector3f(1,0,0), compages::core::Vector3f(0,3,0)));

         ASSERT_EQ(true, compages::core::vector::arePointsAligned(compages::core::Vector3f(0,0,0),
         compages::core::Vector3f(1,0,0), compages::core::Vector3f(3,0,0))); ASSERT_EQ(false,
         compages::core::vector::arePointsAligned(compages::core::Vector3f(0,0,0), compages::core::Vector3f(1,0,0),
         compages::core::Vector3f(0,3,0)));*/
    }

    // Cross product 3D: notation 1
    {
        compages::core::Vector3f v = compages::core::vector::cross(compages::core::Vector3f::UNIT_X, compages::core::Vector3f::UNIT_Y);
        ASSERT_THAT_VECTOR3((v == compages::core::Vector3f::UNIT_Z), true, true, true);
        v = compages::core::vector::cross(compages::core::Vector3f::UNIT_Y, compages::core::Vector3f::UNIT_X);
        ASSERT_THAT_VECTOR3((v == -compages::core::Vector3f::UNIT_Z), true, true, true);
    }

    // Cross product 3D: notation 2
    {
        compages::core::Vector3f v = compages::core::Vector3f::UNIT_X % compages::core::Vector3f::UNIT_Y;
        ASSERT_THAT_VECTOR3((v == compages::core::Vector3f::UNIT_Z), true, true, true);
        v = compages::core::Vector3f::UNIT_Y % compages::core::Vector3f::UNIT_X;
        ASSERT_THAT_VECTOR3((v == -compages::core::Vector3f::UNIT_Z), true, true, true);
    }

    // Cross product 2D: notation 1
    {
        float b = compages::core::vector::cross(compages::core::Vector2f::UNIT_X, compages::core::Vector2f::UNIT_Y);
        ASSERT_EQ(1.0f, b);
        b = compages::core::vector::cross(compages::core::Vector2f::UNIT_Y, compages::core::Vector2f::UNIT_X);
        ASSERT_EQ(-1.0f, b);
    }

    // Cross product 2D: notation 2
    {
        float b = compages::core::Vector2f::UNIT_X % compages::core::Vector2f::UNIT_Y;
        ASSERT_EQ(1.0f, b);
        b = compages::core::Vector2f::UNIT_Y % compages::core::Vector2f::UNIT_X;
        ASSERT_EQ(-1.0f, b);
    }

    // Self Cross product
    {
        compages::core::Vector3f v = compages::core::Vector3f::UNIT_X;
        v %= compages::core::Vector3f::UNIT_Y;
        ASSERT_THAT_VECTOR3((v == compages::core::Vector3f::UNIT_Z), true, true, true);

        v = compages::core::Vector3f::UNIT_Y;
        v %= compages::core::Vector3f::UNIT_X;
        ASSERT_THAT_VECTOR3((v == -compages::core::Vector3f::UNIT_Z), true, true, true);
    }

    // Scalar product: notation 1
    {
        float b = compages::core::vector::dot(compages::core::Vector3f::UNIT_X, compages::core::Vector3f::UNIT_X);
        ASSERT_EQ(1.0f, b);
        b = compages::core::vector::dot(compages::core::Vector3f::UNIT_X, compages::core::Vector3f::UNIT_Y);
        ASSERT_EQ(0.0f, b);
    }

    // Scalar product: notation 2
    {
        float b = compages::core::Vector3f::UNIT_X * compages::core::Vector3f::UNIT_X;
        ASSERT_EQ(1.0f, b);
        b = compages::core::Vector3f::UNIT_X * compages::core::Vector3f::UNIT_Y;
        ASSERT_EQ(0.0f, b);
    }
}

//--------------------------------------------------------------------------
TEST(TestVectors, testComplexMath)
{
    compages::core::Vector3b b1 =
        compages::core::vector::compare(compages::core::Vector3f::NEGATIVE_UNIT_X,
                        compages::core::vector::reflect(compages::core::Vector3f::UNIT_X, compages::core::Vector3f::UNIT_X));
    compages::core::Vector3b b2 =
        compages::core::vector::compare(compages::core::Vector3f::NEGATIVE_UNIT_Y,
                        compages::core::vector::reflect(compages::core::Vector3f::UNIT_Y, compages::core::Vector3f::UNIT_Y));
    compages::core::Vector3b b3 =
        compages::core::vector::compare(compages::core::Vector3f::NEGATIVE_UNIT_Z,
                        compages::core::vector::reflect(compages::core::Vector3f::UNIT_Z, compages::core::Vector3f::UNIT_Z));
    compages::core::Vector3b b4 = compages::core::vector::compare(
        compages::core::Vector3f::UNIT_X, compages::core::vector::reflect(compages::core::Vector3f::UNIT_X, compages::core::Vector3f::UNIT_Y));

    ASSERT_THAT_VECTOR3(b1, true, true, true);
    ASSERT_THAT_VECTOR3(b2, true, true, true);
    ASSERT_THAT_VECTOR3(b3, true, true, true);
    ASSERT_THAT_VECTOR3(b4, true, true, true);

    ASSERT_DOUBLES_EQUAL(
        00.0f,
        units::angle::degree_t(
            compages::core::vector::angleBetween(compages::core::Vector3f::UNIT_X, compages::core::Vector3f::UNIT_X))
            .to<float>(),
        0.0001f);
    ASSERT_DOUBLES_EQUAL(
        90.0f,
        units::angle::degree_t(
            compages::core::vector::angleBetween(compages::core::Vector3f::UNIT_X, compages::core::Vector3f::UNIT_Y))
            .to<float>(),
        0.0001f);
    ASSERT_DOUBLES_EQUAL(
        90.0f,
        units::angle::degree_t(
            compages::core::vector::angleBetween(compages::core::Vector3f::UNIT_Y, compages::core::Vector3f::UNIT_X))
            .to<float>(),
        0.0001f);
    ASSERT_DOUBLES_EQUAL(
        180.0f,
        units::angle::degree_t(
            compages::core::vector::angleBetween(compages::core::Vector3f::UNIT_X, compages::core::Vector3f::NEGATIVE_UNIT_X))
            .to<float>(),
        0.0001f);
    ASSERT_DOUBLES_EQUAL(
        180.0f,
        units::angle::degree_t(
            compages::core::vector::angleBetween(compages::core::Vector3f::NEGATIVE_UNIT_X, compages::core::Vector3f::UNIT_X))
            .to<float>(),
        0.0001f);
    ASSERT_DOUBLES_EQUAL(
        90.0f,
        units::angle::degree_t(
            compages::core::vector::angleBetween(compages::core::Vector3f::UNIT_X, compages::core::Vector3f::NEGATIVE_UNIT_Y))
            .to<float>(),
        0.0001f);
    ASSERT_DOUBLES_EQUAL(
        90.0f,
        units::angle::degree_t(
            compages::core::vector::angleBetween(compages::core::Vector3f::NEGATIVE_UNIT_X, compages::core::Vector3f::UNIT_Y))
            .to<float>(),
        0.0001f);
    ASSERT_DOUBLES_EQUAL(
        90.0f,
        units::angle::degree_t(
            compages::core::vector::angleBetween(compages::core::Vector3f::NEGATIVE_UNIT_X, compages::core::Vector3f::UNIT_Z))
            .to<float>(),
        0.0001f);

    compages::core::Vector2f a(1.0, 2.0);
    ASSERT_EQ(1.5f, compages::core::vector::mean(a));
    compages::core::Vector3f b(1.0, 2.0, 3.0);
    ASSERT_EQ(2.0f, compages::core::vector::mean(b));
    compages::core::Vector4f c(1.0, 2.0, 3.0, 4.0);
    ASSERT_EQ(2.5f, compages::core::vector::mean(c));

    compages::core::Vector<float, 5_z> measurements({ 2.0f, 4.0f, 5.0f, 7.0f, 7.0f });
    // Mean: (2+4+5+7+7)/5 = 5
    float m = compages::core::vector::mean(measurements);
    ASSERT_EQ(5.0f, m);
    // Deviation from average = mean - x[i]
    // = [5-2, 5-4, 5-5, 5-7, 5-7]
    // = [3 1 0 2 2]
    compages::core::Vector<float, 5_z> deviation = m - measurements;
    // Square of the deviation: (mean - x[i])^2
    // = [3^2 1^2 0^2 2^2 2^2]
    compages::core::Vector<float, 5_z> deviation2 =
        compages::core::vector::componentProduct(deviation, deviation);
    // Variance: sum((mean - x[i])^2) / size()
    // = (3^2 + 1^2 + 0^2 + 2^2 + 2^2) / 5
    ASSERT_EQ(3.6f, compages::core::vector::mean(deviation2));
}
