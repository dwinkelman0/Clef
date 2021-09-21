// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <fw/Config.h>
#include <if/Clock.h>
#include <if/Interrupts.h>
#include <util/Initialized.h>
#include <util/Units.h>

namespace Clef::Fw {
template <typename DType>
class Sensor {
 private:
  /**
   * Use a finite state machine model.
   */
  enum class State {
    NO_DATA,     /*!< Neither current_ nor staged_ have data. */
    DATA_READY,  /*!< current_ has data. */
    CHECKED_OUT, /*!< current_ has data that is currently being read. */
    CHECKED_OUT_AND_STAGED /*!< current_ has data that is currently being read,
                              and staged_ is ready to evict current_ when
                              current_ is released. */
  };

 public:
  /**
   * It is important that time be an integer so that it maintains precision at
   * high values.
   */
  using Time = Clef::Util::Time<uint64_t, Clef::Util::TimeUnit::USEC>;
  struct DataPoint {
    Time time;
    DType data;
  };

  Sensor(Clef::If::Clock &clock)
      : clock_(clock),
        state_(State::NO_DATA),
        current_({0, 0}),
        staged_({0, 0}) {}

  /**
   * Provide a data point from an external source.
   */
  void inject(DType data) {
    DataPoint dataPoint({*clock_.getMicros(), data});
    Clef::If::DisableInterrupts noInterrupts;
    switch (state_) {
      case State::NO_DATA:
        current_ = dataPoint;
        state_ = State::DATA_READY;
        onCurrentUpdate(current_);
        break;
      case State::DATA_READY:
        current_ = dataPoint;
        onCurrentUpdate(current_);
        break;
      case State::CHECKED_OUT:
        staged_ = dataPoint;
        state_ = State::CHECKED_OUT_AND_STAGED;
        break;
      case State::CHECKED_OUT_AND_STAGED:
        staged_ = dataPoint;
        break;
    }
  }

  /**
   * Transition the sensor to a state in which it is safe to read.
   */
  bool checkOut() {
    Clef::If::DisableInterrupts noInterrupts;
    switch (state_) {
      case State::DATA_READY:
        state_ = State::CHECKED_OUT;
        return true;
      default:
        return false;
    }
  }

  /**
   * Transition the sensor out of the state in which it is safe to read so that
   * the readable data can be refreshed.
   */
  void release() {
    Clef::If::DisableInterrupts noInterrupts;
    switch (state_) {
      case State::CHECKED_OUT:
        state_ = State::NO_DATA;
        break;
      case State::CHECKED_OUT_AND_STAGED:
        current_ = staged_;
        state_ = State::DATA_READY;
        onCurrentUpdate(current_);
        break;
      default:
        break;
    }
  }

  /**
   * This function is called whenever current_ updates.
   */
  virtual void onCurrentUpdate(const DataPoint dataPoint) {}

  /**
   * This function is only safe to call if state_ is CHECKED_OUT or
   * CHECKED_OUT_AND_STAGED.
   */
  DataPoint read() const { return current_; }

