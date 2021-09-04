// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <avr/io.h>
#include <if/PwmTimer.h>
#include <impl/atmega2560/AvrUtils.h>
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
  virtual Clef::Util::Frequency<float> _getMinFrequency() const = 0;
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
  void setFrequency(const Clef::Util::Frequency<float> frequency) override;
  Clef::Util::Frequency<float> getMinFrequency() const override;
  void setRisingEdgeCallback(const TransitionCallback callback,
                             void *data) override;
  void setFallingEdgeCallback(const TransitionCallback callback,
                              void *data) override;

  TransitionCallback risingEdgeCallback_ = nullptr;
  void *risingEdgeCallbackData_ = nullptr;
  TransitionCallback fallingEdgeCallback_ = nullptr;
  void *fallingEdgeCallbackData_ = nullptr;
};

/**
 * Explicitly instantiate classes with specializations that will be used.
 */
template class GenericTimer<uint8_t>;
template class GenericTimer<uint16_t>;

/**
 * Create classes which are essentially wrappers around timer registers. This is
 * the version for an 8-bit timer (0, 2).
 */
#define TIMER8(N, MAX_PRESCALING)                                            \
 public                                                                      \
  GenericTimer<uint8_t> {                                                    \
   private:                                                                  \
    void initRegs() override {                                               \
      REG3(TCCR, N, A) = 0;                                                  \
      REG3(TCCR, N, B) = 0;                                                  \
      REG2(TCNT, N) = 0;                                                     \
      REG3(TCCR, N, A) |= (1 << REG3(WGM, N, 1)); /*!< CTC Mode. */          \
      REG3(OCR, N, A) = 0xff;                                                \
      REG3(OCR, N, B) = 0xff;                                                \
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
    void setCount(const uint8_t value) override { REG2(TCNT, N) = value; }   \
    uint8_t getCount() const override { return REG2(TCNT, N); }              \
    void setCompareA(const uint8_t value) override {                         \
      REG3(OCR, N, A) = value;                                               \
    }                                                                        \
    void setCompareB(const uint8_t value) override {                         \
      REG3(OCR, N, B) = value;                                               \
    }                                                                        \
    void setPrescaler(const HardwareTimer<uint8_t>::Prescaling value)        \
        override {                                                           \
      REG3(TCCR, N, B) &= ~((1 << REG3(CS, N, 0)) | (1 << REG3(CS, N, 1)) |  \
                            (1 << REG3(CS, N, 2)));                          \
      REG3(TCCR, N, B) |= (static_cast<int>(value) & 1) << REG3(CS, N, 0);   \
      REG3(TCCR, N, B) |= ((static_cast<int>(value) >> 1) & 1)               \
                          << REG3(CS, N, 1);                                 \
      REG3(TCCR, N, B) |= ((static_cast<int>(value) >> 2) & 1)               \
                          << REG3(CS, N, 2);                                 \
    }                                                                        \
    uint8_t getMaxValue() const override { return 0xff; }                    \
    Clef::Util::Frequency<float> _getMinFrequency() const override {         \
      return static_cast<float>(F_CPU) / (static_cast<uint32_t>(1) << 8) /   \
             static_cast<uint32_t>(MAX_PRESCALING);                          \
    }                                                                        \
  }

/**
 * Create classes which are essentially wrappers around timer registers. This is
 * the version for a 16-bit timer (1, 3, 4, 5).
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
      REG3(TCCR, N, B) |= (1 << REG3(WGM, N, 2)); /*!< CTC Mode. */          \
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
    uint16_t getMaxValue() const override { return 0xffff; }                 \
    Clef::Util::Frequency<float> _getMinFrequency() const override {         \
      return static_cast<float>(F_CPU) / (static_cast<uint32_t>(1) << 16) /  \
             (static_cast<uint32_t>(1) << 10);                               \
    }                                                                        \
  }

class Timer1 : TIMER8(0, 1024);
class Timer2 : TIMER8(2, 64);
class ClockTimer : TIMER16(1);
class XAxisTimer : TIMER16(3);
class YAxisTimer : TIMER16(4);
class ZEAxisTimer : TIMER16(5);

#undef TIMER8
#undef TIMER16

extern Timer1 timer1;
extern Timer2 timer2;
extern ClockTimer clockTimer;
extern XAxisTimer xAxisTimer;
extern YAxisTimer yAxisTimer;
extern ZEAxisTimer zeAxisTimer;
}  // namespace Clef::Impl::Atmega2560
