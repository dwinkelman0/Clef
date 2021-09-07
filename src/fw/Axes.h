// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <fw/Config.h>
#include <fw/Sensor.h>
#include <if/Interrupts.h>
#include <if/PwmTimer.h>
#include <if/Stepper.h>
#include <math.h>
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
  using StepperPosition = typename Clef::If::Stepper<USTEPS_PER_MM>::Position;
  using GcodeFeedrate =
      Clef::Util::Feedrate<float, Clef::Util::PositionUnit::MM,
                           Clef::Util::TimeUnit::MIN, USTEPS_PER_MM>;

  Axis(Clef::If::Stepper<USTEPS_PER_MM> &stepper, Clef::If::PwmTimer &pwmTimer)
      : stepper_(stepper), pwmTimer_(pwmTimer) {}

  bool init() override {
    stepper_.init();
    pwmTimer_.init();
    return true;
  }

  void acquire() { stepper_.acquire(); }

  void release() { stepper_.release(); }

  void releaseAll() { stepper_.releaseAll(); }

  void setTargetPosition(const StepperPosition position) {
    Clef::If::DisableInterrupts noInterrupts;
    pwmTimer_.setRisingEdgeCallback(onRisingEdge, this);
    pwmTimer_.setFallingEdgeCallback(onFallingEdge, this);
    stepper_.setTargetPosition(position);
    if (!isAtTargetPosition()) {
      pwmTimer_.enable();
    }
  }

  StepperPosition getPosition() const { return stepper_.getPosition(); }

  bool isAtTargetPosition() const { return stepper_.isAtTargetPosition(); }

  void setFeedrate(const GcodeFeedrate feedrate) {
    using Resolution = typename Clef::If::Stepper<USTEPS_PER_MM>::Resolution;
    const Clef::Util::Frequency maxFrequency(MAX_STEPPER_FREQ);
    Clef::Util::Feedrate<float, Clef::Util::PositionUnit::USTEP,
                         Clef::Util::TimeUnit::SEC, USTEPS_PER_MM>
        stepperFeedrate(feedrate);
    Clef::Util::Frequency feedrateFrequency(
        (Clef::Util::Position<float, Clef::Util::PositionUnit::USTEP,
                              USTEPS_PER_MM>(1.0f) /
         stepperFeedrate)
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

template <uint32_t SENSOR_USTEPS_PER_MM, uint32_t USTEPS_PER_MM>
class ExtrusionAxis : public Axis<USTEPS_PER_MM> {
 public:
  ExtrusionAxis(Clef::If::Stepper<USTEPS_PER_MM> &stepper,
                Clef::If::PwmTimer &pwmTimer,
                Clef::Fw::DisplacementSensor<SENSOR_USTEPS_PER_MM,
                                             USTEPS_PER_MM> &displacementSensor)
      : Axis<USTEPS_PER_MM>(stepper, pwmTimer),
        displacementSensor_(displacementSensor) {}

  /**
   * If there is new sensor data, handle feedrate throttling.
   */
  void throttle() {
    if (displacementSensor_.checkOut()) {
      // TODO: actual logic
      displacementSensor_.release();
    }
  }

 private:
  Clef::Fw::DisplacementSensor<SENSOR_USTEPS_PER_MM, USTEPS_PER_MM>
      &displacementSensor_;
};

class Axes : public Clef::Util::Initialized {
 public:
  using XAxis = Axis<USTEPS_PER_MM_X>;
  using YAxis = Axis<USTEPS_PER_MM_Y>;
  using ZAxis = Axis<USTEPS_PER_MM_Z>;
  using EAxis = ExtrusionAxis<USTEPS_PER_MM_DISPLACEMENT, USTEPS_PER_MM_E>;

  struct XYEPosition {
    using XPosition = XAxis::StepperPosition;
    using YPosition = YAxis::StepperPosition;
    using EPosition = EAxis::StepperPosition;
    XPosition x = 0;
    YPosition y = 0;
    EPosition e = 0;

    bool operator==(const XYEPosition &other) const;
    bool operator!=(const XYEPosition &other) const;
    XYEPosition operator-(const XYEPosition &other) const {
      return {x - other.x, y - other.y, e - other.e};
    }
    float magnitude() const { return sqrt(*x * *x + *y * *y + *e * *e); }
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
    XYZEPosition operator-(const XYZEPosition &other) const {
      return {x - other.x, y - other.y, z - other.z, e - other.e};
    }
    float magnitude() const {
      return sqrt(*x * *x + *y * *y + *z * *z + *e * *e);
    }
  };

  static const XYEPosition originXye;
  static const XYZEPosition originXyze;

  Axes(XAxis &x, YAxis &y, ZAxis &z, EAxis &e)
      : x_(x), y_(y), z_(z), e_(e), feedrate_(1200.0f) {}
  XAxis &getX() { return x_; }
  const XAxis &getX() const { return x_; }
  YAxis &getY() { return y_; }
  const YAxis &getY() const { return y_; }
  ZAxis &getZ() { return z_; }
  const ZAxis &getZ() const { return z_; }
  EAxis &getE() { return e_; }
  const EAxis &getE() const { return e_; }

  bool init() override;
  void setFeedrate(const XAxis::GcodeFeedrate feedrate) {
    feedrate_ = feedrate;
  }
  XAxis::GcodeFeedrate getFeedrate() const { return feedrate_; }
  XYZEPosition getPosition() const {
    Clef::If::DisableInterrupts noInterrupts;
    return {x_.getPosition(), y_.getPosition(), z_.getPosition(),
            e_.getPosition()};
  }

 private:
  XAxis &x_;
  YAxis &y_;
  ZAxis &z_;
  EAxis &e_;
  XAxis::GcodeFeedrate feedrate_;
};
}  // namespace Clef::Fw
