// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <avr/interrupt.h>
#include <avr/io.h>
#include <impl/atmega2560/AvrUtils.h>

namespace Clef::Impl::Atmega2560 {
/**
 * Writeable boolean register.
 */
#define W_REGISTER_BOOL(P, N, INVERT)                                  \
  {                                                                    \
   public:                                                             \
    static void init() { REG2(DDR, P) |= 1 << shamt_; }                \
    static void write(const bool value) {                              \
      REG2(PORT, P) &= ~(1 << shamt_);                                 \
      REG2(PORT, P) |= static_cast<uint8_t>(INVERT ^ value) << shamt_; \
    }                                                                  \
    static bool getCurrentState() {                                    \
      return INVERT ^ ((REG2(PORT, P) >> shamt_) & 1);                 \
    }                                                                  \
                                                                       \
   private:                                                            \
    static const uint8_t shamt_ = REG3(PORT, P, N);                    \
  };

/**
 * Readable boolean register.
 */
#define R_REGISTER_BOOL(P, N)                                                 \
  {                                                                           \
   public:                                                                    \
    static void init() {                                                      \
      REG2(DDR, P) &= ~(1 << REG3(PIN, P, N)); /*!< Set this pin as input. */ \
    }                                                                         \
    static void setPullUp() { REG2(PORT, P) |= 1 << REG3(PORT, P, N); }       \
    static bool read() { return REG2(PIN, P) & (1 << REG3(PIN, P, N)); }      \
  }

/**
 * Readable boolean register with its own on-change interrupt.
 */
#define RINT_REGISTER_BOOL_EDGE_LOW 0
#define RINT_REGISTER_BOOL_EDGE_BOTH 1
#define RINT_REGISTER_BOOL_EDGE_FALLING 2
#define RINT_REGISTER_BOOL_EDGE_RISING 3
#define RINT_REGISTER_BOOL(P, N, INT_GROUP, INT_NUM, EDGES)                    \
  {                                                                            \
   public:                                                                     \
    static void init() {                                                       \
      REG2(DDR, P) &= ~(1 << REG3(PIN, P, N)); /*!< Set this pin as input. */  \
      REG2(EICR, INT_GROUP) &= ~(                                              \
          3 << REG3(ISC, INT_NUM, 0)); /*!< Enable interrupts for this pin. */ \
      REG2(EICR, INT_GROUP) |=                                                 \
          EDGES << REG3(ISC, INT_NUM,                                          \
                        0); /*!< Enable rising/falling edge trigger. */        \
      EIMSK |= (1 << REG2(INT, INT_NUM));                                      \
    }                                                                          \
    static bool read() { return REG2(PIN, P) & (1 << REG3(PIN, P, N)); }       \
    static void setCallback(void (*callback)(void *), void *data) {            \
      callback_ = callback;                                                    \
      callbackData_ = data;                                                    \
    }                                                                          \
                                                                               \
    static void (*callback_)(void *);                                          \
    static void *callbackData_;                                                \
  }

#define RINT_REGISTER_BOOL_ISRS(NAME, INT_NUM)                   \
  void (*NAME::callback_)(void *); /*!< Define callback. */      \
  void *NAME::callbackData_;       /*!< Define callback data. */ \
  ISR(REG3(INT, INT_NUM, _vect)) {                               \
    if (NAME::callback_) {                                       \
      NAME::callback_(NAME::callbackData_);                      \
    }                                                            \
  }

/**
 * Readable boolean register as part of a group of registers which share the
 * same on-change interrupts.
 */
#define RINTGROUP_REGISTER_BOOL(P, N, INT_GROUP, INT_INDEX)                    \
  {                                                                            \
   public:                                                                     \
    static void init() {                                                       \
      REG2(DDR, P) &= ~(1 << REG3(PIN, P, N)); /*!< Set this pin as input. */  \
      PCICR |=                                                                 \
          1 << REG2(                                                           \
              PCIE,                                                            \
              INT_GROUP); /*!< Enable interrupts for this group of pins. */    \
      REG2(PCMSK, INT_GROUP) |=                                                \
          1 << INT_INDEX; /*!< Enable interrupts for this pin. */              \
    }                                                                          \
    static bool read() { return REG2(PIN, P) & (1 << REG3(PIN, P, N)); }       \
    static void setGroupChangeCallback(void (*callback)(void *), void *data) { \
      groupChangeCallback_ = callback;                                         \
      callbackData_ = data;                                                    \
    }                                                                          \
                                                                               \
   private:                                                                    \
    static void (*groupChangeCallback_)(void *);                               \
    static void *callbackData_;                                                \
  }

class WPin8 W_REGISTER_BOOL(H, 5, false);
class RIntGroupPinA15 RINTGROUP_REGISTER_BOOL(K, 7, 2, 7);
}  // namespace Clef::Impl::Atmega2560
