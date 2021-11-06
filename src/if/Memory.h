// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <util/Matrix.h>

#ifdef TARGET_AVR
#include <avr/pgmspace.h>
#endif

/**
 * Macro for defining constant program memory so that it is stored efficiently
 * in memory (e.g. as PROGMEM in AVR).
 */
#ifdef TARGET_AVR
#define STRING(name, contents) const char *const name PROGMEM = #contents
#define ARRAY(type, name, ...) const type name[] PROGMEM = __VA_ARGS__
#else
#define STRING(name, contents) const char *const name = contents
#define ARRAY(type, name, ...) const type name[] = __VA_ARGS__
#endif

namespace Clef::If {
template <uint16_t N>
class RomDiagonalMatrix : public Clef::Util::BaseDiagonalMatrix<N> {
 public:
  RomDiagonalMatrix(const float *const data) : data_(data) {}

 private:
  float readData(const uint16_t index) const override {
#ifdef TARGET_AVR
    return pgm_read_float(data_ + index);
#else
    return data_[index];
#endif
  }

  const float *data_;
};
}  // namespace Clef::If
