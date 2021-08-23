// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

namespace Clef::If {
/**
 * Abstraction of a read-only serial connection.
 */
class RSerial {
 public:
  virtual void init() = 0;
  virtual bool isReadyToRead() const = 0;
  virtual bool read(char *const c) = 0;
};

/**
 * Abstraction of a write-only serial connection.
 */
class WSerial {
 public:
  virtual void init() = 0;
  virtual void writeChar(const char c) = 0;
  virtual void writeStr(const char *str) = 0;
  virtual void writeLine(const char *line) = 0;
};

/**
 * Abstraction of a two-way serial connection.
 */
class RWSerial : public RSerial, public WSerial {
 public:
  virtual void init() = 0;
};
}  // namespace Clef::If
