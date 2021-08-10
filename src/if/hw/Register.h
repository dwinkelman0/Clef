// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

namespace Clef::If::Hw {

/**
 * Abstraction of a read-only register.
 */
template <typename DType>
class RRegister {
 public:
  virtual DType read() const = 0;
};

/**
 * Abstraction of a write-only register.
 */
template <typename DType>
class WRegister {
 public:
  virtual void write(const DType value) = 0;
};

/**
 * Abstraction of a read-write register.
 */
template <typename DType>
class RWRegister : public RRegister, public WRegister {};
}  // namespace Clef::If::Hw
