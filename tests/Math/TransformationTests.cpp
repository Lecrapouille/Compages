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

#include "Math/Transformation.hpp"
#include "main.hpp"

//--------------------------------------------------------------------------
#define ASSERT_NEAR_VECTOR4(vect, a, b, c, d, thresh) \
    ASSERT_NEAR(vect[0], a, thresh);                  \
    ASSERT_NEAR(vect[1], b, thresh);                  \
    ASSERT_NEAR(vect[2], c, thresh);                  \
    ASSERT_NEAR(vect[3], d, thresh)

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
TEST(TestTransformation, testTranslate)
{
    Matrix44f I(matrix::Identity);
    Matrix44f M = matrix::translate(I, Vector3f(2.0f, 3.0f, 4.0f));

    ASSERT_MATRIX(M,
                  Matrix44f({ 1.0f,
                              0.0f,
                              0.0f,
                              0.0f,
                              0.0f,
                              1.0f,
                              0.0f,
                              0.0f,
                              0.0f,
                              0.0f,
                              1.0f,
                              0.0f,
                              2.0f,
                              3.0f,
                              4.0f,
                              1.0f }));
}

//--------------------------------------------------------------------------
TEST(TestTransformation, testScale)
{
    Matrix44f I(matrix::Identity);
    Matrix44f M = matrix::scale(I, Vector3f(2.0f, 3.0f, 4.0f));

    ASSERT_MATRIX(M,
                  Matrix44f({ 2.0f,
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

//--------------------------------------------------------------------------
TEST(TestTransformation, testRotation)
{
    const units::angle::radian_t angle(units::angle::degree_t(45.0f));
    const float c = cos(angle.to<float>());
    const float s = cos(angle.to<float>());
    const float oc = 1.0f - c;
    const Vector3f axis(0.371391f, 0.557086f, 0.742781f);
    Matrix44f I(matrix::Identity);

    // Actual
    Matrix44f A = matrix::rotate(I, angle, axis);

    // Expected https://fr.wikipedia.org/wiki/Matrice_de_rotation
    Matrix44f E = { //
                    axis[0] * axis[0] * oc + c,
                    axis[0] * axis[1] * oc + axis[2] * s,
                    axis[0] * axis[2] * oc - axis[1] * s,
                    0.0f,

                    //
                    axis[0] * axis[1] * oc - axis[2] * s,
                    axis[1] * axis[1] * oc + c,
                    axis[1] * axis[2] * oc + axis[0] * s,
                    0.0f,

                    //
                    axis[0] * axis[2] * oc + axis[1] * s,
                    axis[1] * axis[2] * oc - axis[0] * s,
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

//--------------------------------------------------------------------------
TEST(TestTransformation, testOrtho)
{
    const float left = 1.0f;
    const float right = 2.0f;
    const float bottom = 3.0f;
    const float top = 4.0f;
    const float near = 5.0f;
    const float far = 6.0f;

    // Actual
    Matrix44f A = matrix::ortho(left, right, bottom, top, near, far);

    // Expected (glm)
    Matrix44f E(matrix::Identity);
    E[0][0] = 2.0f / (right - left);
    E[1][1] = 2.0f / (top - bottom);
    E[2][2] = 2.0f / (far - near);
    E[3][0] = -(right + left) / (right - left);
    E[3][1] = -(top + bottom) / (top - bottom);
    E[3][2] = -(far + near) / (far - near);

    ASSERT_MATRIX_NEAR(A, E, 0.0001f);
}

//--------------------------------------------------------------------------
TEST(TestTransformation, testPersp)
{
    units::angle::radian_t const fov(units::angle::degree_t(45.0f));
    float const aspect = 800.0f / 600.0f;
    float const near = 0.1f;
    float const far = 100.0f;
    float const tanHalfFovY = std::tan(fov.to<float>() / 2.0f);

    // Actual
    Matrix44f A = matrix::perspective(fov, aspect, near, far);

    // Expected (glm)
    Matrix44f E(0.0f);
    E[0][0] = 1.0f / (aspect * tanHalfFovY);
    E[1][1] = 1.0f / (tanHalfFovY);
    E[2][3] = -1.0f;
    E[2][2] = -(far + near) / (far - near);
    E[3][2] = -(2.0f * far * near) / (far - near);

    ASSERT_MATRIX_NEAR(A, E, 0.0001f);
}

//--------------------------------------------------------------------------
TEST(TestTransformation, testLookAt)
{
    const Vector3f position(1.0f, 1.0f, 1.0f);
    const Vector3f target(0.0f, 0.0f, 0.0f);
    const Vector3f upwards(0.0f, 0.0f, 1.0f);

    // Actual
    Matrix44f A = matrix::lookAt(position, target, upwards);

    // Expected
    Matrix44f E(matrix::Identity);
    Vector3f const direction(vector::normalize(target - position));
    Vector3f const right(vector::normalize(vector::cross(direction, upwards)));
    Vector3f const up(vector::cross(right, direction));

    E[0][0] = right.x;
    E[1][0] = right.y;
    E[2][0] = right.z;
    E[0][1] = up.x;
    E[1][1] = up.y;
    E[2][1] = up.z;
    E[0][2] = -direction.x;
    E[1][2] = -direction.y;
    E[2][2] = -direction.z;
    E[3][0] = -(vector::dot(right, position));
    E[3][1] = -(vector::dot(up, position));
    E[3][2] = vector::dot(direction, position);

    ASSERT_MATRIX_NEAR(A, E, 0.0001f);
}

//--------------------------------------------------------------------------
TEST(TestTransformation, testOperations)
{
    Matrix44f const I(matrix::Identity);
    Vector4f x(1, 2, 3, 1); // Initial position
    Vector4f R;             // Final position

    // Rotation order: translate a point (1,2,3) through a transform matrix
    // translation by (1,2,3).
    {
        // Translation matrix
        Matrix44f M({ 1, 0, 0, 1, 0, 1, 0, 2, 0, 0, 1, 3, 0, 0, 0, 1 });

        // Correct formula (Scilab code):
        // [1 0 0 1; 0 1 0 2; 0 0 1 3; 0 0 0 1] * [1 2 3 1]'
        // Where ' is the transpose operation.
        // std::cout << "M . x = " << M * x << std::endl << std::endl;
        R = M * x;
        ASSERT_NEAR_VECTOR4(R, 2.0f, 4.0f, 6.0f, 1.0f, 0.0001f);

        // Incorrect formula (Scilab code):
        // [1 2 3 1] * [1 0 0 0; 0 1 0 0; 0 0 1 0; 1 2 3 1]
        // std::cout << "x . M = " << x * M << std::endl << std::endl;
        R = x * M;
        ASSERT_NEAR_VECTOR4(R, 1.0f, 2.0f, 3.0f, 15.0f, 0.0001f);
    }

    // But because OpenGL needs transposed matrix matrices operations are
    // inverted: transpose(A.B) == transpose(B) * transpose(A)
    {
        Matrix44f M = matrix::translate(I, Vector3f(1, 2, 3));
        R = x * M;
        // std::cout << "Translation: "
        //           << x << " * " << M << " = " << R
        //           << std::endl << std::endl;
        ASSERT_NEAR_VECTOR4(R, 2.0f, 4.0f, 6.0f, 1.0f, 0.0001f);
    }

    // Scaling
    {
        Matrix44f M = matrix::scale(I, Vector3f(1, 2, 3));
        R = x * M;
        // std::cout << "Scale: "
        //           << x << " * " << M << " = " << R
        //           << std::endl << std::endl;
        ASSERT_NEAR_VECTOR4(R, 1.0f, 4.0f, 9.0f, 1.0f, 0.0001f);
    }

    // Multiple rotations
    {
        x = Vector3f(1, 0, 0);
        units::angle::degree_t angle(90.0f);
        Matrix44f M = matrix::rotate(I, angle, Vector3f::UNIT_Y);
        R = x * M;
        // std::cout << "Rotate: "
        //           << x << " * " << M << " = " << R
        //           << std::endl << std::endl;
        ASSERT_NEAR_VECTOR4(R, 0.0f, 0.0f, -1.0f, 0.0f, 0.0001f);

        R = R * M;
        R = R * M;
        R = R * M;
        // std::cout << "3 Rotates: "
        //           << x << " * " << M << " = " << R
        //           << std::endl << std::endl;
        ASSERT_NEAR_VECTOR4(R, 1.0f, 0.0f, 0.0f, 0.0f, 0.0001f);
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

void qq_lookAt(Vector3f const &position, Vector3f const &target, Vector3f const &up)
{
    std::cout << "\n\nQQ:" << std::endl;
    Matrix44f mat = matrix::lookAt(position, target, up);
    std::cout << "  qq::LookAt: " << mat << std::endl;

    Quatf orientation = Quatf::fromMatrix(mat).conjugate();
    std::cout << "  qq::quat: " << orientation << std::endl;
    std::cout << "  =>: " << orientation.angle() * 57.2958f << " angle: "
              << orientation.axis() << std::endl;

    // Camera::view
    Matrix44f I(matrix::Identity);
    Matrix44f trans = matrix::translate(I, -position);
    std::cout << "  qq::trans " << trans << std::endl;

    Matrix44f rot = orientation.toMatrix();
    std::cout << "  qq::rot " << rot << std::endl;

    Matrix44f view = trans * rot; // FIXME shall be rot * trans
    std::cout << "  qq::view " << view << std::endl;
    //std::cout << "  qq::view\n" << operator*(rot, trans) << std::endl;

    {
        Matrix44f tra({1, 0, 0, 0,
                       0, 1, 0, 0,
                       0, 0, 1, 0,
                       -1, 0, 0, 1});
        Matrix44f rot({0, 0, -1, 0,
                       0, 1, 0, 0,
                       1, 0, 0, 0,
                       0, 0, 0, 1});

        std::cout << rot * tra << std::endl;
    }
}


// g++ -W -Wall --std=c++14 -I/home/qq/MyGitHub/OpenGLCppWrapper/external/glm -I/home/qq/MyGitHub/OpenGLCppWrapper/external/units/include/ -I.. test_glm.cpp -o prog
int main()
{
    glm_lookAt(glm::vec3(1,0,0),
               glm::vec3(0,0,0),
               glm::vec3(0,1,0));

    qq_lookAt(Vector3f(1,0,0),
              Vector3f(0,0,0),
              Vector3f(0,1,0));

    return 0;
}
#endif
