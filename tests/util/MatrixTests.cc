// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <gtest/gtest.h>
#include <util/Matrix.h>

namespace Clef::Util {
TEST(MatrixTest, RamMatrixGet) {
  float data[] = {1, 2, 3, 4};
  RamMatrix<2, 2> m1(data);
  ASSERT_EQ(m1.get(0, 0), 1);
  ASSERT_EQ(m1.get(0, 1), 2);
  ASSERT_EQ(m1.get(1, 0), 3);
  ASSERT_EQ(m1.get(1, 1), 4);
}

TEST(MatrixTest, TransposedMatrix) {
  // Create transposed matrix from scratch
  float data[] = {1, 2, 3, 4, 5, 6};
  RamMatrix<3, 2>::Transpose m1(data);
  ASSERT_EQ(m1.get(0, 0), 1);
  ASSERT_EQ(m1.get(1, 0), 2);
  ASSERT_EQ(m1.get(0, 1), 3);
  ASSERT_EQ(m1.get(1, 1), 4);
  ASSERT_EQ(m1.get(0, 2), 5);
  ASSERT_EQ(m1.get(1, 2), 6);

  // Create transposed matrix from original matrix
  RamMatrix<3, 2> m2(data);
  RamMatrix<3, 2>::Transpose m3 = m2.transpose();
  ASSERT_EQ(m3.get(0, 0), 1);
  ASSERT_EQ(m3.get(1, 0), 2);
  ASSERT_EQ(m3.get(0, 1), 3);
  ASSERT_EQ(m3.get(1, 1), 4);
  ASSERT_EQ(m3.get(0, 2), 5);
  ASSERT_EQ(m3.get(1, 2), 6);
}

TEST(MatrixTest, DotProduct) {
  float data1[] = {1, 2, 3, 4, 5, 6};
  float data2[4];
  RamMatrix<2, 3> m1(data1);
  RamMatrix<2, 3>::Transpose m2 = m1.transpose();
  RamMatrix<2, 2> m3(data2);
  Matrix::dot(m1, m2, m3);
  ASSERT_EQ(m3.get(0, 0), 14);
  ASSERT_EQ(m3.get(0, 1), 32);
  ASSERT_EQ(m3.get(1, 0), 32);
  ASSERT_EQ(m3.get(1, 1), 77);

  float data3[] = {4, 6, 7, -3, 2, 0};
  float data4[] = {5, 1, -5, 2, 5, 2};
  float data5[4];
  RamMatrix<2, 3> m4(data3);
  RamMatrix<3, 2> m5(data4);
  RamMatrix<2, 2> m6(data5);
  Matrix::dot(m4, m5, m6);
  ASSERT_EQ(m6.get(0, 0), 25);
  ASSERT_EQ(m6.get(0, 1), 30);
  ASSERT_EQ(m6.get(1, 0), -25);
  ASSERT_EQ(m6.get(1, 1), 1);
}

TEST(MatrixTest, DiagonalDotProduct) {
  class RamDiagonalMatrix : public BaseDiagonalMatrix<3>,
                            public RamMatrix<3, 3> {
   public:
    RamDiagonalMatrix(float *data) : RamMatrix<3, 3>(data) {}
    float readData(const uint16_t index) const {
      return RamMatrix<3, 3>::readData(index);
    }
  };

  float data1[] = {1, 2, 3};
  float data2[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  float data3[9];
  RamDiagonalMatrix m1(data1);
  RamMatrix<3, 3> m2(data2);
  RamMatrix<3, 3> m3(data3);
  Matrix::dot(m1, m2, m3);
  ASSERT_EQ(m3.get(0, 0), 1);
  ASSERT_EQ(m3.get(0, 1), 2);
  ASSERT_EQ(m3.get(0, 2), 3);
  ASSERT_EQ(m3.get(1, 0), 8);
  ASSERT_EQ(m3.get(1, 1), 10);
  ASSERT_EQ(m3.get(1, 2), 12);
  ASSERT_EQ(m3.get(2, 0), 21);
  ASSERT_EQ(m3.get(2, 1), 24);
  ASSERT_EQ(m3.get(2, 2), 27);

  float data4[9];
  RamMatrix<3, 3> m4(data4);
  Matrix::dot(m2, m1, m4);
  ASSERT_EQ(m4.get(0, 0), 1);
  ASSERT_EQ(m4.get(0, 1), 4);
  ASSERT_EQ(m4.get(0, 2), 9);
  ASSERT_EQ(m4.get(1, 0), 4);
  ASSERT_EQ(m4.get(1, 1), 10);
  ASSERT_EQ(m4.get(1, 2), 18);
  ASSERT_EQ(m4.get(2, 0), 7);
  ASSERT_EQ(m4.get(2, 1), 16);
  ASSERT_EQ(m4.get(2, 2), 27);
}

TEST(MatrixTest, Arithmetic) {
  float data1[] = {1, 2, 3};
  float data2[] = {3, 4, 5};
  float data3[3];
  float data4[3];
  RamMatrix<3, 1> v1(data1);
  RamMatrix<3, 1> v2(data2);

  RamMatrix<3, 1> v3(data3);
  Matrix::add(v1, v2, v3);
  ASSERT_EQ(v3.get(0, 0), 4);
  ASSERT_EQ(v3.get(1, 0), 6);
  ASSERT_EQ(v3.get(2, 0), 8);

  RamMatrix<3, 1> v4(data3);
  Matrix::sub(v1, v2, v4);
  ASSERT_EQ(v4.get(0, 0), -2);
  ASSERT_EQ(v4.get(1, 0), -2);
  ASSERT_EQ(v4.get(2, 0), -2);
}

TEST(MatrixTest, Inverse) {
  float data1[16];
  float data2[16];
  float data3[16];
  RamMatrix<4, 4> m2(data1);
  RamMatrix<4, 4> m3(data2);
  RamMatrix<4, 4> m4(data3);

  float input1[] = {1, 5, 4, 6, 8, -9, 2, 5, 0, 3, 5, -1, 5, -6, -8, 3};
  RamMatrix<4, 4> m1a(input1);
  Matrix::inverse(m1a, m2, m3);
  Matrix::dot(m1a, m2, m4);
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      ASSERT_FLOAT_EQ(m3.get(r, c), r == c ? 1 : 0);
      ASSERT_NEAR(m4.get(r, c), r == c ? 1 : 0, 1e-6);
    }
  }

  // This test requires row swapping since there is a zero on the diagonal
  float input2[] = {4, 0, 0, 0, 0, 0, 2, 0, 0, 1, 2, 0, 1, 0, 0, 1};
  RamMatrix<4, 4> m1b(input1);
  Matrix::inverse(m1b, m2, m3);
  Matrix::dot(m1b, m2, m4);
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      ASSERT_FLOAT_EQ(m3.get(r, c), r == c ? 1 : 0);
      ASSERT_NEAR(m4.get(r, c), r == c ? 1 : 0, 1e-6);
    }
  }
}
}  // namespace Clef::Util
