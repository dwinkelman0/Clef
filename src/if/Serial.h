// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <stdint.h>
#include <util/Initialized.h>
#include <util/Units.h>

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

/**
 * Abstraction of a read-only Serial Protocol Interface (SPI) connection.
 */
class RSpi : public Clef::Util::Initialized {
 public:
  using ReadCompleteCallback = void (*)(const uint16_t, const char *const,
                                        void *);

  RSpi();

  /**
   * Read some number of bytes; this will fail and return false if the interface
   * is busy, i.e. currently performing a read or executing
   * readCompleteCallback. Specify the delay between SS being asserted and CLK
   * oscillation (for cases in which devices cannot transmit immediately).
   */
  virtual bool initRead(
      const uint16_t size,
      const Clef::Util::Time<uint16_t, Clef::Util::TimeUnit::USEC> delay) = 0;

  /**
   * This callback is called after initRead() prepares the specified number of
   * bytes.
   */
  void setReadCompleteCallback(const ReadCompleteCallback callback, void *data);

 protected:
  ReadCompleteCallback readCompleteCallback_;
  void *readCompleteCallbackData_;
};
}  // namespace Clef::If
