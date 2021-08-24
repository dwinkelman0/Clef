// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <if/Serial.h>
#include <stdint.h>

namespace Clef::Fw {
namespace Str {
extern const char *OK; /*!< The commmand was received and properly enqueued. */
extern const char *BUFFER_OVERFLOW_ERROR; /*!< The internal buffer in the G-code
                                             parser is full; the current line is
                                             dumped from the buffer. */
extern const char
    *INVALID_CODE_LETTER_ERROR; /*!< An invalid code letter (not A-Z,
                                   case-sensitive) was detected. */
extern const char *DUPLICATE_CODE_LETTER_ERROR; /*!< The same code letter was
                                                   supplied more than once. */
extern const char *UNDEFINED_CODE_LETTER_ERROR; /*!< A code letter was supplied
                                                   but did not have a value. */
extern const char
    *INVALID_INT_ERROR; /*!< An integer was expected but could not be parsed. */
extern const char
    *INVALID_FLOAT_ERROR; /*!< A float was expected but could not be parsed. */
extern const char
    *MISSING_COMMAND_CODE_ERROR; /*!< Neither a 'G' nor 'M' code was given. */
extern const char
    *INVALID_G_CODE_ERROR; /*!< The requested G-code is not supported. */
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
  static const uint16_t size_ = 80; /*!< Static size instead of templating. */
  char buffer_[size_]; /*!< Accumulate characters until a line is complete. */
  char *head_; /*!< Points at the next available buffer_ char to fill. */
  const char *buckets_[26]; /*!< Start locations of the contents of every
                               detected code letter. */
  Clef::If::RWSerial
      &serial_;      /*!< Input stream for receiving G-codes, output stream for
                        sending status messages to printer client. */
  bool commentMode_; /*!< Whether a comment was detected in the line. */
};
}  // namespace Clef::Fw
