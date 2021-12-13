// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <fw/Config.h>
#include <fw/ExtrusionPredictor.h>
#include <fw/Heater.h>
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
  template <typename DType, Clef::Util::PositionUnit PositionU>
  using Position = Clef::Util::Position<DType, PositionU, USTEPS_PER_MM>;
  using GcodePosition =
      Clef::Util::Position<float, Clef::Util::PositionUnit::MM, USTEPS_PER_MM>;
  using StepperPosition = typename Clef::If::Stepper<USTEPS_PER_MM>::Position;
  using GcodeFeedrate =
      Clef::Util::Feedrate<float, Clef::Util::PositionUnit::MM,
                           Clef::Util::TimeUnit::MIN, USTEPS_PER_MM>;
  using UstepFeedrate =
      Clef::Util::Feedrate<float, Clef::Util::PositionUnit::USTEP,
                           Clef::Util::TimeUnit::MIN, USTEPS_PER_MM>;

  Axis(Clef::If::Stepper<USTEPS_PER_MM> &stepper, Clef::If::PwmTimer &pwmTimer)
      : stepper_(stepper), pwmTimer_(pwmTimer) {}

  static StepperPosition gcodePositionToStepper(const GcodePosition pos) {
    return StepperPosition(
        *Clef::Util::Position<float, Clef::Util::PositionUnit::USTEP,
                              USTEPS_PER_MM>(pos));
  }

  static GcodePosition stepperPositionToGcode(const StepperPosition pos) {
    return GcodePosition(
        Clef::Util::Position<float, Clef::Util::PositionUnit::USTEP,
                             USTEPS_PER_MM>(*pos));
  }

  bool init() override {
    stepper_.init();
    pwmTimer_.init();
    return true;
  }

  void acquire() { stepper_.acquire(); }

  void release() { stepper_.release(); }

  void releaseAll() { stepper_.releaseAll(); }

  void setTargetPosition(const GcodePosition position) {
    StepperPosition convertedPosition(static_cast<int32_t>(
        *Position<float, Clef::Util::PositionUnit::USTEP>(position)));
    Clef::If::DisableInterrupts noInterrupts;
    pwmTimer_.setRisingEdgeCallback(onRisingEdge, this);
    pwmTimer_.setFallingEdgeCallback(onFallingEdge, this);
    stepper_.setTargetPosition(convertedPosition);
    if (!isAtTargetPosition() && !pwmTimer_.isEnabled()) {
      pwmTimer_.enable();
    }
  }

  StepperPosition getTargetStepperPosition() const {
    return stepper_.getTargetPosition();
  }

  StepperPosition getPosition() const { return stepper_.getPosition(); }
  GcodePosition getGcodePosition() const {
    StepperPosition pos = stepper_.getPosition();
    return GcodePosition(Position<float, Clef::Util::PositionUnit::USTEP>(
        static_cast<float>(*pos)));
  }

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

 protected:
  Clef::If::Stepper<USTEPS_PER_MM> &stepper_;
  Clef::If::PwmTimer &pwmTimer_;
};

struct XYEPosition {
  using XAxis = Axis<USTEPS_PER_MM_X>;
  using YAxis = Axis<USTEPS_PER_MM_Y>;
  using EAxis = Axis<USTEPS_PER_MM_E>;
  using XPosition = XAxis::GcodePosition;
  using YPosition = YAxis::GcodePosition;
  using EPosition = EAxis::GcodePosition;

  XPosition x = 0;
  YPosition y = 0;
  EPosition e = 0;

  bool operator==(const XYEPosition &other) const;
  bool operator!=(const XYEPosition &other) const;
  XYEPosition operator-(const XYEPosition &other) const {
    return {x - other.x, y - other.y, e - other.e};
  }
  float getXyMagnitude() const { return sqrt(*x * *x + *y * *y); }
};

struct XYZEPosition {
  using XAxis = Axis<USTEPS_PER_MM_X>;
  using YAxis = Axis<USTEPS_PER_MM_Y>;
  using ZAxis = Axis<USTEPS_PER_MM_Z>;
  using EAxis = Axis<USTEPS_PER_MM_E>;
  using XPosition = XAxis::GcodePosition;
  using YPosition = YAxis::GcodePosition;
  using ZPosition = ZAxis::GcodePosition;
  using EPosition = EAxis::GcodePosition;

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
  float getXyMagnitude() const { return sqrt(*x * *x + *y * *y); }
};

template <uint32_t SENSOR_USTEPS_PER_MM, uint32_t USTEPS_PER_MM>
class ExtrusionAxis : public Axis<USTEPS_PER_MM> {
 public:
  ExtrusionAxis(Clef::If::Stepper<USTEPS_PER_MM> &stepper,
                Clef::If::PwmTimer &pwmTimer,
                Clef::Fw::DisplacementSensor<SENSOR_USTEPS_PER_MM,
                                             USTEPS_PER_MM> &displacementSensor,
                Clef::Fw::PressureSensor &pressureSensor,
                Clef::Fw::ExtrusionPredictor &predictor,
                Clef::Fw::Heater &syringeHeater, Clef::Fw::Heater &needleHeater)
      : Axis<USTEPS_PER_MM>(stepper, pwmTimer),
        displacementSensor_(displacementSensor),
        pressureSensor_(pressureSensor),
        predictor_(predictor),
        displacementSensorOffset_(0.0f),
        syringeHeater_(syringeHeater),
        needleHeater_(needleHeater) {
    displacementSensorToken_ = displacementSensor_.subscribe();
    pressureSensorToken_ = pressureSensor_.subscribe();
  }

