// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <avr/io.h>
#include <impl/atmega2560/AvrUtils.h>

namespace Clef::Impl::Atmega2560 {
#define WREGISTER_BOOL(P, N, INVERT)                                   \
  {                                                                    \
   public:                                                             \
    static void init() { REG2(DDR, P) |= 1 << shamt_; }                \
    static void write(const bool value) {                              \
      REG2(PORT, P) &= ~(1 << shamt_);                                 \
      REG2(PORT, P) |= static_cast<uint8_t>(INVERT ^ value) << shamt_; \
    }                                                                  \
    static bool getCurrentState() {                                    \
      return INVERT ^ ((REG2(PORT, P) >> shamt_) & 1);                 \
    }                                                                  \
                                                                       \
   private:                                                            \
    static const uint8_t shamt_ = REG3(PORT, P, N);                    \
  };

class WPin8 WREGISTER_BOOL(H, 5, false);
}  // namespace Clef::Impl::Atmega2560
