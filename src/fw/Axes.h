// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <fw/Config.h>
#include <if/PwmTimer.h>
#include <if/Stepper.h>
#include <stdint.h>
#include <util/Initialized.h>
#include <util/Units.h>

namespace Clef::Fw {
template <uint32_t USTEPS_PER_MM>
class Axis : public Clef::Util::Initialized {
 public:
  static const uint32_t UstepsPerMm = USTEPS_PER_MM;

  template <typename DType, Clef::Util::PositionUnit PositionU>
  using Position = Clef::Util::Position<DType, PositionU, USTEPS_PER_MM>;
  using GcodePosition =
      Clef::Util::Position<float, Clef::Util::PositionUnit::MM, USTEPS_PER_MM>;
  using StepperPosition =
      Clef::Util::Position<int32_t, Clef::Util::PositionUnit::USTEP,
                           USTEPS_PER_MM>;
  using Feedrate =
      Clef::Util::Feedrate<float, Clef::Util::PositionUnit::USTEP,
                           Clef::Util::TimeUnit::SEC, USTEPS_PER_MM>;

  Axis(Clef::If::Stepper<USTEPS_PER_MM> &stepper, Clef::If::PwmTimer &pwmTimer)
      : stepper_(stepper), pwmTimer_(pwmTimer) {}

  bool init() override {
    stepper_.init();
    pwmTimer_.init();
    pwmTimer_.setRisingEdgeCallback(onRisingEdge, this);
    pwmTimer_.setFallingEdgeCallback(onFallingEdge, this);
    pwmTimer_.enable();
    return true;
  }

  void acquire() { stepper_.acquire(); }

  void release() { stepper_.release(); }

  void releaseAll() { stepper_.releaseAll(); }

  void setTargetPosition(const StepperPosition position) {
    stepper_.setTargetPosition(position);
  }

  StepperPosition getPosition() const { return stepper_.getPosition(); }

  bool isAtTargetPosition() const { return stepper_.isAtTargetPosition(); }

  void setFeedrate(const Feedrate feedrate) {
    using Resolution = typename Clef::If::Stepper<USTEPS_PER_MM>::Resolution;
    const Clef::Util::Frequency maxFrequency(MAX_STEPPER_FREQ);
    Clef::Util::Frequency feedrateFrequency(
        (Clef::Util::Position<float, Clef::Util::PositionUnit::USTEP,
                              USTEPS_PER_MM>(1.0f) /
         feedrate)
            .asFrequency());
    Resolution resolution = Resolution::_32;
    Clef::Util::Frequency pulseFrequency(feedrateFrequency);
    if (feedrateFrequency < maxFrequency) {
    } else if (feedrateFrequency < (maxFrequency * 2)) {
      resolution = Resolution::_16;
      pulseFrequency = feedrateFrequency / 2;
    } else if (feedrateFrequency < (maxFrequency * 4)) {
      resolution = Resolution::_8;
      pulseFrequency = feedrateFrequency / 4;
    } else if (feedrateFrequency < (maxFrequency * 8)) {
      resolution = Resolution::_4;
      pulseFrequency = feedrateFrequency / 8;
    } else if (feedrateFrequency < (maxFrequency * 16)) {
      resolution = Resolution::_2;
      pulseFrequency = feedrateFrequency / 16;
    } else {
      resolution = Resolution::_1;
      pulseFrequency = feedrateFrequency / 32;
    }
    stepper_.setResolution(resolution);
    pwmTimer_.setFrequency(pulseFrequency);
  }

 private:
  static void onRisingEdge(void *arg) {
    Clef::Fw::Axis<USTEPS_PER_MM> *axis =
        reinterpret_cast<Clef::Fw::Axis<USTEPS_PER_MM> *>(arg);
    axis->stepper_.pulse();
  }

  static void onFallingEdge(void *arg) {
    Clef::Fw::Axis<USTEPS_PER_MM> *axis =
        reinterpret_cast<Clef::Fw::Axis<USTEPS_PER_MM> *>(arg);
    axis->stepper_.unpulse();
    if (axis->stepper_.isAtTargetPosition()) {
      axis->pwmTimer_.disable();
    }
  }

  Clef::If::Stepper<USTEPS_PER_MM> &stepper_;
  Clef::If::PwmTimer &pwmTimer_;
};

class Axes : public Clef::Util::Initialized {
 public:
  class XAxis : public Axis<USTEPS_PER_MM_X> {
   public:
    XAxis(Clef::If::Stepper<USTEPS_PER_MM_X> &stepper,
          Clef::If::PwmTimer &pwmTimer)
        : Axis(stepper, pwmTimer) {}
  };
  class YAxis : public Axis<USTEPS_PER_MM_Y> {};
  class ZAxis : public Axis<USTEPS_PER_MM_Z> {};
  class EAxis : public Axis<USTEPS_PER_MM_E> {};

  struct XYEPosition {
    using XPosition = XAxis::StepperPosition;
    using YPosition = YAxis::StepperPosition;
    using EPosition = EAxis::StepperPosition;
    XPosition x = 0;
    YPosition y = 0;
    EPosition e = 0;

    bool operator==(const XYEPosition &other) const;
    bool operator!=(const XYEPosition &other) const;
  };

  struct XYZEPosition {
    using XPosition = XAxis::StepperPosition;
    using YPosition = YAxis::StepperPosition;
    using ZPosition = ZAxis::StepperPosition;
    using EPosition = EAxis::StepperPosition;
    XPosition x = 0;
    YPosition y = 0;
    ZPosition z = 0;
    EPosition e = 0;

    XYEPosition asXyePosition() const;

    bool operator==(const XYZEPosition &other) const;
    bool operator!=(const XYZEPosition &other) const;
  };

  static const XYEPosition originXye;
  static const XYZEPosition originXyze;

  Axes(XAxis &x, YAxis &y, ZAxis &z, EAxis &e);
  XAxis &getX();
  const XAxis &getX() const;
  XAxis &getY();
  const XAxis &getY() const;
  XAxis &getZ();
  const XAxis &getZ() const;
  XAxis &getE();
  const XAxis &getE() const;

  bool init() override;

 private:
  XAxis &x_;
  YAxis &y_;
  ZAxis &z_;
  EAxis &e_;
};
}  // namespace Clef::Fw
