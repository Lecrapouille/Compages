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
#include "Compages/Core/Matrix.hpp"


#pragma GCC diagnostic pop
#undef protected
#undef private


//--------------------------------------------------------------------------
#define ASSERT_ARR_FLOATS_NEARLY_EQ(expected, actual, size, thresh) \
    for (size_t idx = 0; idx < size; ++idx)                         \
    {                                                               \
        ASSERT_NEAR(expected[idx], actual[idx], thresh)             \
            << "at index: " << idx;                                 \
    }

//--------------------------------------------------------------------------
#define ASSERT_VECT_FLOATS_NEARLY_EQ(expected, actual, thresh)           \
    ASSERT_EQ(expected.size(), actual.size()) << "Array sizes differ.";  \
    for (size_t idx = 0; idx < std::min(expected.size(), actual.size()); \
         ++idx)                                                          \
    {                                                                    \
        ASSERT_NEAR(expected[idx], actual[idx], thresh)                  \
            << "at index: " << idx;                                      \
    }

//--------------------------------------------------------------------------
TEST(TestMatrices, testConstructor)
{
    size_t rows = 0_z;
    size_t cols = 0_z;

    compages::core::Matrix44f A1;
    A1.size(rows, cols);
    ASSERT_EQ(rows, 4_z);
    ASSERT_EQ(cols, 4_z);

    compages::core::Matrix33g A2;
    A2.size(rows, cols);
    ASSERT_EQ(rows, 3_z);
    ASSERT_EQ(cols, 3_z);

    compages::core::Matrix32i A3;
    A3.size(rows, cols);
    ASSERT_EQ(rows, 3_z);
    ASSERT_EQ(cols, 2_z);

    compages::core::Matrix23i A4;
    A4.size(rows, cols);
    ASSERT_EQ(rows, 2_z);
    ASSERT_EQ(cols, 3_z);

    compages::core::Matrix33f A5(
        0); // FIXME .data() should be implicit but this is not the case
    ASSERT_THAT(A5[0].data(), ElementsAre(0.0f, 0.0f, 0.0f));
    ASSERT_THAT(A5[1].data(), ElementsAre(0.0f, 0.0f, 0.0f));
    ASSERT_THAT(A5[2].data(), ElementsAre(0.0f, 0.0f, 0.0f));

    compages::core::Matrix44i A6(42);
    ASSERT_THAT(A6[0].data(), ElementsAre(42, 42, 42, 42));
    ASSERT_THAT(A6[1].data(), ElementsAre(42, 42, 42, 42));
    ASSERT_THAT(A6[2].data(), ElementsAre(42, 42, 42, 42));
    ASSERT_THAT(A6[3].data(), ElementsAre(42, 42, 42, 42));

    compages::core::Matrix44f I1(compages::core::matrix::Identity);
    ASSERT_THAT(I1[0].data(), ElementsAre(1.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(I1[1].data(), ElementsAre(0.0f, 1.0f, 0.0f, 0.0f));
    ASSERT_THAT(I1[2].data(), ElementsAre(0.0f, 0.0f, 1.0f, 0.0f));
    ASSERT_THAT(I1[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 1.0f));

    compages::core::Matrix33i I2(compages::core::matrix::Identity);
    ASSERT_THAT(I2[0].data(), ElementsAre(1, 0, 0));
    ASSERT_THAT(I2[1].data(), ElementsAre(0, 1, 0));
    ASSERT_THAT(I2[2].data(), ElementsAre(0, 0, 1));

    compages::core::Matrix44i O0(compages::core::matrix::Zero);
    ASSERT_THAT(O0[0].data(), ElementsAre(0, 0, 0, 0));
    ASSERT_THAT(O0[1].data(), ElementsAre(0, 0, 0, 0));
    ASSERT_THAT(O0[2].data(), ElementsAre(0, 0, 0, 0));
    ASSERT_THAT(O0[3].data(), ElementsAre(0, 0, 0, 0));

    compages::core::Matrix33f O1(compages::core::matrix::Zero);
    ASSERT_THAT(O1[0].data(), ElementsAre(0.0f, 0.0f, 0.0f));
    ASSERT_THAT(O1[1].data(), ElementsAre(0.0f, 0.0f, 0.0f));
    ASSERT_THAT(O1[2].data(), ElementsAre(0.0f, 0.0f, 0.0f));

    compages::core::Matrix22i O2(compages::core::matrix::One);
    ASSERT_THAT(O2[0].data(), ElementsAre(1, 1));
    ASSERT_THAT(O2[1].data(), ElementsAre(1, 1));

    compages::core::Matrix33f O3(compages::core::matrix::One);
    ASSERT_THAT(O3[0].data(), ElementsAre(1.0f, 1.0f, 1.0f));
    ASSERT_THAT(O3[1].data(), ElementsAre(1.0f, 1.0f, 1.0f));
    ASSERT_THAT(O3[2].data(), ElementsAre(1.0f, 1.0f, 1.0f));

    compages::core::Matrix44f B1(O3);
    ASSERT_THAT(B1[0].data(), ElementsAre(1.0f, 1.0f, 1.0f, 0.0f));
    ASSERT_THAT(B1[1].data(), ElementsAre(1.0f, 1.0f, 1.0f, 0.0f));
    ASSERT_THAT(B1[2].data(), ElementsAre(1.0f, 1.0f, 1.0f, 0.0f));
    ASSERT_THAT(B1[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));

    compages::core::Matrix44f B2({ 1.0f,
                   2.0f,
                   3.0f,
                   4.0f,
                   5.0f,
                   6.0f,
                   7.0f,
                   8.0f,
                   9.0f,
                   10.0f,
                   11.0f,
                   12.0f,
                   13.0f,
                   14.0f,
                   15.0f,
                   16.0f });
    ASSERT_THAT(B2[0].data(), ElementsAre(1.0f, 2.0f, 3.0f, 4.0f));
    ASSERT_THAT(B2[1].data(), ElementsAre(5.0f, 6.0f, 7.0f, 8.0f));
    ASSERT_THAT(B2[2].data(), ElementsAre(9.0f, 10.0f, 11.0f, 12.0f));
    ASSERT_THAT(B2[3].data(), ElementsAre(13.0f, 14.0f, 15.0f, 16.0f));

    compages::core::Matrix33f B3({ 1.0f,
                   2.0f,
                   3.0f,
                   4.0f,
                   5.0f,
                   6.0f,
                   7.0f,
                   8.0f,
                   9.0f,
                   10.0f,
                   11.0f,
                   12.0f,
                   13.0f,
                   14.0f,
                   15.0f,
                   16.0f });
    ASSERT_THAT(B3[0].data(), ElementsAre(1.0f, 2.0f, 3.0f));
    ASSERT_THAT(B3[1].data(), ElementsAre(4.0f, 5.0f, 6.0f));
    ASSERT_THAT(B3[2].data(), ElementsAre(7.0f, 8.0f, 9.0f));

    compages::core::Matrix44f B4({ 1.0f, 2.0f, 3.0f, 4.0f });
    ASSERT_THAT(B4[0].data(), ElementsAre(1.0f, 2.0f, 3.0f, 4.0f));
    ASSERT_THAT(B4[1].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(B4[2].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(B4[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));

    compages::core::Matrix44f B5({ 1.0f, 2.0f, 3.0f, 4.0f }, 42.0f);
    ASSERT_THAT(B5[0].data(), ElementsAre(1.0f, 2.0f, 3.0f, 4.0f));
    ASSERT_THAT(B5[1].data(), ElementsAre(42.0f, 42.0f, 42.0f, 42.0f));
    ASSERT_THAT(B5[2].data(), ElementsAre(42.0f, 42.0f, 42.0f, 42.0f));
    ASSERT_THAT(B5[3].data(), ElementsAre(42.0f, 42.0f, 42.0f, 42.0f));

    compages::core::Matrix33f C1 = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
    ASSERT_THAT(C1[0].data(), ElementsAre(1.0f, 2.0f, 3.0f));
    ASSERT_THAT(C1[1].data(), ElementsAre(4.0f, 5.0f, 6.0f));
    ASSERT_THAT(C1[2].data(), ElementsAre(7.0f, 8.0f, 9.0f));

    compages::core::Matrix33f C2 = C1;
    ASSERT_THAT(C2[0].data(), ElementsAre(1.0f, 2.0f, 3.0f));
    ASSERT_THAT(C2[1].data(), ElementsAre(4.0f, 5.0f, 6.0f));
    ASSERT_THAT(C2[2].data(), ElementsAre(7.0f, 8.0f, 9.0f));

    compages::core::Matrix22f C3(C1);
    ASSERT_THAT(C3[0].data(), ElementsAre(1.0f, 2.0f));
    ASSERT_THAT(C3[1].data(), ElementsAre(4.0f, 5.0f));

    compages::core::Matrix44f C4(C1);
    ASSERT_THAT(C4[0].data(), ElementsAre(1.0f, 2.0f, 3.0f, 0.0f));
    ASSERT_THAT(C4[1].data(), ElementsAre(4.0f, 5.0f, 6.0f, 0.0f));
    ASSERT_THAT(C4[2].data(), ElementsAre(7.0f, 8.0f, 9.0f, 0.0f));
    ASSERT_THAT(C4[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));

    compages::core::Matrix44f D;
    compages::core::identity(D);
    ASSERT_THAT(D[0].data(), ElementsAre(1.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(D[1].data(), ElementsAre(0.0f, 1.0f, 0.0f, 0.0f));
    ASSERT_THAT(D[2].data(), ElementsAre(0.0f, 0.0f, 1.0f, 0.0f));
    ASSERT_THAT(D[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 1.0f));
}

//--------------------------------------------------------------------------
TEST(TestMatrices, testPrint)
{
    compages::core::Matrix44f A(compages::core::matrix::Identity);

    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    std::cout << A << std::endl;
    std::cout.rdbuf(old);

    ASSERT_THAT(buffer.str().c_str(),
                HasSubstr("[1 0 0 0; 0 1 0 0; 0 0 1 0; 0 0 0 1]"));
}

//--------------------------------------------------------------------------
TEST(TestMatrices, testSwapRows)
{
    compages::core::Matrix33f M = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };

    ASSERT_THAT(M[0].data(), ElementsAre(1.0f, 2.0f, 3.0f));
    ASSERT_THAT(M[1].data(), ElementsAre(4.0f, 5.0f, 6.0f));
    ASSERT_THAT(M[2].data(), ElementsAre(7.0f, 8.0f, 9.0f));

    ASSERT_EQ(true, compages::core::swapRows(M, 0U, 2U));
    ASSERT_THAT(M[0].data(), ElementsAre(7.0f, 8.0f, 9.0f));
    ASSERT_THAT(M[1].data(), ElementsAre(4.0f, 5.0f, 6.0f));
    ASSERT_THAT(M[2].data(), ElementsAre(1.0f, 2.0f, 3.0f));

    ASSERT_EQ(true, compages::core::swapRows(M, 0U, 0U));
    ASSERT_THAT(M[0].data(), ElementsAre(7.0f, 8.0f, 9.0f));
    ASSERT_THAT(M[1].data(), ElementsAre(4.0f, 5.0f, 6.0f));
    ASSERT_THAT(M[2].data(), ElementsAre(1.0f, 2.0f, 3.0f));

    ASSERT_EQ(true, compages::core::swapRows(M, 10U, 10U));
    ASSERT_THAT(M[0].data(), ElementsAre(7.0f, 8.0f, 9.0f));
    ASSERT_THAT(M[1].data(), ElementsAre(4.0f, 5.0f, 6.0f));
    ASSERT_THAT(M[2].data(), ElementsAre(1.0f, 2.0f, 3.0f));

    ASSERT_EQ(false, compages::core::swapRows(M, 0U, 10U));
    ASSERT_THAT(M[0].data(), ElementsAre(7.0f, 8.0f, 9.0f));
    ASSERT_THAT(M[1].data(), ElementsAre(4.0f, 5.0f, 6.0f));
    ASSERT_THAT(M[2].data(), ElementsAre(1.0f, 2.0f, 3.0f));
}

//--------------------------------------------------------------------------
TEST(TestMatrices, testSwapMatrices)
{
    // TODO
}

//--------------------------------------------------------------------------
TEST(TestMatrices, testComparaisons)
{
    compages::core::Matrix44f one(1.0f);
    compages::core::Matrix44f two(2.0f);

    // Operator <
    {
        compages::core::Matrix44b A = (one < two);
        compages::core::Matrix44b B = (two < one);
        compages::core::Matrix44b C = (one < one);

        ASSERT_THAT(A[0].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(A[1].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(A[2].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(A[3].data(), ElementsAre(true, true, true, true));

        ASSERT_THAT(B[0].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(B[1].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(B[2].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(B[3].data(), ElementsAre(false, false, false, false));

        ASSERT_THAT(C[0].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(C[1].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(C[2].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(C[3].data(), ElementsAre(false, false, false, false));
    }

    // Operator >
    {
        compages::core::Matrix44b A = (one > two);
        compages::core::Matrix44b B = (two > one);
        compages::core::Matrix44b C = (one > one);

        ASSERT_THAT(A[0].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(A[1].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(A[2].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(A[3].data(), ElementsAre(false, false, false, false));

        ASSERT_THAT(B[0].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(B[1].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(B[2].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(B[3].data(), ElementsAre(true, true, true, true));

        ASSERT_THAT(C[0].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(C[1].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(C[2].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(C[3].data(), ElementsAre(false, false, false, false));
    }

    // Operator <=
    {
        compages::core::Matrix44b A = (one <= two);
        compages::core::Matrix44b B = (two <= one);
        compages::core::Matrix44b C = (one <= one);

        ASSERT_THAT(A[0].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(A[1].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(A[2].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(A[3].data(), ElementsAre(true, true, true, true));

        ASSERT_THAT(B[0].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(B[1].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(B[2].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(B[3].data(), ElementsAre(false, false, false, false));

        ASSERT_THAT(C[0].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(C[1].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(C[2].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(C[3].data(), ElementsAre(true, true, true, true));
    }

    // Operator >=
    {
        compages::core::Matrix44b A = (one >= two);
        compages::core::Matrix44b B = (two >= one);
        compages::core::Matrix44b C = (one >= one);

        ASSERT_THAT(A[0].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(A[1].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(A[2].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(A[3].data(), ElementsAre(false, false, false, false));

        ASSERT_THAT(B[0].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(B[1].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(B[2].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(B[3].data(), ElementsAre(true, true, true, true));

        ASSERT_THAT(C[0].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(C[1].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(C[2].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(C[3].data(), ElementsAre(true, true, true, true));
    }

    // Operator ==
    {
        compages::core::Matrix44b A = (one == two);
        compages::core::Matrix44b B = (two == one);
        compages::core::Matrix44b C = (one == one);

        ASSERT_THAT(A[0].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(A[1].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(A[2].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(A[3].data(), ElementsAre(false, false, false, false));

        ASSERT_THAT(B[0].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(B[1].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(B[2].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(B[3].data(), ElementsAre(false, false, false, false));

        ASSERT_THAT(C[0].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(C[1].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(C[2].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(C[3].data(), ElementsAre(true, true, true, true));
    }

    // Operator !=
    {
        compages::core::Matrix44b A = (one != two);
        compages::core::Matrix44b B = (two != one);
        compages::core::Matrix44b C = (one != one);

        ASSERT_THAT(A[0].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(A[1].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(A[2].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(A[3].data(), ElementsAre(true, true, true, true));

        ASSERT_THAT(B[0].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(B[1].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(B[2].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(B[3].data(), ElementsAre(true, true, true, true));

        ASSERT_THAT(C[0].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(C[1].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(C[2].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(C[3].data(), ElementsAre(false, false, false, false));
    }

    // Operator !
    {
        compages::core::Matrix44b A = (one != two);
        compages::core::Matrix44b B = !A;
        compages::core::Matrix44b C = !B;

        ASSERT_THAT(A[0].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(A[1].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(A[2].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(A[3].data(), ElementsAre(true, true, true, true));

        ASSERT_THAT(B[0].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(B[1].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(B[2].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(B[3].data(), ElementsAre(false, false, false, false));

        ASSERT_THAT(C[0].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(C[1].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(C[2].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(C[3].data(), ElementsAre(true, true, true, true));
    }

    // Operator &
    {
        compages::core::Matrix44b A(true);
        compages::core::Matrix44b B(false);
        compages::core::Matrix44b C = A & B;
        compages::core::Matrix44b D = A | B;
        compages::core::Matrix44b E = A ^ true;
        compages::core::Matrix44b F = true ^ B;

        ASSERT_THAT(A[0].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(A[1].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(A[2].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(A[3].data(), ElementsAre(true, true, true, true));

        ASSERT_THAT(B[0].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(B[1].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(B[2].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(B[3].data(), ElementsAre(false, false, false, false));

        ASSERT_THAT(C[0].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(C[1].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(C[2].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(C[3].data(), ElementsAre(false, false, false, false));

        ASSERT_THAT(D[0].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(D[1].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(D[2].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(D[3].data(), ElementsAre(true, true, true, true));

        ASSERT_THAT(E[0].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(E[1].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(E[2].data(), ElementsAre(false, false, false, false));
        ASSERT_THAT(E[3].data(), ElementsAre(false, false, false, false));

        ASSERT_THAT(F[0].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(F[1].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(F[2].data(), ElementsAre(true, true, true, true));
        ASSERT_THAT(F[3].data(), ElementsAre(true, true, true, true));
    }
}

//--------------------------------------------------------------------------
TEST(TestMatrices, testProduct)
{
    compages::core::Matrix44f A({ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1 });
    compages::core::Matrix44f B({ 0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1 });

    compages::core::Matrix44f C = A * B;
    ASSERT_THAT(C[0].data(), ElementsAre(0.0f, 0.0f, -1.0f, 0.0f));
    ASSERT_THAT(C[1].data(), ElementsAre(0.0f, 1.0f, 0.0f, 0.0f));
    ASSERT_THAT(C[2].data(), ElementsAre(1.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(C[3].data(), ElementsAre(0.0f, 0.0f, -1.0f, 1.0f));

    compages::core::Matrix44f D = B * A;
    ASSERT_THAT(D[0].data(), ElementsAre(0.0f, 0.0f, -1.0f, 0.0f));
    ASSERT_THAT(D[1].data(), ElementsAre(0.0f, 1.0f, 0.0f, 0.0f));
    ASSERT_THAT(D[2].data(), ElementsAre(1.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(D[3].data(), ElementsAre(1.0f, 0.0f, 0.0f, 1.0f));

    compages::core::Matrix44f E1 = compages::core::hadamard(A, B);
    ASSERT_THAT(E1[0].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(E1[1].data(), ElementsAre(0.0f, 1.0f, 0.0f, 0.0f));
    ASSERT_THAT(E1[2].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(E1[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 1.0f));

    compages::core::Matrix44f E2 = compages::core::hadamard(B, A);
    ASSERT_THAT(E2[0].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(E2[1].data(), ElementsAre(0.0f, 1.0f, 0.0f, 0.0f));
    ASSERT_THAT(E2[2].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(E2[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 1.0f));

    ASSERT_THAT(compages::core::isDiagonal(A), false);
    ASSERT_THAT(compages::core::isDiagonal(E1), true);
    ASSERT_THAT(compages::core::isDiagonal(E2), true);
}

//--------------------------------------------------------------------------
TEST(TestMatrices, testArithmetic)
{
    compages::core::Matrix44f A({ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1 });
    compages::core::Matrix44f B({ 0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1 });

    compages::core::Matrix44f C1 = A + B;
    ASSERT_THAT(C1[0].data(), ElementsAre(1.0f, 0.0f, -1.0f, 0.0f));
    ASSERT_THAT(C1[1].data(), ElementsAre(0.0f, 2.0f, 0.0f, 0.0f));
    ASSERT_THAT(C1[2].data(), ElementsAre(1.0f, 0.0f, 1.0f, 0.0f));
    ASSERT_THAT(C1[3].data(), ElementsAre(1.0f, 0.0f, 0.0f, 2.0f));

    compages::core::Matrix44f C2 = B + A;
    ASSERT_THAT(C2[0].data(), ElementsAre(1.0f, 0.0f, -1.0f, 0.0f));
    ASSERT_THAT(C2[1].data(), ElementsAre(0.0f, 2.0f, 0.0f, 0.0f));
    ASSERT_THAT(C2[2].data(), ElementsAre(1.0f, 0.0f, 1.0f, 0.0f));
    ASSERT_THAT(C2[3].data(), ElementsAre(1.0f, 0.0f, 0.0f, 2.0f));

    compages::core::Matrix44f C3 = compages::core::transpose(C2);
    ASSERT_THAT(C3[0].data(), ElementsAre(1.0f, 0.0f, 1.0f, 1.0f));
    ASSERT_THAT(C3[1].data(), ElementsAre(0.0f, 2.0f, 0.0f, 0.0f));
    ASSERT_THAT(C3[2].data(), ElementsAre(-1.0f, 0.0f, 1.0f, 0.0f));
    ASSERT_THAT(C3[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 2.0f));

    compages::core::Matrix44f C4 = compages::core::transpose(C3);
    ASSERT_THAT(C4[0].data(), ElementsAre(1.0f, 0.0f, -1.0f, 0.0f));
    ASSERT_THAT(C4[1].data(), ElementsAre(0.0f, 2.0f, 0.0f, 0.0f));
    ASSERT_THAT(C4[2].data(), ElementsAre(1.0f, 0.0f, 1.0f, 0.0f));
    ASSERT_THAT(C4[3].data(), ElementsAre(1.0f, 0.0f, 0.0f, 2.0f));

    compages::core::Matrix44f C5 = 42.0f * A * 3.0f + 4.0f * B * 6.0f;
    ASSERT_THAT(C5[0].data(), ElementsAre(126.0f, 0.0f, -24.0f, 0.0f));
    ASSERT_THAT(C5[1].data(), ElementsAre(0.0f, 150.0f, 0.0f, 0.0f));
    ASSERT_THAT(C5[2].data(), ElementsAre(24.0f, 0.0f, 126.0f, 0.0f));
    ASSERT_THAT(C5[3].data(), ElementsAre(126.0f, 0.0f, 0.0f, 150.0f));

    ASSERT_THAT(compages::core::trace(C5), 552.0f);
}

//--------------------------------------------------------------------------
TEST(TestMatrices, testWithVector)
{
    compages::core::Matrix22f A = { 1.0f, 2.0f, 3.0f, 4.0f };
    compages::core::Vector2f v = { 6.0f, 7.0f };

    // V is considered as column vector:
    //     |1 2|   |6|   |20|
    // B = |3 4| * |7| = |46|
    /*compages::core::Matrix<float, 1_z, 2_z>*/ compages::core::Vector2f B = A * v;
    ASSERT_EQ(B.size(), 2_z);
    ASSERT_THAT(B.data(), ElementsAre(20.0f, 46.0f));

    // v^T * A, written with the explicit transpose:
    //             |1 2|
    // C = |6 7| * |3 4| = |27 40|
    compages::core::Vector2f C = compages::core::transpose(A) * v;
    ASSERT_EQ(C.size(), 2_z);
    ASSERT_THAT(C.data(), ElementsAre(27.0f, 40.0f));

    // Same product with a real 1 x 2 row matrix.
    compages::core::Matrix<float, 1u, 2u> const R =
        compages::core::castToRowVector(v) * A;
    ASSERT_THAT(R.data(), ElementsAre(27.0f, 40.0f));
}

//--------------------------------------------------------------------------
TEST(TestMatrices, testSelfArithmetic)
{
    compages::core::Matrix44f A(1.0f);
    compages::core::Matrix44f B;

    //
    A *= 2.0f;
    ASSERT_THAT(A[0].data(), ElementsAre(2.0f, 2.0f, 2.0f, 2.0f));
    ASSERT_THAT(A[1].data(), ElementsAre(2.0f, 2.0f, 2.0f, 2.0f));
    ASSERT_THAT(A[2].data(), ElementsAre(2.0f, 2.0f, 2.0f, 2.0f));
    ASSERT_THAT(A[3].data(), ElementsAre(2.0f, 2.0f, 2.0f, 2.0f));

    //
    A /= 2.0f;
    ASSERT_THAT(A[0].data(), ElementsAre(1.0f, 1.0f, 1.0f, 1.0f));
    ASSERT_THAT(A[1].data(), ElementsAre(1.0f, 1.0f, 1.0f, 1.0f));
    ASSERT_THAT(A[2].data(), ElementsAre(1.0f, 1.0f, 1.0f, 1.0f));
    ASSERT_THAT(A[3].data(), ElementsAre(1.0f, 1.0f, 1.0f, 1.0f));

    //
    A -= 1.0f;
    ASSERT_THAT(A[0].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(A[1].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(A[2].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(A[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));

    //
    A += 1.0f;
    ASSERT_THAT(A[0].data(), ElementsAre(1.0f, 1.0f, 1.0f, 1.0f));
    ASSERT_THAT(A[1].data(), ElementsAre(1.0f, 1.0f, 1.0f, 1.0f));
    ASSERT_THAT(A[2].data(), ElementsAre(1.0f, 1.0f, 1.0f, 1.0f));
    ASSERT_THAT(A[3].data(), ElementsAre(1.0f, 1.0f, 1.0f, 1.0f));

    //
    B = -A;
    ASSERT_THAT(B[0].data(), ElementsAre(-1.0f, -1.0f, -1.0f, -1.0f));
    ASSERT_THAT(B[1].data(), ElementsAre(-1.0f, -1.0f, -1.0f, -1.0f));
    ASSERT_THAT(B[2].data(), ElementsAre(-1.0f, -1.0f, -1.0f, -1.0f));
    ASSERT_THAT(B[3].data(), ElementsAre(-1.0f, -1.0f, -1.0f, -1.0f));
}

//--------------------------------------------------------------------------
TEST(TestMatrices, testOperations)
{
    compages::core::Matrix22f A(compages::core::matrix::Identity);
    compages::core::Matrix33f B(compages::core::matrix::Zero);
    compages::core::Matrix33f C({ 9.0f,
                  -36.0f,
                  30.0f,
                  -36.0f,
                  192.0f,
                  -180.0f,
                  30.0f,
                  -180.0f,
                  180.0f });
    compages::core::Matrix44f D(compages::core::matrix::One);
    compages::core::Matrix44f E({ 0.0f, 1.0f });
    compages::core::Matrix44g F({
        -0.5003796,
        0.1910551,
        -0.1043591,
        -0.3966362,
        1.1937458,
        -1.3189198,
        0.2973099,
        0.5163254,
        -1.5206395,
        0.9307226,
        0.5308515,
        0.0075659,
        1.8655072,
        -0.8575199,
        -1.5404673,
        1.0422456,
    });

    ASSERT_THAT(compages::core::isSymmetric(A), true);
    ASSERT_THAT(compages::core::isSymmetric(B), true);
    ASSERT_THAT(compages::core::isSymmetric(C), true);
    ASSERT_THAT(compages::core::isSymmetric(D), true);
    ASSERT_THAT(compages::core::isSymmetric(E), false);
    ASSERT_THAT(compages::core::isSymmetric(F), false);

    ASSERT_THAT(compages::core::determinant(A), 1.0f);
    ASSERT_THAT(compages::core::determinant(B), 0.0f);
    ASSERT_THAT(compages::core::determinant(C), 2160.0f);
    ASSERT_THAT(compages::core::determinant(D), 0.0f);
    ASSERT_THAT(compages::core::determinant(E), 0.0f);
    ASSERT_NEAR(compages::core::determinant(F), 0.732664, 10e-6);
}

//--------------------------------------------------------------------------
TEST(TestMatrices, testDecomposition)
{
    // Random matrix
    compages::core::Matrix44g Ra = {
        -0.5003796, 0.1910551,  -0.1043591, -0.3966362, 1.1937458, -1.3189198,
        0.2973099,  0.5163254,  -1.5206395, 0.9307226,  0.5308515, 0.0075659,
        1.8655072,  -0.8575199, -1.5404673, 1.0422456,
    };
    // LU decomposition of Ra. Expected result.
    compages::core::Matrix44g U = { 1.8655072, -0.8575199, -1.5404673, 1.0422456,
                    0.0,       -0.7701892, 1.2830613,  -0.1506119,
                    0.0,       0.0,        -0.5824496, -0.1094599,
                    0.0,       0.0,        0.0,        0.8754921 };
    // LU decomposition of Ra. Expected result.
    compages::core::Matrix44g L = {
        1.0,        0.0,       0.0, 0.0, 0.6399042,  1.0,        0.0,       0.0,
        -0.2682271, 0.0505785, 1.0, 0.0, -0.8151346, -0.3008722, 0.5816799, 1.0,
    };

    compages::core::Matrix44g LL(11111.0), UU(22222.0), P(333.3); // Init with random values
    compages::core::LUdecomposition(Ra, LL, UU, P);
    ASSERT_ARR_FLOATS_NEARLY_EQ(LL.data(), L.data(), 16u, 0.001);
    ASSERT_ARR_FLOATS_NEARLY_EQ(UU.data(), U.data(), 16u, 0.001);

    compages::core::Vector3g a(3.0f, -24.0f, 30.0f);
    compages::core::Matrix33g B = { 9.0f,    -36.0f, 30.0f,   -36.0f, 192.0f,
                    -180.0f, 30.0f,  -180.0f, 180.0f };

    // ScicosLab: x = B \ a
    compages::core::Vector3g x(compages::core::LUsolve(B, a));
    compages::core::Vector3g Z(B * x - a);

    ASSERT_THAT(x.data(), ElementsAre(1.0, 1.0, 1.0)); // Close to 1
    ASSERT_THAT(Z.data(), ElementsAre(0.0, 0.0, 0.0)); // Close to 0
}

//--------------------------------------------------------------------------
TEST(TestMatrices, testInverse)
{
    compages::core::Matrix44g A = {
        -1.0, 0.0, 1.0,  1.0, 1.0, -2.0, 1.0, -1.0,
        1.0,  0.0, -1.0, 1.0, 1.0, 0.0,  1.0, -1.0,
    };

    compages::core::Matrix44g B = compages::core::inverse(A);

    ASSERT_THAT(B[0].data(), ElementsAre(0.0, 0.0, 0.5, 0.5));
    ASSERT_THAT(B[1].data(), ElementsAre(0.0, -0.5, 0.0, 0.5));
    ASSERT_THAT(B[2].data(), ElementsAre(0.5, 0.0, 0.0, 0.5));
    ASSERT_THAT(B[3].data(), ElementsAre(0.5, 0.0, 0.5, 0.0));
}
