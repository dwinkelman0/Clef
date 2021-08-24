// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "GcodeParser.h"

#include <if/String.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Clef::Fw {
namespace Str {
STRING(OK, "ok");
STRING(BUFFER_OVERFLOW_ERROR, "buffer_overflow_error");
STRING(INVALID_CODE_LETTER_ERROR, "invalid_code_letter_error");
STRING(DUPLICATE_CODE_LETTER_ERROR, "duplicate_code_letter_error");
STRING(UNDEFINED_CODE_LETTER_ERROR, "undefined_code_letter_error");
STRING(INVALID_INT_ERROR, "invalid_int_error");
STRING(INVALID_FLOAT_ERROR, "invalid_float_error");
STRING(MISSING_COMMAND_CODE_ERROR, "missing_command_code_error");
STRING(INVALID_G_CODE_ERROR, "invalid_g_code_error");
}  // namespace Str

GcodeParser::GcodeParser(Clef::If::RWSerial &serial) : serial_(serial) {
  reset();
}

void GcodeParser::init() { serial_.init(); }

void GcodeParser::ingest() {
  const uint16_t errorBufferSize = 64;
  char errorBuffer[errorBufferSize];
  char newChar;
  while (serial_.read(&newChar)) {
    if (newChar == '\n') {
      commentMode_ = false;
      // Process the line
      if (parse(errorBufferSize, errorBuffer)) {
        bool anyCodes = false;
        for (uint8_t i = 0; i < 26; ++i) {
          if (buckets_[i]) {
            anyCodes = true;
          }
        }
        if (anyCodes) {
          if (interpret(errorBufferSize, errorBuffer)) {
            serial_.writeLine(Str::OK);
          } else {
            serial_.writeLine(errorBuffer);
          }
        }
      } else {
        serial_.writeLine(errorBuffer);
      }
      reset();
    } else if (newChar == ';') {
      // If there is a semicolon, ignore everything until the next new line.
      commentMode_ = true;
    } else if (!commentMode_) {
      // Add the char to the buffer
      if (!append(newChar)) {
        // Flush the buffer
        while (serial_.read(&newChar)) {
        }
        serial_.writeLine(Str::BUFFER_OVERFLOW_ERROR);
        reset();
      }
    }
  }
}

void GcodeParser::reset() {
  head_ = buffer_;
  commentMode_ = false;
  memset(buckets_, 0, sizeof(buckets_));
  memset(buffer_, 0, sizeof(buffer_));
}

bool GcodeParser::append(const char newChar) {
  if (head_ < (buffer_ + size_ - 1)) {
    *(head_++) = newChar;
    *head_ = '\0';
    return true;
  }
  return false;
}

bool GcodeParser::parse(const uint16_t errorBufferSize, char *errorBuffer) {
  memset(buckets_, 0, sizeof(buckets_));
  char *pch = strtok(buffer_, " ");
  while (pch) {
    uint8_t letter = pch[0] - 'A';
    if (letter >= 26) {
      snprintf(errorBuffer, errorBufferSize, "%s: %c",
               Str::INVALID_CODE_LETTER_ERROR, pch[0]);
      return false;
    }
    if (buckets_[letter]) {
      snprintf(errorBuffer, errorBufferSize, "%s: %c",
               Str::DUPLICATE_CODE_LETTER_ERROR, pch[0]);
      return false;
    }
    buckets_[letter] = pch + 1;
    pch = strtok(NULL, " ");
  }
  return true;
}

bool GcodeParser::interpret(const uint16_t errorBufferSize, char *errorBuffer) {
  // Check for a 'G' code
  int32_t gcode;
  if (parseInt('G', &gcode, 0, nullptr)) {
    switch (gcode) {
      case 0:
      case 1:
        return true;
      default:
        snprintf(errorBuffer, errorBufferSize, "%s: %d",
                 Str::INVALID_G_CODE_ERROR, gcode);
        return false;
    }
  }
  snprintf(errorBuffer, errorBufferSize, "%s", Str::MISSING_COMMAND_CODE_ERROR);
  return false;
}

bool GcodeParser::parseInt(const char code, int32_t *result,
                           const uint16_t errorBufferSize, char *errorBuffer) {
  if ('A' <= code && code <= 'Z') {
    const char *str = buckets_[code - 'A'];
    if (str && str[0]) {
      // TODO: check if the int is actually valid
      *result = atol(str);
      return true;
    } else if (errorBuffer) {
      snprintf(errorBuffer, errorBufferSize, "%s: %c",
               Str::UNDEFINED_CODE_LETTER_ERROR, code);
    }
  }
  *result = 0;
  return false;
}
}  // namespace Clef::Fw
