//=====================================================================
// OpenGLCppWrapper: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenGLCppWrapper is distributedin the hope that it will be useful, but
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include "Math/Transformable.hpp"
#pragma GCC diagnostic pop
#undef protected
#undef private

//--------------------------------------------------------------------------
#define ASSERT_NEAR_VECTOR3(vect, a, b, c, thresh) \
    ASSERT_NEAR(vect[0], a, thresh);               \
    ASSERT_NEAR(vect[1], b, thresh);               \
    ASSERT_NEAR(vect[2], c, thresh)

//--------------------------------------------------------------------------
#define ASSERT_EQ_VECTOR3(vect, a, b, c) \
    ASSERT_EQ(vect[0], a);               \
    ASSERT_EQ(vect[1], b);               \
    ASSERT_EQ(vect[2], c)

//--------------------------------------------------------------------------
#define ASSERT_MATRIX_NEAR(expected, actual, thresh)                  \
    for (size_t idx = 0; idx < 16u; ++idx)                            \
    {                                                                 \
        ASSERT_NEAR(expected.data()[idx], actual.data()[idx], thresh) \
            << "at index: " << idx;                                   \
    }

//--------------------------------------------------------------------------
#define ASSERT_MATRIX(expected, actual)                     \
    for (size_t idx = 0; idx < 16u; ++idx)                  \
    {                                                       \
        ASSERT_EQ(expected.data()[idx], actual.data()[idx]) \
            << "at index: " << idx;                         \
    }

//--------------------------------------------------------------------------
TEST(TestTransformable, testConstructor)
{
    Transformable3D M;

    // Constructor
    {
        ASSERT_EQ_VECTOR3(M.m_origin, 0.0f, 0.0f, 0.0f);
        ASSERT_EQ_VECTOR3(M.m_position, 0.0f, 0.0f, 0.0f);
        ASSERT_EQ_VECTOR3(M.m_scale, 1.0f, 1.0f, 1.0f);
        ASSERT_EQ_VECTOR3(M.m_local_scaling, 1.0f, 1.0f, 1.0f);
        ASSERT_MATRIX(Matrix44f(matrix::Identity), M.m_transform);
        ASSERT_MATRIX(Matrix44f(matrix::Identity), M.m_inverse_transform);
        ASSERT_EQ(false, M.m_transform_needs_update);
        ASSERT_EQ(false, M.m_inverse_trans_needs_update);

        ASSERT_EQ_VECTOR3(M.origin(), 0.0f, 0.0f, 0.0f);
        ASSERT_EQ_VECTOR3(M.position(), 0.0f, 0.0f, 0.0f);
        ASSERT_EQ_VECTOR3(M.local_position(), 0.0f, 0.0f, 0.0f);
        ASSERT_EQ_VECTOR3(M.scaling(), 1.0f, 1.0f, 1.0f);
        ASSERT_EQ_VECTOR3(M.localScale(), 1.0f, 1.0f, 1.0f);
        ASSERT_MATRIX(Matrix44f(matrix::Identity), M.matrix());
        ASSERT_MATRIX(Matrix44f(matrix::Identity), M.invMatrix());
    }

    // Change
    {
        M.m_origin = M.m_position = Vector3f(1.0f);
        M.m_scale = M.m_local_scaling = Vector3f(2.0f);
        M.m_transform *= 2.0f;
        M.m_inverse_transform *= 2.0f;
        M.m_transform_needs_update = M.m_inverse_trans_needs_update = true;

        ASSERT_EQ_VECTOR3(M.origin(), 1.0f, 1.0f, 1.0f);
        ASSERT_EQ_VECTOR3(M.position(), 1.0f, 1.0f, 1.0f);
        ASSERT_EQ_VECTOR3(M.local_position(), 0.0f, 0.0f, 0.0f);
        ASSERT_EQ_VECTOR3(M.scaling(), 2.0f, 2.0f, 2.0f);
        ASSERT_EQ_VECTOR3(M.localScale(), 2.0f, 2.0f, 2.0f);
        ASSERT_MATRIX(M.matrix(),
                      Matrix44f({ 2.0f,
                                  0.0f,
                                  0.0f,
                                  0.0f,
                                  0.0f,
                                  2.0f,
                                  0.0f,
                                  0.0f,
                                  0.0f,
                                  0.0f,
                                  2.0f,
                                  0.0f,
                                  0.0f,
                                  0.0f,
                                  0.0f,
                                  1.0f }));
        ASSERT_MATRIX(M.invMatrix(),
                      Matrix44f({ 0.5f,
                                  0.0f,
                                  0.0f,
                                  0.0f,
                                  0.0f,
                                  0.5f,
                                  0.0f,
                                  0.0f,
                                  0.0f,
                                  0.0f,
                                  0.5f,
                                  0.0f,
                                  0.0f,
                                  0.0f,
                                  0.0f,
                                  1.0f }));
    }

    // Reset
    {
        M.reset();
        ASSERT_EQ_VECTOR3(M.m_origin, 0.0f, 0.0f, 0.0f);
        ASSERT_EQ_VECTOR3(M.m_position, 0.0f, 0.0f, 0.0f);
        ASSERT_EQ_VECTOR3(M.m_scale, 1.0f, 1.0f, 1.0f);
        ASSERT_EQ_VECTOR3(M.m_local_scaling, 1.0f, 1.0f, 1.0f);
        ASSERT_MATRIX(Matrix44f(matrix::Identity), M.m_transform);
        ASSERT_MATRIX(Matrix44f(matrix::Identity), M.m_inverse_transform);
        ASSERT_EQ(false, M.m_transform_needs_update);
        ASSERT_EQ(false, M.m_inverse_trans_needs_update);
    }
}

//--------------------------------------------------------------------------
TEST(TestTransformable, QQ)
{
    Transformable3D M;

    std::cout << "Up: " << M.up() << std::endl;
    std::cout << "Right: " << M.right() << std::endl;
    std::cout << "Forward: " << M.forward() << std::endl;
    std::cout << "Direction: " << M.direction() << std::endl;

    std::cout << "Rotation: " << M.rotation() << std::endl;
    M.rotate(units::angle::degree_t(45.0f), Vector3f(1, 0, 0));
    std::cout << "Rotation: " << M.rotation() << std::endl;
    M.position(Vector3f(1, 2, 3));
    M.roll(units::angle::degree_t(45.0f));
    std::cout << "Transform: " << M.matrix() << std::endl << std::endl;
}
