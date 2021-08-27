// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

namespace Clef::Util {
/**
 * Interface for data structures which need to be explicitly initialized.
 */
class Initialized {
 public:
  Initialized() : initialized_(false) {}

  /**
   * Returns false if and only if this object is already initialized.
   */
  virtual bool init() {
    if (initialized_) {
      return false;
    }
    initialized_ = true;
    return true;
  }

 private:
  bool initialized_;
};
}  // namespace Clef::Util
