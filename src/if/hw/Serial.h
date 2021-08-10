// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

namespace Clef::If::Hw {
/**
 * Abstraction of a read-only serial connection.
 */
class RSerial {
 public:
  virtual void init() = 0;
  virtual bool readyToRead() const = 0;
  virtual bool read(char *const c) = 0;
};

/**
 * Abstraction of a write-only serial connection.
 */
class WSerial {
 public:
  virtual void init() = 0;
  virtual bool readyToWrite() const = 0;
  virtual bool write(const char c) = 0;
};

/**
 * Abstraction of a two-way serial connection.
 */
class RWSerial : public RSerial, public WSerial {};
}  // namespace Clef::If::Hw
