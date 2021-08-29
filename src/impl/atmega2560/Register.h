// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <avr/io.h>
#include <if/Register.h>
#include <impl/atmega2560/AvrUtils.h>

namespace Clef::Impl::Atmega2560 {
class WBooleanRegister : public Clef::If::WRegister<bool> {};

#define WREGISTER_BOOL(P, N, INVERT)                                   \
  {                                                                    \
   public:                                                             \
    void init() override { REG2(DDR, P) |= 1 << shamt_; }              \
    void write(const bool value) override {                            \
      REG2(PORT, P) &= ~(1 << shamt_);                                 \
      REG2(PORT, P) |= static_cast<uint8_t>(INVERT ^ value) << shamt_; \
    }                                                                  \
    bool getCurrentState() const override {                            \
      return INVERT ^ ((REG2(PORT, P) >> shamt_) & 1);                 \
    }                                                                  \
                                                                       \
   private:                                                            \
    const uint8_t shamt_ = REG3(PORT, P, N);                           \
  };

class WPin8 : public WBooleanRegister WREGISTER_BOOL(H, 5, false);
WPin8 pin8;
}  // namespace Clef::Impl::Atmega2560
