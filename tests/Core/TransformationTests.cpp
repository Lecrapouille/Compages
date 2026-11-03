// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Core/Transformation.hpp"
#include "main.hpp"



#define ASSERT_NEAR_VECTOR4(vect, a, b, c, d, thresh) \
    ASSERT_NEAR(vect[0], a, thresh);                  \
    ASSERT_NEAR(vect[1], b, thresh);                  \
    ASSERT_NEAR(vect[2], c, thresh);                  \
    ASSERT_NEAR(vect[3], d, thresh)

#define ASSERT_MATRIX_NEAR(expected, actual, thresh)                  \
    for (size_t idx = 0; idx < 16u; ++idx)                            \
    {                                                                 \
        ASSERT_NEAR(expected.data()[idx], actual.data()[idx], thresh) \
            << "at index: " << idx;                                   \
    }

#define ASSERT_MATRIX(expected, actual)                     \
    for (size_t idx = 0; idx < 16u; ++idx)                  \
    {                                                       \
        ASSERT_EQ(expected.data()[idx], actual.data()[idx]) \
            << "at index: " << idx;                         \
    }

TEST(TestTransformation, testTranslate)
{
    compages::Matrix44f I(compages::matrix::Identity);
    compages::Matrix44f M = compages::translate(I, compages::Vector3f(2.0f, 3.0f, 4.0f));

    ASSERT_MATRIX(M,
                  compages::Matrix44f({ 1.0f,
                              0.0f,
                              0.0f,
                              2.0f,
                              0.0f,
                              1.0f,
                              0.0f,
                              3.0f,
                              0.0f,
                              0.0f,
                              1.0f,
                              4.0f,
                              0.0f,
                              0.0f,
                              0.0f,
                              1.0f }));
}

TEST(TestTransformation, testScale)
{
    compages::Matrix44f I(compages::matrix::Identity);
    compages::Matrix44f M = compages::scale(I, compages::Vector3f(2.0f, 3.0f, 4.0f));

    ASSERT_MATRIX(M,
                  compages::Matrix44f({ 2.0f,
                              0.0f,
                              0.0f,
                              0.0f,
                              0.0f,
                              3.0f,
                              0.0f,
                              0.0f,
                              0.0f,
                              0.0f,
                              4.0f,
                              0.0f,
                              0.0f,
                              0.0f,
                              0.0f,
                              1.0f }));
}

TEST(TestTransformation, testRotation)
{
    // Not 45 degrees: there the sine and the cosine are equal, so the expected
    // matrix below stayed correct even when it used the wrong one of the two,
    // and the test could not have caught the mistake in the first place.
    const units::angle::radian_t angle(units::angle::degree_t(30.0f));
    const float c = std::cos(angle.to<float>());
    const float s = std::sin(angle.to<float>());
    const float oc = 1.0f - c;
    const compages::Vector3f axis(0.371391f, 0.557086f, 0.742781f);
    compages::Matrix44f I(compages::matrix::Identity);

    // Actual
    compages::Matrix44f A = compages::rotate(I, angle, axis);

    // Expected https://fr.wikipedia.org/wiki/Matrice_de_rotation
    compages::Matrix44f E = { //
                    axis[0] * axis[0] * oc + c,
                    axis[0] * axis[1] * oc - axis[2] * s,
                    axis[0] * axis[2] * oc + axis[1] * s,
                    0.0f,

                    //
                    axis[0] * axis[1] * oc + axis[2] * s,
                    axis[1] * axis[1] * oc + c,
                    axis[1] * axis[2] * oc - axis[0] * s,
                    0.0f,

                    //
                    axis[0] * axis[2] * oc - axis[1] * s,
                    axis[1] * axis[2] * oc + axis[0] * s,
                    axis[2] * axis[2] * oc + c,
                    0.0f,

                    //
                    0.0f,
                    0.0f,
                    0.0f,
                    1.0f
    };

    ASSERT_MATRIX_NEAR(A, E, 0.0001f);
}

