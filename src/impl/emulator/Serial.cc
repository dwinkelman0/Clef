// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Serial.h"

#include <mutex>

namespace Clef::Impl::Emulator {
Serial::Serial(std::shared_ptr<std::mutex> globalMutex)
    : globalMutex_(globalMutex) {}

bool Serial::init() { return Clef::Util::Initialized::init(); }

bool Serial::isReadyToRead() const {
  std::unique_lock<std::mutex> lock(*globalMutex_);
  return inputStream_.size() > 0;
}

bool Serial::read(char *const c) {
  std::unique_lock<std::mutex> lock(*globalMutex_);
  if (inputStream_.size() > 0) {
    *c = inputStream_.front();
    inputStream_.pop();
    return true;
  } else {
    *c = '\0';
    return false;
  }
}

void Serial::writeChar(const char c) {
  std::unique_lock<std::mutex> lock(*globalMutex_);
  outputStream_.push(c);
}

void Serial::writeStr(const char *str) {
  std::unique_lock<std::mutex> lock(*globalMutex_);
  for (const char *it = str; *it; ++it) {
    outputStream_.push(*it);
  };
}

void Serial::writeLine(const char *line) {
  writeStr(line);
  writeChar('\n');
}

void Serial::inject(const std::string &str) {
  std::unique_lock<std::mutex> lock(*globalMutex_);
  for (char c : str) {
    inputStream_.push(c);
  }
}

std::string Serial::extract() {
  std::unique_lock<std::mutex> lock(*globalMutex_);
  std::string output;
  while (outputStream_.size() > 0) {
    output.push_back(outputStream_.front());
    outputStream_.pop();
  }
  return output;
}
}  // namespace Clef::Impl::Emulator
