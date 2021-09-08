// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <stdint.h>

namespace Clef::Util {
class Acquired {
 public:
  void acquire() {
    if (numAcquisitions_ == 0) {
      onFirstAcquire();
    }
    numAcquisitions_++;
  }

  void release() {
    if (numAcquisitions_ > 0) {
      numAcquisitions_--;
    }
    if (numAcquisitions_ == 0) {
      onLastRelease();
    }
  }

  void releaseAll() {
    numAcquisitions_ = 0;
    onLastRelease();
  }

  virtual void onFirstAcquire() = 0;

  virtual void onLastRelease() = 0;

 private:
  uint8_t numAcquisitions_ = 0;
};
}  // namespace Clef::Util
