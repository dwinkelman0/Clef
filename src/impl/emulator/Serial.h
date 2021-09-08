// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/Serial.h>
#include <stdint.h>

#include <memory>
#include <mutex>
#include <queue>
#include <string>

namespace Clef::Impl::Emulator {
class Serial : public Clef::If::RWSerial {
 public:
  Serial(std::shared_ptr<std::mutex> globalMutex);

  bool init() override;
  bool isReadyToRead() const override;
  bool read(char *const c) override;
  void writeChar(const char c) override;
  void writeStr(const char *str) override;
  void writeLine(const char *line) override;

  /**
   * Provide characters for the consumer of this serial interface to read.
   */
  void inject(const std::string &str);

  /**
   * Collect characters written by the producer on this serial interface.
   */
  std::string extract();

 private:
  std::shared_ptr<std::mutex> globalMutex_;
  std::queue<char> inputStream_;
  std::queue<char> outputStream_;
};
}  // namespace Clef::Impl::Emulator
