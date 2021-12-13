// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <fw/Action.h>
#include <if/Serial.h>
#include <stdint.h>
#include <util/Initialized.h>

namespace Clef::Fw {
namespace Str {
extern const char
    *const OK; /*!< The commmand was received and properly enqueued. */
extern const char *const BUFFER_OVERFLOW_ERROR; /*!< The internal buffer in the
                                             G-code parser is full; the current
                                             line is dumped from the buffer. */
extern const char
    *const INVALID_CODE_LETTER_ERROR; /*!< An invalid code letter (not A-Z,
                                   case-sensitive) was detected. */
extern const char
    *const DUPLICATE_CODE_LETTER_ERROR; /*!< The same code letter was
                                     supplied more than once. */
extern const char
    *const UNDEFINED_CODE_LETTER_ERROR; /*!< A code letter was not supplied or
                                     was supplied but did not have a value. */
extern const char *const
    INVALID_INT_ERROR; /*!< An integer was expected but could not be parsed. */
extern const char *const
    INVALID_FLOAT_ERROR; /*!< A float was expected but could not be parsed. */
extern const char *const
    MISSING_COMMAND_CODE_ERROR; /*!< Neither a 'G' nor an 'M' code was given. */
extern const char
    *const INVALID_G_CODE_ERROR; /*!< The requested G-code is not supported. */
extern const char
    *const INVALID_M_CODE_ERROR; /*!< The requested M-code is not supported. */
extern const char
    *const INSUFFICIENT_QUEUE_CAPACITY_ERROR; /*!< There is not enough space in
                                           the queue to insert all the actions
                                           required by the instruction. */
extern const char *const MISSING_ARGUMENT_ERROR; /*!< The code has a required
                                                    argument that is missing. */
extern const char *const INVALID_ARGUMENT_ERROR; /*!< The code has an argument
with a value that does not make sense. */
}  // namespace Str

/**
 * G-code parser. Characters are consumed through the ingest() function and
 * stored in a buffer. When a complete line is collected, it is parsed into
 * "buckets". Once a command type is detected (i.e. G or M code), other
 * arguments are parsed as needed and actions are enqueued for the firmware to
 * process.
 */
class GcodeParser {
 public:
  GcodeParser();

  /**
   * Consume as many characters as possible from serial input. This should be
   * called from the main event loop.
   */
  void ingest(Context &context);

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
  bool parse(const uint16_t errorBufferSize, char *const errorBuffer);

  /**
   * Interpret the arguments and perform and action.
   */
  bool interpret(Context &context, const uint16_t errorBufferSize,
                 char *const errorBuffer);

  /**
   * Check whether a code letter exists.
   */
  bool hasCodeLetter(const char code) const;

  /**
   * Interpret an integer from the buffer; returns false and writes a string to
   * the supplied error buffer if there was an error.
   */
  bool parseInt(const char code, int32_t *const result,
                const uint16_t errorBufferSize, char *const errorBuffer) const;

  /**
   * Interpret a float from the buffer; returns false and writes a string to the
   * supplied error buffer if there was an error.
   */
  bool parseFloat(const char code, float *const result,
                  const uint16_t errorBufferSize,
                  char *const errorBuffer) const;

  bool handleG1(Context &context, const uint16_t errorBufferSize,
                char *const errorBuffer);

  bool handleM104(Context &context, const uint16_t errorBufferSize,
                  char *const errorBuffer);

  bool handleM116(Context &context, const uint16_t errorBufferSize,
                  char *const errorBuffer);

 private:
  static const uint16_t size_ = 80; /*!< Static size instead of templating. */
  char buffer_[size_]; /*!< Accumulate characters until a line is complete. */
  char *head_; /*!< Points at the next available buffer_ char to fill. */
  const char *buckets_[26]; /*!< Start locations of the contents of every
                               detected code letter. */
  bool commentMode_;        /*!< Whether a comment was detected in the line. */
};
}  // namespace Clef::Fw
