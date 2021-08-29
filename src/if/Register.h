// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

namespace Clef::If {

/**
 * Abstraction of a read-only register.
 */
template <typename DType>
class RRegister {
 public:
  virtual void init() = 0;
  virtual DType read() const = 0;
};

/**
 * Abstraction of a write-only register.
 */
template <typename DType>
class WRegister {
 public:
  virtual void init() = 0;
  virtual void write(const DType value) = 0;
  virtual DType getCurrentState() const = 0;
};
}  // namespace Clef::If
