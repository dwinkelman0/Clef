// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <avr/io.h>
#include <if/PwmTimer.h>
#include <stdint.h>

namespace Clef::Impl::Atmega2560 {
/**
 * A general interface for interacting with AVR-style timers.
 */
template <typename DType>
class HardwareTimer {
 public:
  enum class Prescaling : uint8_t {
    _1 = 1,
    _8 = 2,
    _64 = 3,
    _256 = 4,
    _1024 = 5
  };

  virtual void initRegs() = 0;
  virtual void setEnabled(const bool enabled) = 0;
  virtual void setCount(const DType value) = 0;
  virtual DType getCount() const = 0;
  virtual void setCompareA(const DType value) = 0;
  virtual void setCompareB(const DType value) = 0;
  virtual void setPrescaler(const Prescaling value) = 0;
  virtual DType getMaxValue() const = 0;
};

/**
 * This class defines how the required high-level functionalities of a timer are
 * expressed in terms of the AVR-style timer interface.
 */
template <typename DType>
class GenericTimer : public Clef::If::PwmTimer, public HardwareTimer<DType> {
 public:
  bool init() override;
  void enable() override;
  void disable() override;
  void setFrequency(const Clef::Util::Frequency<float> frequency);
};

/**
 * Explicitly instantiate classes with specializations that will be used.
 */
template class GenericTimer<uint8_t>;
template class GenericTimer<uint16_t>;

#define REG2(PREFIX, N) PREFIX##N
#define REG3(PREFIX, N, POSTFIX) PREFIX##N##POSTFIX

/**
 * Create classes which are essentially wrappers around timer registers.
 */
#define TIMER16(N)                                                           \
 public                                                                      \
  GenericTimer<uint16_t> {                                                   \
    friend class Clock;                                                      \
                                                                             \
   private:                                                                  \
    void initRegs() override {                                               \
      REG3(TCCR, N, A) = 0;                                                  \
      REG3(TCCR, N, B) = 0;                                                  \
      REG3(TCCR, N, C) = 0;                                                  \
      REG2(TCNT, N) = 0;                                                     \
      REG3(TCCR, N, B) |= (1 << REG3(WGM, N, 2));                            \
      REG3(OCR, N, A) = 0xffff;                                              \
      REG3(OCR, N, B) = 0xffff;                                              \
      REG2(TIMSK, N) = 0;                                                    \
    }                                                                        \
    void setEnabled(const bool enabled) override {                           \
      if (enabled) {                                                         \
        REG2(TIMSK, N) |= (1 << REG3(OCIE, N, A)) | (1 << REG3(OCIE, N, B)); \
        setCount(0);                                                         \
      } else {                                                               \
        REG2(TIMSK, N) &=                                                    \
            ~((1 << REG3(OCIE, N, A)) | (1 << REG3(OCIE, N, B)));            \
      }                                                                      \
    }                                                                        \
    void setCount(const uint16_t value) override { REG2(TCNT, N) = value; }  \
    uint16_t getCount() const override { return REG2(TCNT, N); }             \
    void setCompareA(const uint16_t value) override {                        \
      REG3(OCR, N, A) = value;                                               \
    }                                                                        \
    void setCompareB(const uint16_t value) override {                        \
      REG3(OCR, N, B) = value;                                               \
    }                                                                        \
    void setPrescaler(const HardwareTimer<uint16_t>::Prescaling value)       \
        override {                                                           \
      REG3(TCCR, N, B) &= ~((1 << REG3(CS, N, 0)) | (1 << REG3(CS, N, 1)) |  \
                            (1 << REG3(CS, N, 2)));                          \
      REG3(TCCR, N, B) |= (static_cast<int>(value) & 1) << REG3(CS, N, 0);   \
      REG3(TCCR, N, B) |= ((static_cast<int>(value) >> 1) & 1)               \
                          << REG3(CS, N, 1);                                 \
      REG3(TCCR, N, B) |= ((static_cast<int>(value) >> 2) & 1)               \
                          << REG3(CS, N, 2);                                 \
    }                                                                        \
    uint16_t getMaxValue() const { return 0xffff; }                          \
  }

class ClockTimer : TIMER16(1);
class XAxisTimer : TIMER16(3);
class YAxisTimer : TIMER16(4);
class ZEAxisTimer : TIMER16(5);

extern ClockTimer clockTimer;
extern XAxisTimer xAxisTimer;
extern YAxisTimer yAxisTimer;
extern ZEAxisTimer zeAxisTimer;
}  // namespace Clef::Impl::Atmega2560