 private:
  Clef::If::Clock &clock_;
  State state_;
  DataPoint current_;
  DataPoint staged_;
};

template <uint32_t SENSOR_USTEPS_PER_MM, uint32_t AXIS_USTEPS_PER_MM>
class DisplacementSensor
    : public Sensor<Clef::Util::Position<float, Clef::Util::PositionUnit::MM,
                                         SENSOR_USTEPS_PER_MM>> {
 public:
  using SensorIf =
      Sensor<Clef::Util::Position<float, Clef::Util::PositionUnit::MM,
                                  SENSOR_USTEPS_PER_MM>>;
  using SensorAnalogPosition =
      Clef::Util::Position<float, Clef::Util::PositionUnit::MM,
                           SENSOR_USTEPS_PER_MM>;
  using SensorUstepsPosition =
      Clef::Util::Position<float, Clef::Util::PositionUnit::USTEP,
                           SENSOR_USTEPS_PER_MM>;
  using AxisPosition =
      Clef::Util::Position<float, Clef::Util::PositionUnit::USTEP,
                           AXIS_USTEPS_PER_MM>;
  using AxisFeedrate =
      Clef::Util::Feedrate<float, Clef::Util::PositionUnit::USTEP,
                           Clef::Util::TimeUnit::MIN, AXIS_USTEPS_PER_MM>;
  using DataPoint = typename SensorIf::DataPoint;

  DisplacementSensor(Clef::If::Clock &clock,
                     const float lowPassFilterCoefficient)
      : SensorIf(clock),
        currentFeedrate_(0),
        lastDataPoint_({0, 0}),
        lowPassFilterCoefficient_(lowPassFilterCoefficient) {}

  static void injectWrapper(SensorAnalogPosition data, void *arg) {
    DisplacementSensor *sensor = reinterpret_cast<DisplacementSensor *>(arg);
    sensor->inject(data);
  }

  AxisPosition readPosition() const {
    return convertToAxisPosition(read().data);
  }

  AxisFeedrate readFeedrate() const { return currentFeedrate_; }

  typename SensorIf::Time getMeasurementTime() const { return read().time; }

 protected:
  void onCurrentUpdate(const DataPoint dataPoint) override {
    if (lastDataPoint_.time > 0) {
      // Not the first sample
      AxisFeedrate newFeedrate(
          convertToAxisPosition(
              SensorAnalogPosition(dataPoint.data - lastDataPoint_.data)),
          Clef::Util::Time<float, Clef::Util::TimeUnit::MIN>(
              Clef::Util::Time<float, Clef::Util::TimeUnit::USEC>(
                  *(dataPoint.time - lastDataPoint_.time))));
      currentFeedrate_ = currentFeedrate_ * (1 - lowPassFilterCoefficient_) +
                         newFeedrate * lowPassFilterCoefficient_;
    }
    lastDataPoint_ = dataPoint;
  }

  static AxisPosition convertToAxisPosition(
      const SensorAnalogPosition position) {
    return AxisPosition(*SensorUstepsPosition(position));
  }

 private:
  /**
   * Make the underlying read() function private since it is in the wrong units.
   */
  DataPoint read() const { return SensorIf::read(); }

 private:
  AxisFeedrate currentFeedrate_;
  DataPoint lastDataPoint_;
  float lowPassFilterCoefficient_;
};

/**
 * Pressure is a dimensionless unit since there is no need for conversions and
 * the numbers from the sensor have no physical interpretation.
 */
class PressureSensor : public Sensor<uint16_t> {
 public:
  PressureSensor(Clef::If::Clock &clock, const float lowPassFilterCoefficient)
      : Sensor<uint16_t>(clock),
        currentPressure_(0),
        lowPassFilterCoefficient_(lowPassFilterCoefficient) {}

  /**
   * Inject wrapper is tailored for the SPI callback format.
   */
  static void injectWrapper(const uint16_t numData, const char *const data,
                            void *arg) {
    if (static_cast<uint8_t>(data[0]) & 0x80) {
      // Either a fault or stale data.
      return;
    }
    PressureSensor *sensor = reinterpret_cast<PressureSensor *>(arg);
    uint16_t hi = static_cast<uint16_t>(static_cast<uint8_t>(data[0]));
    uint16_t lo = static_cast<uint16_t>(static_cast<uint8_t>(data[1]));
    uint16_t rawData = ((hi << 8) | lo) & static_cast<uint16_t>(0x3fff);
    sensor->inject(rawData);
  }

  float readPressure() const { return currentPressure_; }

  typename Sensor<uint16_t>::Time getMeasurementTime() const {
    return read().time;
  }

 protected:
  void onCurrentUpdate(const DataPoint dataPoint) override {
    currentPressure_ =
        currentPressure_ * (1 - lowPassFilterCoefficient_) +
        static_cast<float>(dataPoint.data) * lowPassFilterCoefficient_;
  }

 private:
  /**
   * Make the underlying read() function private since it does not have
   * filtering.
   */
  DataPoint read() const { return Sensor<uint16_t>::read(); }

 private:
  float currentPressure_;
  float lowPassFilterCoefficient_;
};
}  // namespace Clef::Fw
