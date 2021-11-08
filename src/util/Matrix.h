// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <assert.h>
#include <stdint.h>
#include <string.h>

#ifndef TARGET_AVR
#include <iomanip>
#include <iostream>
#endif

namespace Clef::Util {
template <uint16_t R, uint16_t C, bool T>
class BaseMatrix {
 public:
  virtual float get(const uint16_t r, const uint16_t c) const {
    return readData(calculateIndex(r, c));
  }

#ifndef TARGET_AVR
  template <uint16_t R_, uint16_t C_, bool T_>
  friend std::ostream &operator<<(std::ostream &os,
                                  const BaseMatrix<R_, C_, T_> &mat);
#endif

 protected:
  virtual uint16_t calculateIndex(const uint16_t r, const uint16_t c) const {
#ifndef TARGET_AVR
    assert(r < R);
    assert(c < C);
#endif
    return T ? c * R + r : r * C + c;
  }
  virtual float readData(const uint16_t index) const = 0;
};

#ifndef TARGET_AVR
template <uint16_t R, uint16_t C, bool T>
std::ostream &operator<<(std::ostream &os, const BaseMatrix<R, C, T> &mat) {
  os << std::fixed << std::setprecision(6);
  for (unsigned int r = 0; r < R; ++r) {
    for (unsigned int c = 0; c < C; ++c) {
      os << std::setw(10) << mat.get(r, c) << ",";
    }
    os << std::endl;
  }
  return os;
}
#endif

template <uint16_t N>
class BaseDiagonalMatrix : public BaseMatrix<N, N, false> {
 public:
  virtual float get(const uint16_t r, const uint16_t c) const override {
#ifndef TARGET_AVR
    assert(r < N);
    assert(c < N);
#endif
    return r == c ? this->readData(r) : 0;
  }
};

template <uint16_t N>
class IdentityMatrix : public BaseDiagonalMatrix<N> {
 public:
  IdentityMatrix() {}

  float get(const uint16_t r, const uint16_t c) const override {
#ifndef TARGET_AVR
    assert(r < N);
    assert(c < N);
#endif
    return r == c ? 1 : 0;
  }

 private:
  float readData(const uint16_t index) const override { return 0; }
};

template <uint16_t R, uint16_t C, bool T>
class BaseWritableMatrix : public BaseMatrix<R, C, T> {
 public:
  void set(const uint16_t r, const uint16_t c, const float value) {
    writeData(this->calculateIndex(r, c), value);
  }

  virtual void fill(const float value) {
    for (unsigned int r = 0; r < R; ++r) {
      for (unsigned int c = 0; c < C; ++c) {
        set(r, c, value);
      }
    }
  }

 protected:
  virtual void writeData(const uint16_t index, const float value) = 0;
};

template <uint16_t R, uint16_t C, bool T>
class BaseRamMatrix : public BaseWritableMatrix<R, C, T> {
  friend class BaseRamMatrix<C, R, !T>;

 public:
  BaseRamMatrix(float *data) : data_(data) {}
  BaseRamMatrix(const BaseRamMatrix &other) : data_(other.data_) {}
  BaseRamMatrix(const BaseRamMatrix<C, R, !T> &other) : data_(other.data_) {}

  BaseRamMatrix<C, R, !T> transpose() const {
    return BaseRamMatrix<C, R, !T>(*this);
  }

  void fill(const float value) override {
    if (value == 0) {
      memset(data_, 0, R * C * sizeof(float));
    } else {
      BaseWritableMatrix<R, C, T>::fill(value);
    }
  }

 protected:
  float readData(const uint16_t index) const override { return data_[index]; }

  virtual void writeData(const uint16_t index, const float value) override {
    data_[index] = value;
  }

