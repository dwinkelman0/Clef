// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <util/Initialized.h>

namespace Clef::If {
/**
 * Abstraction of a read-only serial connection.
 */
class RSerial : public Clef::Util::Initialized {
 public:
  virtual bool isReadyToRead() const = 0;
  virtual bool read(char *const c) = 0;
};

/**
 * Abstraction of a two-way serial connection.
 */
class RWSerial : public RSerial {
 public:
  virtual void writeChar(const char c) = 0;
  virtual void writeStr(const char *str) = 0;
  virtual void writeLine(const char *line) = 0;
};
}  // namespace Clef::If
