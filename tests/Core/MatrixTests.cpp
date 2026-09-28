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
#include "Compages/Core/Matrix.hpp"
#pragma GCC diagnostic pop
#undef protected
#undef private

#define ASSERT_ARR_FLOATS_NEARLY_EQ(expected, actual, size, thresh) \
    for (size_t idx = 0; idx < size; ++idx)                         \
    {                                                               \
        ASSERT_NEAR(expected[idx], actual[idx], thresh)             \
            << "at index: " << idx;                                 \
    }

#define ASSERT_VECT_FLOATS_NEARLY_EQ(expected, actual, thresh)           \
    ASSERT_EQ(expected.size(), actual.size()) << "Array sizes differ.";  \
    for (size_t idx = 0; idx < std::min(expected.size(), actual.size()); \
         ++idx)                                                          \
    {                                                                    \
        ASSERT_NEAR(expected[idx], actual[idx], thresh)                  \
            << "at index: " << idx;                                      \
    }

TEST(TestMatrices, testConstructor)
{
    size_t rows = 0_z;
    size_t cols = 0_z;

    Matrix44f A1;
    A1.size(rows, cols);
    ASSERT_EQ(rows, 4_z);
    ASSERT_EQ(cols, 4_z);

    Matrix33g A2;
    A2.size(rows, cols);
    ASSERT_EQ(rows, 3_z);
    ASSERT_EQ(cols, 3_z);

    Matrix32i A3;
    A3.size(rows, cols);
    ASSERT_EQ(rows, 3_z);
    ASSERT_EQ(cols, 2_z);

    Matrix23i A4;
    A4.size(rows, cols);
    ASSERT_EQ(rows, 2_z);
    ASSERT_EQ(cols, 3_z);

    Matrix33f A5(
        0); // FIXME .data() should be implicit but this is not the case
    ASSERT_THAT(A5[0].data(), ElementsAre(0.0f, 0.0f, 0.0f));
    ASSERT_THAT(A5[1].data(), ElementsAre(0.0f, 0.0f, 0.0f));
    ASSERT_THAT(A5[2].data(), ElementsAre(0.0f, 0.0f, 0.0f));

    Matrix44i A6(42);
    ASSERT_THAT(A6[0].data(), ElementsAre(42, 42, 42, 42));
    ASSERT_THAT(A6[1].data(), ElementsAre(42, 42, 42, 42));
    ASSERT_THAT(A6[2].data(), ElementsAre(42, 42, 42, 42));
    ASSERT_THAT(A6[3].data(), ElementsAre(42, 42, 42, 42));

    Matrix44f I1(compages::matrix::Identity);
    ASSERT_THAT(I1[0].data(), ElementsAre(1.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(I1[1].data(), ElementsAre(0.0f, 1.0f, 0.0f, 0.0f));
    ASSERT_THAT(I1[2].data(), ElementsAre(0.0f, 0.0f, 1.0f, 0.0f));
    ASSERT_THAT(I1[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 1.0f));

    Matrix33i I2(compages::matrix::Identity);
    ASSERT_THAT(I2[0].data(), ElementsAre(1, 0, 0));
    ASSERT_THAT(I2[1].data(), ElementsAre(0, 1, 0));
    ASSERT_THAT(I2[2].data(), ElementsAre(0, 0, 1));

    Matrix44i O0(compages::matrix::Zero);
    ASSERT_THAT(O0[0].data(), ElementsAre(0, 0, 0, 0));
    ASSERT_THAT(O0[1].data(), ElementsAre(0, 0, 0, 0));
    ASSERT_THAT(O0[2].data(), ElementsAre(0, 0, 0, 0));
    ASSERT_THAT(O0[3].data(), ElementsAre(0, 0, 0, 0));

    Matrix33f O1(compages::matrix::Zero);
    ASSERT_THAT(O1[0].data(), ElementsAre(0.0f, 0.0f, 0.0f));
    ASSERT_THAT(O1[1].data(), ElementsAre(0.0f, 0.0f, 0.0f));
    ASSERT_THAT(O1[2].data(), ElementsAre(0.0f, 0.0f, 0.0f));

    Matrix22i O2(compages::matrix::One);
    ASSERT_THAT(O2[0].data(), ElementsAre(1, 1));
    ASSERT_THAT(O2[1].data(), ElementsAre(1, 1));

    Matrix33f O3(compages::matrix::One);
    ASSERT_THAT(O3[0].data(), ElementsAre(1.0f, 1.0f, 1.0f));
    ASSERT_THAT(O3[1].data(), ElementsAre(1.0f, 1.0f, 1.0f));
    ASSERT_THAT(O3[2].data(), ElementsAre(1.0f, 1.0f, 1.0f));

    Matrix44f B1(O3);
    ASSERT_THAT(B1[0].data(), ElementsAre(1.0f, 1.0f, 1.0f, 0.0f));
    ASSERT_THAT(B1[1].data(), ElementsAre(1.0f, 1.0f, 1.0f, 0.0f));
    ASSERT_THAT(B1[2].data(), ElementsAre(1.0f, 1.0f, 1.0f, 0.0f));
    ASSERT_THAT(B1[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));

    Matrix44f B2({ 1.0f,
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

    Matrix33f B3({ 1.0f,
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

    Matrix44f B4({ 1.0f, 2.0f, 3.0f, 4.0f });
    ASSERT_THAT(B4[0].data(), ElementsAre(1.0f, 2.0f, 3.0f, 4.0f));
    ASSERT_THAT(B4[1].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(B4[2].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(B4[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));

    Matrix44f B5({ 1.0f, 2.0f, 3.0f, 4.0f }, 42.0f);
    ASSERT_THAT(B5[0].data(), ElementsAre(1.0f, 2.0f, 3.0f, 4.0f));
    ASSERT_THAT(B5[1].data(), ElementsAre(42.0f, 42.0f, 42.0f, 42.0f));
    ASSERT_THAT(B5[2].data(), ElementsAre(42.0f, 42.0f, 42.0f, 42.0f));
    ASSERT_THAT(B5[3].data(), ElementsAre(42.0f, 42.0f, 42.0f, 42.0f));

    Matrix33f C1 = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
    ASSERT_THAT(C1[0].data(), ElementsAre(1.0f, 2.0f, 3.0f));
    ASSERT_THAT(C1[1].data(), ElementsAre(4.0f, 5.0f, 6.0f));
    ASSERT_THAT(C1[2].data(), ElementsAre(7.0f, 8.0f, 9.0f));

    Matrix33f C2 = C1;
    ASSERT_THAT(C2[0].data(), ElementsAre(1.0f, 2.0f, 3.0f));
    ASSERT_THAT(C2[1].data(), ElementsAre(4.0f, 5.0f, 6.0f));
    ASSERT_THAT(C2[2].data(), ElementsAre(7.0f, 8.0f, 9.0f));

    Matrix22f C3(C1);
    ASSERT_THAT(C3[0].data(), ElementsAre(1.0f, 2.0f));
    ASSERT_THAT(C3[1].data(), ElementsAre(4.0f, 5.0f));

    Matrix44f C4(C1);
    ASSERT_THAT(C4[0].data(), ElementsAre(1.0f, 2.0f, 3.0f, 0.0f));
    ASSERT_THAT(C4[1].data(), ElementsAre(4.0f, 5.0f, 6.0f, 0.0f));
    ASSERT_THAT(C4[2].data(), ElementsAre(7.0f, 8.0f, 9.0f, 0.0f));
    ASSERT_THAT(C4[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));

    Matrix44f D;
    compages::matrix::identity(D);
    ASSERT_THAT(D[0].data(), ElementsAre(1.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(D[1].data(), ElementsAre(0.0f, 1.0f, 0.0f, 0.0f));
    ASSERT_THAT(D[2].data(), ElementsAre(0.0f, 0.0f, 1.0f, 0.0f));
    ASSERT_THAT(D[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 1.0f));
}

TEST(TestMatrices, testPrint)
{
    Matrix44f A(compages::matrix::Identity);

    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    std::cout << A << std::endl;
    std::cout.rdbuf(old);

    ASSERT_THAT(buffer.str().c_str(),
                HasSubstr("[1 0 0 0; 0 1 0 0; 0 0 1 0; 0 0 0 1]"));
}

TEST(TestMatrices, testSwapRows)
{
    Matrix33f M = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };

    ASSERT_THAT(M[0].data(), ElementsAre(1.0f, 2.0f, 3.0f));
    ASSERT_THAT(M[1].data(), ElementsAre(4.0f, 5.0f, 6.0f));
    ASSERT_THAT(M[2].data(), ElementsAre(7.0f, 8.0f, 9.0f));

    ASSERT_EQ(true, compages::matrix::swapRows(M, 0U, 2U));
    ASSERT_THAT(M[0].data(), ElementsAre(7.0f, 8.0f, 9.0f));
    ASSERT_THAT(M[1].data(), ElementsAre(4.0f, 5.0f, 6.0f));
    ASSERT_THAT(M[2].data(), ElementsAre(1.0f, 2.0f, 3.0f));

    ASSERT_EQ(true, compages::matrix::swapRows(M, 0U, 0U));
    ASSERT_THAT(M[0].data(), ElementsAre(7.0f, 8.0f, 9.0f));
    ASSERT_THAT(M[1].data(), ElementsAre(4.0f, 5.0f, 6.0f));
    ASSERT_THAT(M[2].data(), ElementsAre(1.0f, 2.0f, 3.0f));

    ASSERT_EQ(true, compages::matrix::swapRows(M, 10U, 10U));
    ASSERT_THAT(M[0].data(), ElementsAre(7.0f, 8.0f, 9.0f));
    ASSERT_THAT(M[1].data(), ElementsAre(4.0f, 5.0f, 6.0f));
    ASSERT_THAT(M[2].data(), ElementsAre(1.0f, 2.0f, 3.0f));

    ASSERT_EQ(false, compages::matrix::swapRows(M, 0U, 10U));
    ASSERT_THAT(M[0].data(), ElementsAre(7.0f, 8.0f, 9.0f));
    ASSERT_THAT(M[1].data(), ElementsAre(4.0f, 5.0f, 6.0f));
    ASSERT_THAT(M[2].data(), ElementsAre(1.0f, 2.0f, 3.0f));
}

TEST(TestMatrices, testSwapMatrices)
{
    // TODO
}

TEST(TestMatrices, testComparaisons)
{
    Matrix44f one(1.0f);
    Matrix44f two(2.0f);

    // Operator <
    {
        Matrix44b A = (one < two);
        Matrix44b B = (two < one);
        Matrix44b C = (one < one);

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
        Matrix44b A = (one > two);
        Matrix44b B = (two > one);
        Matrix44b C = (one > one);

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
        Matrix44b A = (one <= two);
        Matrix44b B = (two <= one);
        Matrix44b C = (one <= one);

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
        Matrix44b A = (one >= two);
        Matrix44b B = (two >= one);
        Matrix44b C = (one >= one);

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
        Matrix44b A = (one == two);
        Matrix44b B = (two == one);
        Matrix44b C = (one == one);

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
        Matrix44b A = (one != two);
        Matrix44b B = (two != one);
        Matrix44b C = (one != one);

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
        Matrix44b A = (one != two);
        Matrix44b B = !A;
        Matrix44b C = !B;

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
        Matrix44b A(true);
        Matrix44b B(false);
        Matrix44b C = A & B;
        Matrix44b D = A | B;
        Matrix44b E = A ^ true;
        Matrix44b F = true ^ B;

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

TEST(TestMatrices, testProduct)
{
    Matrix44f A({ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1 });
    Matrix44f B({ 0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1 });

    Matrix44f C = A * B;
    ASSERT_THAT(C[0].data(), ElementsAre(0.0f, 0.0f, -1.0f, 0.0f));
    ASSERT_THAT(C[1].data(), ElementsAre(0.0f, 1.0f, 0.0f, 0.0f));
    ASSERT_THAT(C[2].data(), ElementsAre(1.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(C[3].data(), ElementsAre(0.0f, 0.0f, -1.0f, 1.0f));

    Matrix44f D = B * A;
    ASSERT_THAT(D[0].data(), ElementsAre(0.0f, 0.0f, -1.0f, 0.0f));
    ASSERT_THAT(D[1].data(), ElementsAre(0.0f, 1.0f, 0.0f, 0.0f));
    ASSERT_THAT(D[2].data(), ElementsAre(1.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(D[3].data(), ElementsAre(1.0f, 0.0f, 0.0f, 1.0f));

    Matrix44f E1 = compages::matrix::hadamard(A, B);
    ASSERT_THAT(E1[0].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(E1[1].data(), ElementsAre(0.0f, 1.0f, 0.0f, 0.0f));
    ASSERT_THAT(E1[2].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(E1[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 1.0f));

    Matrix44f E2 = compages::matrix::hadamard(B, A);
    ASSERT_THAT(E2[0].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(E2[1].data(), ElementsAre(0.0f, 1.0f, 0.0f, 0.0f));
    ASSERT_THAT(E2[2].data(), ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_THAT(E2[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 1.0f));

    ASSERT_THAT(compages::matrix::isDiagonal(A), false);
    ASSERT_THAT(compages::matrix::isDiagonal(E1), true);
    ASSERT_THAT(compages::matrix::isDiagonal(E2), true);
}

TEST(TestMatrices, testArithmetic)
{
    Matrix44f A({ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1 });
    Matrix44f B({ 0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1 });

    Matrix44f C1 = A + B;
    ASSERT_THAT(C1[0].data(), ElementsAre(1.0f, 0.0f, -1.0f, 0.0f));
    ASSERT_THAT(C1[1].data(), ElementsAre(0.0f, 2.0f, 0.0f, 0.0f));
    ASSERT_THAT(C1[2].data(), ElementsAre(1.0f, 0.0f, 1.0f, 0.0f));
    ASSERT_THAT(C1[3].data(), ElementsAre(1.0f, 0.0f, 0.0f, 2.0f));

    Matrix44f C2 = B + A;
    ASSERT_THAT(C2[0].data(), ElementsAre(1.0f, 0.0f, -1.0f, 0.0f));
    ASSERT_THAT(C2[1].data(), ElementsAre(0.0f, 2.0f, 0.0f, 0.0f));
    ASSERT_THAT(C2[2].data(), ElementsAre(1.0f, 0.0f, 1.0f, 0.0f));
    ASSERT_THAT(C2[3].data(), ElementsAre(1.0f, 0.0f, 0.0f, 2.0f));

    Matrix44f C3 = compages::matrix::transpose(C2);
    ASSERT_THAT(C3[0].data(), ElementsAre(1.0f, 0.0f, 1.0f, 1.0f));
    ASSERT_THAT(C3[1].data(), ElementsAre(0.0f, 2.0f, 0.0f, 0.0f));
    ASSERT_THAT(C3[2].data(), ElementsAre(-1.0f, 0.0f, 1.0f, 0.0f));
    ASSERT_THAT(C3[3].data(), ElementsAre(0.0f, 0.0f, 0.0f, 2.0f));

    Matrix44f C4 = compages::matrix::transpose(C3);
    ASSERT_THAT(C4[0].data(), ElementsAre(1.0f, 0.0f, -1.0f, 0.0f));
    ASSERT_THAT(C4[1].data(), ElementsAre(0.0f, 2.0f, 0.0f, 0.0f));
    ASSERT_THAT(C4[2].data(), ElementsAre(1.0f, 0.0f, 1.0f, 0.0f));
    ASSERT_THAT(C4[3].data(), ElementsAre(1.0f, 0.0f, 0.0f, 2.0f));

    Matrix44f C5 = 42.0f * A * 3.0f + 4.0f * B * 6.0f;
    ASSERT_THAT(C5[0].data(), ElementsAre(126.0f, 0.0f, -24.0f, 0.0f));
    ASSERT_THAT(C5[1].data(), ElementsAre(0.0f, 150.0f, 0.0f, 0.0f));
    ASSERT_THAT(C5[2].data(), ElementsAre(24.0f, 0.0f, 126.0f, 0.0f));
    ASSERT_THAT(C5[3].data(), ElementsAre(126.0f, 0.0f, 0.0f, 150.0f));

    ASSERT_THAT(compages::matrix::trace(C5), 552.0f);
}

TEST(TestMatrices, testWithVector)
{
    Matrix22f A = { 1.0f, 2.0f, 3.0f, 4.0f };
    Vector2f v = { 6.0f, 7.0f };

    // V is considered as column vector:
    //     |1 2|   |6|   |20|
    // B = |3 4| * |7| = |46|
    /*Matrix<float, 1_z, 2_z>*/ Vector2f B = A * v;
    ASSERT_EQ(B.size(), 2_z);
    ASSERT_THAT(B.data(), ElementsAre(20.0f, 46.0f));

    // V is considered as row vector:
    //             |1 2|
    // C = |6 7| * |3 4| = |27 40|
    /*Matrix<float, 2_z, 1_z>*/ Vector2f C = v * A;
    ASSERT_EQ(C.size(), 2_z);
    ASSERT_THAT(C.data(), ElementsAre(27.0f, 40.0f));
}

TEST(TestMatrices, testSelfArithmetic)
{
    Matrix44f A(1.0f);
    Matrix44f B;

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

TEST(TestMatrices, testOperations)
{
    Matrix22f A(compages::matrix::Identity);
    Matrix33f B(compages::matrix::Zero);
    Matrix33f C({ 9.0f,
                  -36.0f,
                  30.0f,
                  -36.0f,
                  192.0f,
                  -180.0f,
                  30.0f,
                  -180.0f,
                  180.0f });
    Matrix44f D(compages::matrix::One);
    Matrix44f E({ 0.0f, 1.0f });
    Matrix44g F({
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

    ASSERT_THAT(compages::matrix::isSymmetric(A), true);
    ASSERT_THAT(compages::matrix::isSymmetric(B), true);
    ASSERT_THAT(compages::matrix::isSymmetric(C), true);
    ASSERT_THAT(compages::matrix::isSymmetric(D), true);
    ASSERT_THAT(compages::matrix::isSymmetric(E), false);
    ASSERT_THAT(compages::matrix::isSymmetric(F), false);

    ASSERT_THAT(compages::matrix::determinant(A), 1.0f);
    ASSERT_THAT(compages::matrix::determinant(B), 0.0f);
    ASSERT_THAT(compages::matrix::determinant(C), 2160.0f);
    ASSERT_THAT(compages::matrix::determinant(D), 0.0f);
    ASSERT_THAT(compages::matrix::determinant(E), 0.0f);
    ASSERT_NEAR(compages::matrix::determinant(F), 0.732664, 10e-6);
}

TEST(TestMatrices, testDecomposition)
{
    // Random matrix
    Matrix44g Ra = {
        -0.5003796, 0.1910551,  -0.1043591, -0.3966362, 1.1937458, -1.3189198,
        0.2973099,  0.5163254,  -1.5206395, 0.9307226,  0.5308515, 0.0075659,
        1.8655072,  -0.8575199, -1.5404673, 1.0422456,
    };
    // LU decomposition of Ra. Expected result.
    Matrix44g U = { 1.8655072, -0.8575199, -1.5404673, 1.0422456,
                    0.0,       -0.7701892, 1.2830613,  -0.1506119,
                    0.0,       0.0,        -0.5824496, -0.1094599,
                    0.0,       0.0,        0.0,        0.8754921 };
    // LU decomposition of Ra. Expected result.
    Matrix44g L = {
        1.0,        0.0,       0.0, 0.0, 0.6399042,  1.0,        0.0,       0.0,
        -0.2682271, 0.0505785, 1.0, 0.0, -0.8151346, -0.3008722, 0.5816799, 1.0,
    };

    Matrix44g LL(11111.0), UU(22222.0), P(333.3); // Init with random values
    compages::matrix::LUdecomposition(Ra, LL, UU, P);
    compages::maths::maxUlps = 1U;
    ASSERT_ARR_FLOATS_NEARLY_EQ(LL.data(), L.data(), 16u, 0.001);
    ASSERT_ARR_FLOATS_NEARLY_EQ(UU.data(), U.data(), 16u, 0.001);

    Vector3g a(3.0f, -24.0f, 30.0f);
    Matrix33g B = { 9.0f,    -36.0f, 30.0f,   -36.0f, 192.0f,
                    -180.0f, 30.0f,  -180.0f, 180.0f };

    // ScicosLab: x = a / B
    Vector3g x(compages::matrix::LUsolve(B, a));
    Vector3g Z(x * B - a);

    ASSERT_THAT(x.data(), ElementsAre(1.0, 1.0, 1.0)); // Close to 1
    ASSERT_THAT(Z.data(), ElementsAre(0.0, 0.0, 0.0)); // Close to 0
}

TEST(TestMatrices, testInverse)
{
    Matrix44g A = {
        -1.0, 0.0, 1.0,  1.0, 1.0, -2.0, 1.0, -1.0,
        1.0,  0.0, -1.0, 1.0, 1.0, 0.0,  1.0, -1.0,
    };

    Matrix44g B = compages::matrix::inverse(A);

    ASSERT_THAT(B[0].data(), ElementsAre(0.0, 0.0, 0.5, 0.5));
    ASSERT_THAT(B[1].data(), ElementsAre(0.0, -0.5, 0.0, 0.5));
    ASSERT_THAT(B[2].data(), ElementsAre(0.5, 0.0, 0.0, 0.5));
    ASSERT_THAT(B[3].data(), ElementsAre(0.5, 0.0, 0.5, 0.0));
}
