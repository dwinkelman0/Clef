// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <stdint.h>
#include <util/Acquired.h>
#include <util/Initialized.h>
#include <util/Units.h>

namespace Clef::If {
template <uint32_t USTEPS_PER_MM>
class Stepper : public Clef::Util::Initialized, public Clef::Util::Acquired {
 public:
  using Position = Clef::Util::Position<int32_t, Clef::Util::PositionUnit::MM,
                                        USTEPS_PER_MM>;

  enum class Resolution : uint8_t {
    _1 = 0,
    _2 = 1,
    _4 = 2,
    _8 = 3,
    _16 = 4,
    _32 = 5
  };

  virtual void setIncreasing() = 0;
  virtual void setDecreasing() = 0;
  virtual void isIncreasing() const = 0;

  virtual void setResolution(const Resolution resolution) = 0;
  virtual Resolution getResolution() const = 0;
  virtual void getUstepsPerPulse() const = 0;

  virtual void pulse() = 0;
  virtual void unpulse() = 0;

  void setTargetPosition(const Position position) {
    targetPosition_ = position;
  }
  Position getTargetPosition() const { return targetPosition_; }
  Position getPosition() const { return position_; }
  bool isAtTargetPosition() const {
    return isIncreasing() ? position_ >= targetPosition_
                          : position_ <= targetPosition_;
  }

 private:
  Position position_;
  Position targetPosition_;
};
}  // namespace Clef::If
