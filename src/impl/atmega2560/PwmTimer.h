// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <avr/io.h>
#include <if/PwmTimer.h>
#include <impl/atmega2560/AvrUtils.h>
#include <impl/atmega2560/Register.h>
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
  virtual DType getCompareA() const = 0;
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
  GenericTimer();

  bool init() override;
  void enable() override;
  void disable() override;
  void setFrequency(const Clef::Util::Frequency<float> frequency) override;
  Clef::Util::Frequency<float> getMinFrequency() const override;
  void setDutyCycle(const float dutyCycle) override;
  void setRisingEdgeCallback(const TransitionCallback callback,
                             void *data) override;
  void setFallingEdgeCallback(const TransitionCallback callback,
                              void *data) override;

  TransitionCallback risingEdgeCallback_ = nullptr;
  void *risingEdgeCallbackData_ = nullptr;
  TransitionCallback fallingEdgeCallback_ = nullptr;
  void *fallingEdgeCallbackData_ = nullptr;

 private:
  float dutyCycle_;
};

template <typename DType>
class GenericDirectOutputTimer : public Clef::If::DirectOutputPwmTimer,
                                 public HardwareTimer<DType> {
 public:
  bool init() override;
  void enable() override;
  void disable() override;
  void setDutyCycleA(const float dutyCycle) override;
  void setDutyCycleB(const float dutyCycle) override;
  void setCallbackA(const TransitionCallback callback, void *data) override;
  void setCallbackB(const TransitionCallback callback, void *data) override;
  void setCallbackTop(const TransitionCallback callback, void *data) override;

  TransitionCallback callbackA_ = nullptr;
  void *callbackAData_ = nullptr;
  TransitionCallback callbackB_ = nullptr;
  void *callbackBData_ = nullptr;
  TransitionCallback callbackTop_ = nullptr;
  void *callbackTopData_ = nullptr;
};

/**
 * Explicitly instantiate classes with specializations that will be used.
 */
template class GenericTimer<uint8_t>;
template class GenericTimer<uint16_t>;

/**
 * Create classes which are essentially wrappers around timer registers. This is
 * the version for an 8-bit timer (0, 2).
 *
 * NOTE: Timers in PWM mode have non-code-defined output pins.
 */
#define TIMER8(N, EXTRA_PRESCALING)                                          \
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
    bool isEnabled() const override {                                        \
      return REG2(TIMSK, N) &                                                \
             ((1 << REG3(OCIE, N, A)) | (1 << REG3(OCIE, N, B)));            \
    }                                                                        \
    void setCount(const uint8_t value) override { REG2(TCNT, N) = value; }   \
    uint8_t getCount() const override { return REG2(TCNT, N); }              \
    void setCompareA(const uint8_t value) override {                         \
      REG3(OCR, N, A) = value;                                               \
    }                                                                        \
    uint8_t getCompareA() const override { return REG3(OCR, N, A); }         \
    void setCompareB(const uint8_t value) override {                         \
      REG3(OCR, N, B) = value;                                               \
    }                                                                        \
    void setPrescaler(const HardwareTimer<uint8_t>::Prescaling value)        \
        override {                                                           \
      int actual = static_cast<int>(value);                                  \
      if (EXTRA_PRESCALING) {                                                \
        if (actual == 3) {                                                   \
          actual = 4;                                                        \
        } else if (actual == 4) {                                            \
          actual = 6;                                                        \
        } else if (actual == 5) {                                            \
          actual = 7;                                                        \
        }                                                                    \
      }                                                                      \
      REG3(TCCR, N, B) &= ~((1 << REG3(CS, N, 0)) | (1 << REG3(CS, N, 1)) |  \
                            (1 << REG3(CS, N, 2)));                          \
      REG3(TCCR, N, B) |= (actual & 1) << REG3(CS, N, 0);                    \
      REG3(TCCR, N, B) |= ((actual >> 1) & 1) << REG3(CS, N, 1);             \
      REG3(TCCR, N, B) |= ((actual >> 2) & 1) << REG3(CS, N, 2);             \
    }                                                                        \
    uint8_t getMaxValue() const override { return 0xff; }                    \
    Clef::Util::Frequency<float> _getMinFrequency() const override {         \
      return static_cast<float>(F_CPU) / (static_cast<uint32_t>(1) << 8) /   \
             (static_cast<uint32_t>(1) << 10);                               \
    }                                                                        \
  }

