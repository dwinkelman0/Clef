// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <if/Serial.h>
#include <stdint.h>

namespace Clef::If {
namespace Str {
extern const char *OK;
extern const char *BUFFER_OVERFLOW_ERROR;
extern const char *INVALID_CODE_LETTER_ERROR;
extern const char *DUPLICATE_CODE_LETTER_ERROR;
extern const char *UNDEFINED_CODE_LETTER_ERROR;
extern const char *INVALID_INT_ERROR;
extern const char *INVALID_FLOAT_ERROR;
extern const char *MISSING_COMMAND_CODE_ERROR;
extern const char *INVALID_G_CODE_ERROR;
}  // namespace Str

class GcodeParser {
 public:
  GcodeParser(Clef::If::RWSerial &serial);

  void init();

  /**
   * Consume as many characters as possible from serial input. This should be
   * called from the main event loop.
   */
  void ingest();

 private:
  /**
   * Reset the buffer.
   */
  void reset();

  /**
   * Try to add a character to the end of the buffer; returns false if there is
   * not capacity to do so.
   */
  bool append(const char newChar);

  /**
   * Break the stored buffer into ranges based on characters; returns false and
   * writes a string to the supplied error buffer if there was an error.
   */
  bool parse(const uint16_t errorBufferSize, char *errorBuffer);

  /**
   * Interpret the arguments and perform and action.
   */
  bool interpret(const uint16_t errorBufferSize, char *errorBuffer);

  /**
   * Interpret an integer from the buffer; returns false and writes a string to
   * the supplied error buffer if there was an error.
   */
  bool parseInt(const char code, int32_t *result,
                const uint16_t errorBufferSize, char *errorBuffer);

  /**
   * Interpret a float from the buffer; returns false and writes a string to the
   * supplied error buffer if there was an error.
   */
  bool parseFloat(const char code, float *result,
                  const uint16_t errorBufferSize, char *errorBuffer);

 private:
  static const uint16_t size_ = 80;
  char buffer_[size_];
  char *head_;
  const char *buckets_[26];
  Clef::If::RWSerial &serial_;
  bool commentMode_;
};
}  // namespace Clef::If