TEST(TestTransformation, testOrtho)
{
    const float left = 1.0f;
    const float right = 2.0f;
    const float bottom = 3.0f;
    const float top = 4.0f;
    const float near = 5.0f;
    const float far = 6.0f;

    // Actual
    compages::Matrix44f A = compages::ortho(left, right, bottom, top, near, far);

    // Expected: right-handed ortho consistent with compages::perspective. GLM's
    // orthoRH_NO puts -2/(f-n) at [2][2], and this library follows the same
    // convention so both projections can be swapped freely.
    compages::Matrix44f E(compages::matrix::Identity);
    E[0][0] = 2.0f / (right - left);
    E[1][1] = 2.0f / (top - bottom);
    E[2][2] = -2.0f / (far - near);
    E[0][3] = -(right + left) / (right - left);
    E[1][3] = -(top + bottom) / (top - bottom);
    E[2][3] = -(far + near) / (far - near);

    ASSERT_MATRIX_NEAR(A, E, 0.0001f);
}

TEST(TestTransformation, testPersp)
{
    units::angle::radian_t const fov(units::angle::degree_t(45.0f));
    float const aspect = 800.0f / 600.0f;
    float const near = 0.1f;
    float const far = 100.0f;
    float const tanHalfFovY = std::tan(fov.to<float>() / 2.0f);

    // Actual
    compages::Matrix44f A = compages::perspective(fov, aspect, near, far);

    // Expected: transpose of glm::perspective (glm stores column-major).
    compages::Matrix44f E(0.0f);
    E[0][0] = 1.0f / (aspect * tanHalfFovY);
    E[1][1] = 1.0f / (tanHalfFovY);
    E[2][2] = -(far + near) / (far - near);
    E[2][3] = -(2.0f * far * near) / (far - near);
    E[3][2] = -1.0f;

    ASSERT_MATRIX_NEAR(A, E, 0.0001f);
}

TEST(TestTransformation, testLookAt)
{
    const compages::Vector3f position(1.0f, 1.0f, 1.0f);
    const compages::Vector3f target(0.0f, 0.0f, 0.0f);
    const compages::Vector3f upwards(0.0f, 0.0f, 1.0f);

    // Actual
    compages::Matrix44f A = compages::lookAt(position, target, upwards);

    // Expected
    compages::Matrix44f E(compages::matrix::Identity);
    compages::Vector3f const direction(compages::vector::normalize(target - position));
    compages::Vector3f const right(compages::vector::normalize(compages::vector::cross(direction, upwards)));
    compages::Vector3f const up(compages::vector::cross(right, direction));

    E[0][0] = right.x;
    E[0][1] = right.y;
    E[0][2] = right.z;
    E[1][0] = up.x;
    E[1][1] = up.y;
    E[1][2] = up.z;
    E[2][0] = -direction.x;
    E[2][1] = -direction.y;
    E[2][2] = -direction.z;
    E[0][3] = -(compages::vector::dot(right, position));
    E[1][3] = -(compages::vector::dot(up, position));
    E[2][3] = compages::vector::dot(direction, position);

    ASSERT_MATRIX_NEAR(A, E, 0.0001f);
}

TEST(TestTransformation, testOperations)
{
    compages::Matrix44f const I(compages::matrix::Identity);
    compages::Vector4f x(1, 2, 3, 1); // Initial position
    compages::Vector4f R;             // Final position

    // Rotation order: translate a point (1,2,3) through a transform matrix
    // translation by (1,2,3).
    {
        // Translation matrix
        compages::Matrix44f M({ 1, 0, 0, 1, 0, 1, 0, 2, 0, 0, 1, 3, 0, 0, 0, 1 });

        // Correct formula (Scilab code):
        // [1 0 0 1; 0 1 0 2; 0 0 1 3; 0 0 0 1] * [1 2 3 1]'
        // Where ' is the transpose operation.
        // std::cout << "M . x = " << M * x << std::endl << std::endl;
        R = M * x;
        ASSERT_NEAR_VECTOR4(R, 2.0f, 4.0f, 6.0f, 1.0f, 0.0001f);

        // x^T * M, written with the explicit transpose (Scilab code):
        // [1 2 3 1] * [1 0 0 1; 0 1 0 2; 0 0 1 3; 0 0 0 1]
        R = compages::transpose(M) * x;
        ASSERT_NEAR_VECTOR4(R, 1.0f, 2.0f, 3.0f, 15.0f, 0.0001f);
    }

    {
        compages::Matrix44f M = compages::translate(I, compages::Vector3f(1, 2, 3));
        R = M * x;
        ASSERT_NEAR_VECTOR4(R, 2.0f, 4.0f, 6.0f, 1.0f, 0.0001f);
    }

    // Scaling
    {
        compages::Matrix44f M = compages::scale(I, compages::Vector3f(1, 2, 3));
        R = M * x;
        ASSERT_NEAR_VECTOR4(R, 1.0f, 4.0f, 9.0f, 1.0f, 0.0001f);
    }

    // Multiple rotations
    {
        x = compages::Vector4f(1, 0, 0, 1);
        units::angle::degree_t angle(90.0f);
        compages::Matrix44f M = compages::rotate(I, angle, compages::Vector3f::UNIT_Y);
        R = M * x;
        ASSERT_NEAR_VECTOR4(R, 0.0f, 0.0f, -1.0f, 1.0f, 0.0001f);

        compages::Matrix44f chain = M * M * M * M;
        R = chain * x;
        // std::cout << "3 Rotates: "
        //           << x << " * " << M << " = " << R
        //           << std::endl << std::endl;
        ASSERT_NEAR_VECTOR4(R, 1.0f, 0.0f, 0.0f, 1.0f, 0.0001f);
    }
}

