// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Serial.h"

namespace Clef::If {
RSpi::RSpi()
    : readCompleteCallback_(nullptr), readCompleteCallbackData_(nullptr) {}

void RSpi::setReadCompleteCallback(const ReadCompleteCallback callback,
                                   void *data) {
  readCompleteCallback_ = callback;
  readCompleteCallbackData_ = data;
}
}  // namespace Clef::If