  /**
   * Set the amount by which the displacement sensor and axis differ. This
   * should be done during global firmware initialization and homing.
   */
  void setDisplacementSensorOffset(
      const typename Axis<USTEPS_PER_MM>::template Position<
          float, Clef::Util::PositionUnit::USTEP>
          offset) {
    displacementSensorOffset_ = offset;
  }

  /**
   * If there is new sensor data, handle feedrate throttling. Returns the
   * feedrate at which the XY axes should operate.
   */
  bool throttle(const XYEPosition &startPosition,
                const XYEPosition &endPosition,
                const XYEPosition &currentPosition, float *xyFeedrate) {
    bool hasNewData = false;
    if (displacementSensor_.checkOut(displacementSensorToken_)) {
      if (pressureSensor_.checkOut(pressureSensorToken_)) {
        hasNewData = true;
        float t = *pressureSensor_.getMeasurementTime();
        float xe = *this->stepper_.getPosition();
        float xs = *displacementSensor_.readPosition();
        float P = pressureSensor_.readPressure();
        predictor_.evolve(t / 1e6, xe, xs, P);
        pressureSensor_.release(pressureSensorToken_);
      }
      displacementSensor_.release(displacementSensorToken_);
    }
    *xyFeedrate = predictor_.determineXYFeedrate(
        *XYEPosition::XAxis::gcodePositionToStepper(startPosition.x),
        *XYEPosition::YAxis::gcodePositionToStepper(startPosition.y),
        *XYEPosition::EAxis::gcodePositionToStepper(startPosition.e),
        *XYEPosition::XAxis::gcodePositionToStepper(endPosition.x),
        *XYEPosition::YAxis::gcodePositionToStepper(endPosition.y),
        *XYEPosition::EAxis::gcodePositionToStepper(endPosition.e),
        *XYEPosition::XAxis::gcodePositionToStepper(currentPosition.x),
        *XYEPosition::YAxis::gcodePositionToStepper(currentPosition.y));
    return hasNewData;
  }

  void beginExtrusion(
      const Clef::Util::Time<uint64_t, Clef::Util::TimeUnit::USEC> time) {
    typename Axis<USTEPS_PER_MM>::StepperPosition stepperPosition =
        *this->stepper_.getPosition();
    Clef::Util::Time<float, Clef::Util::TimeUnit::USEC> timeFloat(*time);
    Clef::Util::Time<float, Clef::Util::TimeUnit::SEC> timeSeconds(timeFloat);
    predictor_.reset(*timeSeconds, *stepperPosition,
                     *stepperPosition + *displacementSensorOffset_);
  }

  /**
   * Check whether the correct amount of material has been extruded. This method
   * is intended for XYE extrusions.
   */
  bool isExtrusionDone() const { return predictor_.isBeyondEndpoint(); };

  /**
   * Set the amount of material that should be extruded. This method is intended
   * for XYE extrusions.
   */
  void setExtrusionEndpoint(
      const typename Axis<USTEPS_PER_MM>::GcodePosition position) {
    predictor_.setEndpoint(*this->gcodePositionToStepper(position));
    this->setTargetPosition(position);
  }

  typename Axis<USTEPS_PER_MM>::GcodePosition getExtrusionEndpoint() const {
    return this->stepperPositionToGcode(predictor_.getEndpoint());
  }

  Clef::Fw::Heater &getSyringeHeater() { return syringeHeater_; }

  Clef::Fw::Heater &getNeedleHeater() { return needleHeater_; }

 private:
  Clef::Fw::DisplacementSensor<SENSOR_USTEPS_PER_MM, USTEPS_PER_MM>
      &displacementSensor_;
  uint8_t displacementSensorToken_;
  Clef::Fw::PressureSensor &pressureSensor_;
  uint8_t pressureSensorToken_;
  Clef::Fw::ExtrusionPredictor &predictor_;

  typename Axis<USTEPS_PER_MM>::template Position<
      float, Clef::Util::PositionUnit::USTEP>
      displacementSensorOffset_; /*!< Subtract this quantity from xs to get the
                                    corresponding value of xe. */

  Clef::Fw::Heater &syringeHeater_;
  Clef::Fw::Heater &needleHeater_;
};

class Axes : public Clef::Util::Initialized {
 public:
  using XAxis = Axis<USTEPS_PER_MM_X>;
  using YAxis = Axis<USTEPS_PER_MM_Y>;
  using ZAxis = Axis<USTEPS_PER_MM_Z>;
  using EAxis = ExtrusionAxis<USTEPS_PER_MM_DISPLACEMENT, USTEPS_PER_MM_E>;

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

  /**
   * Set feedrate in raw mm/min (a.k.a. GcodeFeedrate).
   */
  void setFeedrate(const XAxis::GcodeFeedrate feedrate) {
    feedrate_ = feedrate;
  }

  XAxis::GcodeFeedrate getFeedrate() const { return feedrate_; }

  /**
   * Set the XY position and feedrate.
   */
  void setXyParams(const XYEPosition &startPosition,
                   const XYEPosition &endPosition,
                   const XAxis::GcodeFeedrate feedrate) {
    const XYEPosition difference = endPosition - startPosition;
    const float magnitude = difference.getXyMagnitude();
    getX().setFeedrate(feedrate * fabs(*difference.x / magnitude));
    getY().setFeedrate(feedrate * fabs(*difference.y / magnitude));
    getX().setTargetPosition(endPosition.x);
    getY().setTargetPosition(endPosition.y);
  }

  XYZEPosition getCurrentPosition() const {
    return {getX().getGcodePosition(), getY().getGcodePosition(),
            getZ().getGcodePosition(), getE().getGcodePosition()};
  }

 private:
  XAxis &x_;
  YAxis &y_;
  ZAxis &z_;
  EAxis &e_;
  XAxis::GcodeFeedrate feedrate_;
};
}  // namespace Clef::Fw