#define TIMER8_PWM(N, PIN_A, PIN_B, EXTRA_PRESCALING)                       \
 public                                                                     \
  GenericDirectOutputTimer<uint8_t> {                                       \
   private:                                                                 \
    void initRegs() override {                                              \
      PIN_A::init();                                                        \
      PIN_B::init();                                                        \
      REG3(TCCR, N, A) = 0;                                                 \
      REG3(TCCR, N, B) = 0;                                                 \
      REG2(TCNT, N) = 0;                                                    \
      REG3(TCCR, N, A) |= (0 << REG3(WGM, N, 1)) |                          \
                          (1 << REG3(WGM, N, 0)); /*!< CTC PWM Mode. */     \
      REG3(OCR, N, A) = 0xff;                                               \
      REG3(OCR, N, B) = 0xff;                                               \
      REG2(TIMSK, N) = 0;                                                   \
    }                                                                       \
    void setEnabled(const bool enabled) override {                          \
      if (enabled) {                                                        \
        REG3(TCCR, N, A) |= (1 << REG3(COM, N, A1));                        \
        REG3(TCCR, N, A) |= (1 << REG3(COM, N, B1));                        \
        REG2(TIMSK, N) |= (1 << REG3(OCIE, N, A));                          \
        REG2(TIMSK, N) |= (1 << REG3(OCIE, N, B));                          \
        REG2(TIMSK, N) |= (1 << REG2(TOIE, N));                             \
      } else {                                                              \
        REG3(TCCR, N, A) &= ~(1 << REG3(COM, N, A1));                       \
        REG3(TCCR, N, A) &= ~(1 << REG3(COM, N, B1));                       \
        REG2(TIMSK, N) &= ~(1 << REG3(OCIE, N, A));                         \
        REG2(TIMSK, N) &= ~(1 << REG3(OCIE, N, B));                         \
        REG2(TIMSK, N) &= ~(1 << REG2(TOIE, N));                            \
      }                                                                     \
    }                                                                       \
    bool isEnabled() const override {                                       \
      return REG2(TIMSK, N) &                                               \
             ((1 << REG3(COM, N, A1)) | (1 << REG3(COM, N, B1)));           \
    }                                                                       \
    void setCount(const uint8_t value) override { REG2(TCNT, N) = value; }  \
    uint8_t getCount() const override { return REG2(TCNT, N); }             \
    void setCompareA(const uint8_t value) override {                        \
      REG3(OCR, N, A) = value;                                              \
    }                                                                       \
    uint8_t getCompareA() const override { return REG3(OCR, N, A); }        \
    void setCompareB(const uint8_t value) override {                        \
      REG3(OCR, N, B) = value;                                              \
    }                                                                       \
    void setPrescaler(const HardwareTimer<uint8_t>::Prescaling value)       \
        override {                                                          \
      int actual = static_cast<int>(value);                                 \
      if (EXTRA_PRESCALING) {                                               \
        if (actual == 3) {                                                  \
          actual = 4;                                                       \
        } else if (actual == 4) {                                           \
          actual = 6;                                                       \
        } else if (actual == 5) {                                           \
          actual = 7;                                                       \
        }                                                                   \
      }                                                                     \
      REG3(TCCR, N, B) &= ~((1 << REG3(CS, N, 0)) | (1 << REG3(CS, N, 1)) | \
                            (1 << REG3(CS, N, 2)));                         \
      REG3(TCCR, N, B) |= (actual & 1) << REG3(CS, N, 0);                   \
      REG3(TCCR, N, B) |= ((actual >> 1) & 1) << REG3(CS, N, 1);            \
      REG3(TCCR, N, B) |= ((actual >> 2) & 1) << REG3(CS, N, 2);            \
    }                                                                       \
    uint8_t getMaxValue() const override { return 0xff; }                   \
    Clef::Util::Frequency<float> _getMinFrequency() const override {        \
      return static_cast<float>(F_CPU) / (static_cast<uint32_t>(1) << 8) /  \
             (static_cast<uint32_t>(1) << 10);                              \
    }                                                                       \
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
    bool isEnabled() const override {                                        \
      return REG2(TIMSK, N) &                                                \
             ((1 << REG3(OCIE, N, A)) | (1 << REG3(OCIE, N, B)));            \
    }                                                                        \
    void setCount(const uint16_t value) override { REG2(TCNT, N) = value; }  \
    uint16_t getCount() const override { return REG2(TCNT, N); }             \
    void setCompareA(const uint16_t value) override {                        \
      REG3(OCR, N, A) = value;                                               \
    }                                                                        \
    uint16_t getCompareA() const override { return REG3(OCR, N, A); }        \
    void setCompareB(const uint16_t value) override {                        \
      REG3(OCR, N, B) = value;                                               \
    }                                                                        \
    void setPrescaler(const HardwareTimer<uint16_t>::Prescaling value)       \
        override {                                                           \
      int actual = static_cast<int>(value);                                  \
      REG3(TCCR, N, B) &= ~((1 << REG3(CS, N, 0)) | (1 << REG3(CS, N, 1)) |  \
                            (1 << REG3(CS, N, 2)));                          \
      REG3(TCCR, N, B) |= (actual & 1) << REG3(CS, N, 0);                    \
      REG3(TCCR, N, B) |= ((actual >> 1) & 1) << REG3(CS, N, 1);             \
      REG3(TCCR, N, B) |= ((actual >> 2) & 1) << REG3(CS, N, 2);             \
    }                                                                        \
    uint16_t getMaxValue() const override { return 0xffff; }                 \
    Clef::Util::Frequency<float> _getMinFrequency() const override {         \
      return static_cast<float>(F_CPU) / (static_cast<uint32_t>(1) << 16) /  \
             (static_cast<uint32_t>(1) << 10);                               \
    }                                                                        \
  }

class Timer1 : TIMER8(0, false);
class Timer2PinA W_REGISTER_BOOL(B, 4, false); /*!< Pin 10. */
class Timer2PinB W_REGISTER_BOOL(H, 6, false); /*!< Pin 9. */
class Timer2 : TIMER8_PWM(2, Timer2PinA, Timer2PinB, true);
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