 private:
  float *data_;
};

template <uint16_t R, uint16_t C>
class WriteableMatrix : public BaseWritableMatrix<R, C, false> {
 public:
  using Transpose = BaseWritableMatrix<C, R, true>;
};

template <uint16_t R, uint16_t C>
class RamMatrix : public BaseRamMatrix<R, C, false> {
 public:
  using Transpose = BaseRamMatrix<C, R, true>;
  RamMatrix(float *data) : BaseRamMatrix<R, C, false>(data) {}
};

namespace Matrix {
template <uint16_t K, uint16_t M, uint16_t N, bool T1, bool T2>
void dot(const BaseMatrix<K, M, T1> &left, const BaseMatrix<M, N, T2> &right,
         BaseWritableMatrix<K, N, false> &output) {
  for (unsigned int r = 0; r < K; ++r) {
    for (unsigned int c = 0; c < N; ++c) {
      float sum = 0;
      for (unsigned int j = 0; j < M; ++j) {
        sum += left.get(r, j) * right.get(j, c);
      }
      output.set(r, c, sum);
    }
  }
}

template <uint16_t M, uint16_t N, bool T>
void dot(const BaseDiagonalMatrix<M> &left, const BaseMatrix<M, N, T> &right,
         BaseWritableMatrix<M, N, false> &output) {
  for (unsigned int r = 0; r < M; ++r) {
    for (unsigned int c = 0; c < N; ++c) {
      output.set(r, c, left.get(r, r) * right.get(r, c));
    }
  }
}

template <uint16_t M, uint16_t N, bool T>
void dot(const BaseMatrix<M, N, T> &left, BaseDiagonalMatrix<N> &right,
         BaseWritableMatrix<M, N, false> &output) {
  for (unsigned int r = 0; r < M; ++r) {
    for (unsigned int c = 0; c < N; ++c) {
      output.set(r, c, left.get(r, c) * right.get(c, c));
    }
  }
}

template <uint16_t M, uint16_t N, bool T1, bool T2>
void add(const BaseMatrix<M, N, T1> &left, const BaseMatrix<M, N, T2> &right,
         BaseWritableMatrix<M, N, false> &output) {
  for (unsigned int r = 0; r < M; ++r) {
    for (unsigned int c = 0; c < N; ++c) {
      output.set(r, c, left.get(r, c) + right.get(r, c));
    }
  }
}

template <uint16_t M, uint16_t N, bool T1, bool T2>
void sub(const BaseMatrix<M, N, T1> &left, const BaseMatrix<M, N, T2> &right,
         BaseWritableMatrix<M, N, false> &output) {
  for (unsigned int r = 0; r < M; ++r) {
    for (unsigned int c = 0; c < N; ++c) {
      output.set(r, c, left.get(r, c) - right.get(r, c));
    }
  }
}

namespace {
/**
 * @brief Substep in Gauss-Jordan elimination inverse matrix algorithm. Scratch
 * becomes an upper-triangular matrix with identity diagonal, and output becomes
 * a lower-triangular matrix
 *
 * @param scratch
 * @param output
 */
template <uint16_t N>
void triangularSwap(RamMatrix<N, N> &scratch, RamMatrix<N, N> &output) {
  for (unsigned int c = 0; c < N; ++c) {
    // Normalize the row
    float diag = scratch.get(c, c);
    for (unsigned int r = c + 1; r < N && diag == 0; ++r) {
      // May need to do swaps if one of the diagonal elements is zero
      for (unsigned int j = 0; j < N; ++j) {
        float temp = output.get(c, j);
        output.set(c, j, output.get(r, j));
        output.set(r, j, temp);
      }
      for (unsigned int j = c; j < N; ++j) {
        float temp = scratch.get(c, j);
        scratch.set(c, j, scratch.get(r, j));
        scratch.set(r, j, temp);
      }
      diag = scratch.get(c, c);
    }
    for (unsigned int j = 0; j < N; ++j) {
      output.set(c, j, output.get(c, j) / diag);
    }
    for (unsigned int j = c; j < N; ++j) {
      scratch.set(c, j, scratch.get(c, j) / diag);
    }

    // Subtract the row from all lower rows
    for (unsigned int r = c + 1; r < N; ++r) {
      float factor = scratch.get(r, c);
      for (unsigned int j = 0; j < N; ++j) {
        output.set(r, j, output.get(r, j) - output.get(c, j) * factor);
      }
      for (unsigned int j = c; j < N; ++j) {
        scratch.set(r, j, scratch.get(r, j) - scratch.get(c, j) * factor);
      }
    }
  }
}

template <uint16_t N, bool T>
void rowMirror(BaseWritableMatrix<N, N, T> &mat) {
  for (unsigned int r = 0; r < N / 2; ++r) {
    for (unsigned int c = 0; c < N; ++c) {
      float temp = mat.get(r, c);
      mat.set(r, c, mat.get(N - r - 1, c));
      mat.set(N - r - 1, c, temp);
    }
  }
}

template <uint16_t N, bool T>
void columnMirror(BaseWritableMatrix<N, N, T> &mat) {
  for (unsigned int r = 0; r < N; ++r) {
    for (unsigned int c = 0; c < N / 2; ++c) {
      float temp = mat.get(r, c);
      mat.set(r, c, mat.get(r, N - c - 1));
      mat.set(r, N - c - 1, temp);
    }
  }
}
}  // namespace

/**
 * @brief Destructive variant of matrix inversion using the Gauss-Jordan
 * elimination algorithm.
 *
 * @tparam N
 * @param mat Input matrix; this is overwritten.
 * @param output Output matrix.
 */
template <uint16_t N>
void inverse(RamMatrix<N, N> &mat, RamMatrix<N, N> &output) {
  for (unsigned int r = 0; r < N; ++r) {
    for (unsigned int c = 0; c < N; ++c) {
      output.set(r, c, r == c ? 1 : 0);
    }
  }

  // TODO: This is a lazy approach and may be worth reworking
  triangularSwap(mat, output);
  columnMirror(mat);
  columnMirror(output);
  rowMirror(mat);
  rowMirror(output);
  triangularSwap(mat, output);
  columnMirror(output);
  rowMirror(output);
}

/**
 * @brief Non-destructive variant of matrix inversion.
 *
 * @tparam N
 * @tparam T
 * @param mat Input matrix; this is not touched.
 * @param output Output matrix.
 * @param scratch Scratch space used for the calculation.
 */
template <uint16_t N, bool T>
void inverse(const BaseMatrix<N, N, T> &mat, RamMatrix<N, N> &output,
             RamMatrix<N, N> &scratch) {
  for (unsigned int r = 0; r < N; ++r) {
    for (unsigned int c = 0; c < N; ++c) {
      scratch.set(r, c, mat.get(r, c));
    }
  }
  inverse(scratch, output);
}

template <uint16_t M, uint16_t N, bool T1, bool T2>
void copy(const BaseMatrix<M, N, T1> &src, BaseWritableMatrix<M, N, T2> &dst) {
  for (unsigned int r = 0; r < M; ++r) {
    for (unsigned int c = 0; c < N; ++c) {
      dst.set(r, c, src.get(r, c));
    }
  }
}
}  // namespace Matrix
}  // namespace Clef::Util
