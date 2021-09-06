// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Register.h"

#include <avr/interrupt.h>

namespace Clef::Impl::Atmega2560 {
#define RINT_REGISTER_BOOL_ISRS(NAME, INT_NUM)                   \
  void (*NAME::callback_)(void *); /*!< Define callback. */      \
  void *NAME::callbackData_;       /*!< Define callback data. */ \
  ISR(REG3(INT, INT_NUM, _vect)) {                               \
    if (NAME::callback_) {                                       \
      NAME::callback_(NAME::callbackData_);                      \
    }                                                            \
  }

RINT_REGISTER_BOOL_ISRS(RIntPin2, 4)
}  // namespace Clef::Impl::Atmega2560
