// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <stdint.h>

namespace Clef::Util {
template <uint16_t R, uint16_t C, bool T>
class BaseMatrix {
 public:
  virtual float get(const uint16_t r, const uint16_t c) const {
    return readData(calculateIndex(r, c));
  }

 protected:
  virtual uint16_t calculateIndex(const uint16_t r, const uint16_t c) const {
    return T ? c * R + r : r * C + c;
  }
  virtual float readData(const uint16_t index) const = 0;
};

template <uint16_t N>
class BaseDiagonalMatrix : public BaseMatrix<N, N, false> {
 public:
  virtual float get(const uint16_t r, const uint16_t c) const override {
    return r == c ? this->readData(r) : 0;
  }
};

template <uint16_t N>
class IdentityMatrix : public BaseDiagonalMatrix<N> {
 public:
  float get(const uint16_t r, const uint16_t c) const override {
    return r == c ? 1 : 0;
  }
};

template <uint16_t R, uint16_t C, bool T>
class BaseWritableMatrix : public BaseMatrix<R, C, T> {
 public:
  void set(const uint16_t r, const uint16_t c, const float value) {
    writeData(this->calculateIndex(r, c), value);
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
  for (int r = 0; r < K; ++r) {
    for (int c = 0; c < N; ++c) {
      float sum = 0;
      for (int j = 0; j < M; ++j) {
        sum += left.get(r, j) * right.get(j, c);
      }
      output.set(r, c, sum);
    }
  }
}

template <uint16_t M, uint16_t N, bool T>
void dot(const BaseDiagonalMatrix<M> &left, const BaseMatrix<M, N, T> &right,
         BaseWritableMatrix<M, N, false> &output) {
  for (int r = 0; r < M; ++r) {
    for (int c = 0; c < N; ++c) {
      output.set(r, c, left.get(r, r) * right.get(r, c));
    }
  }
}

template <uint16_t M, uint16_t N, bool T>
void dot(const BaseMatrix<M, N, T> &left, BaseDiagonalMatrix<N> &right,
         BaseWritableMatrix<M, N, false> &output) {
  for (int r = 0; r < M; ++r) {
    for (int c = 0; c < N; ++c) {
      output.set(r, c, left.get(r, c) * right.get(c, c));
    }
  }
}

template <uint16_t M, uint16_t N, bool T1, bool T2>
void add(const BaseMatrix<M, N, T1> &left, const BaseMatrix<M, N, T2> &right,
         BaseWritableMatrix<M, N, false> &output) {
  for (int r = 0; r < M; ++r) {
    for (int c = 0; c < N; ++c) {
      output.set(r, c, left.get(r, c) + right.get(r, c));
    }
  }
}

template <uint16_t M, uint16_t N, bool T1, bool T2>
void sub(const BaseMatrix<M, N, T1> &left, const BaseMatrix<M, N, T2> &right,
         BaseWritableMatrix<M, N, false> &output) {
  for (int r = 0; r < M; ++r) {
    for (int c = 0; c < N; ++c) {
      output.set(r, c, left.get(r, c) - right.get(r, c));
    }
  }
}
}  // namespace Matrix
}  // namespace Clef::Util
