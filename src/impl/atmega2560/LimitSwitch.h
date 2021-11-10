// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/LimitSwitch.h>
#include <impl/atmega2560/Register.h>

namespace Clef::Impl::Atmega2560 {
#define LIMIT_SWITCH(P, N)                                           \
 public                                                              \
  Clef::If::LimitSwitch {                                            \
   public:                                                           \
    bool init() override {                                           \
      Register::init();                                              \
      Register::setPullUp();                                         \
      return true;                                                   \
    };                                                               \
    bool getInputState() const override { return Register::read(); } \
    void checkForTransition(const uint8_t difference) {              \
      if ((difference >> N) & 1) {                                   \
        onTransition();                                              \
      }                                                              \
    }                                                                \
    uint8_t getInputStateVector() const {                            \
      return static_cast<uint8_t>(getInputState()) << N;             \
    }                                                                \
                                                                     \
   protected:                                                        \
    class Register R_REGISTER_BOOL(P, N);                            \
  };

class XLimitSwitch : LIMIT_SWITCH(K, 2);    /*!< Analog pin 10. */
class YLimitSwitch : LIMIT_SWITCH(K, 1);    /*!< Analog pin 9. */
class ZLimitSwitch : LIMIT_SWITCH(K, 0);    /*!< Analog pin 8. */
class EIncLimitSwitch : LIMIT_SWITCH(K, 3); /*!< Analog pin 11. */
class EDecLimitSwitch : LIMIT_SWITCH(K, 4); /*!< Analog pin 12. */

class LimitSwitchBank : public Clef::Util::Initialized {
 public:
  bool init() override;
  void checkTransitions();

  XLimitSwitch &getX() { return xLimitSwitch_; }
  YLimitSwitch &getY() { return yLimitSwitch_; }
  ZLimitSwitch &getZ() { return zLimitSwitch_; }
  EIncLimitSwitch &getEInc() { return eIncLimitSwitch_; }
  EDecLimitSwitch &getEDec() { return eDecLimitSwitch_; }

 private:
  uint8_t readState() const;

  XLimitSwitch xLimitSwitch_;
  YLimitSwitch yLimitSwitch_;
  ZLimitSwitch zLimitSwitch_;
  EIncLimitSwitch eIncLimitSwitch_;
  EDecLimitSwitch eDecLimitSwitch_;

  uint8_t state_;
};

extern LimitSwitchBank limitSwitches;
}  // namespace Clef::Impl::Atmega2560
