// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <util/Initialized.h>

namespace Clef::If {
template <typename DType>
class SensorInput : public Clef::Util::Initialized {
 public:
  using ConversionCallback = void (*)(const DType, void *);
  void setConversionCallback(const ConversionCallback callback, void *data) {
    conversionCallback_ = callback;
    conversionCallbackData_ = data;
  }

 protected:
  ConversionCallback conversionCallback_;
  void *conversionCallbackData_;
};
}  // namespace Clef::If