// TODO
#if 0
void glm_lookAt(glm::vec3 const &position, glm::vec3 const &target, glm::vec3 const &up)
{
    std::cout << "GLM:" << std::endl;
    glm::mat4 mat = glm::lookAt(position, target, up);
    std::cout << "  glm::lookAt: " << glm::to_string(mat) << std::endl;

    glm::quat orientation = glm::conjugate(glm::quat_cast(mat));
    std::cout << "  glm::quat conjug: " << glm::to_string(orientation) << std::endl;
    std::cout << "  =>: " << glm::angle(orientation) * 57.2958f << " "
              << glm::to_string(glm::axis(orientation)) << std::endl;

    // Camera::view
    //std::cout << "  POS " << glm::to_string(position) << std::endl;
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), -position);
    std::cout << "  glm::trans " << glm::to_string(trans) << std::endl;
    glm::mat4 rot = glm::mat4_cast(orientation);
    std::cout << "  glm::rot " << glm::to_string(rot) << std::endl;
    glm::mat4 view = rot * trans;
    std::cout << "  glm::view " << glm::to_string(view) << std::endl;

    {
        glm::mat4 tra({1, 0, 0, 0,
                       0, 1, 0, 0,
                       0, 0, 1, 0,
                       -1, 0, 0, 1});
        glm::mat4 rot({0, 0, -1, 0,
                       0, 1, 0, 0,
                       1, 0, 0, 0,
                       0, 0, 0, 1});

        std::cout << glm::to_string(rot * tra) << std::endl;
    }
}

void qq_lookAt(compages::Vector3f const &position, compages::Vector3f const &target, compages::Vector3f const &up)
{
    std::cout << "\n\nQQ:" << std::endl;
    compages::Matrix44f mat = compages::lookAt(position, target, up);
    std::cout << "  qq::LookAt: " << mat << std::endl;

    compages::Quatf orientation = compages::Quatf::fromMatrix(mat).conjugate();
    std::cout << "  qq::quat: " << orientation << std::endl;
    std::cout << "  =>: " << orientation.angle() * 57.2958f << " angle: "
              << orientation.axis() << std::endl;

    // Camera::view
    compages::Matrix44f I(compages::matrix::Identity);
    compages::Matrix44f trans = compages::translate(I, -position);
    std::cout << "  qq::trans " << trans << std::endl;

    compages::Matrix44f rot = orientation.toMatrix();
    std::cout << "  qq::rot " << rot << std::endl;

    compages::Matrix44f view = trans * rot; // FIXME shall be rot * trans
    std::cout << "  qq::view " << view << std::endl;
    //std::cout << "  qq::view\n" << operator*(rot, trans) << std::endl;

    {
        compages::Matrix44f tra({1, 0, 0, 0,
                       0, 1, 0, 0,
                       0, 0, 1, 0,
                       -1, 0, 0, 1});
        compages::Matrix44f rot({0, 0, -1, 0,
                       0, 1, 0, 0,
                       1, 0, 0, 0,
                       0, 0, 0, 1});

        std::cout << rot * tra << std::endl;
    }
}


int main()
{
    glm_lookAt(glm::vec3(1,0,0),
               glm::vec3(0,0,0),
               glm::vec3(0,1,0));

    qq_lookAt(compages::Vector3f(1,0,0),
              compages::Vector3f(0,0,0),
              compages::Vector3f(0,1,0));

    return 0;
}
#endif
