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
STRING(INSUFFICIENT_QUEUE_CAPACITY_ERROR, "alloc_error");
}  // namespace Str

GcodeParser::GcodeParser(Clef::If::RWSerial &serial,
                         Clef::Fw::ActionQueue &actionQueue,
                         Clef::Fw::XYEPositionQueue &xyePositionQueue)
    : serial_(serial),
      actionQueue_(actionQueue),
      xyePositionQueue_(xyePositionQueue) {
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
        // Flush the buffer until a new line is detected
        while ((newChar = serial_.read(&newChar))) {
          if (newChar == '\n') {
            break;
          }
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

bool GcodeParser::parse(const uint16_t errorBufferSize,
                        char *const errorBuffer) {
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

bool GcodeParser::interpret(const uint16_t errorBufferSize,
                            char *const errorBuffer) {
  // Check for a 'G' code
  int32_t gcode;
  if (parseInt('G', &gcode, 0, nullptr)) {
    switch (gcode) {
      case 0:
      case 1:
        return handleG1(errorBufferSize, errorBuffer);
      default:
        snprintf(errorBuffer, errorBufferSize, "%s: %d",
                 Str::INVALID_G_CODE_ERROR, gcode);
        return false;
    }
  }
  snprintf(errorBuffer, errorBufferSize, "%s", Str::MISSING_COMMAND_CODE_ERROR);
  return false;
}

bool GcodeParser::hasCodeLetter(const char code) const {
  return 'A' <= code && code <= 'Z' && buckets_[code - 'A'];
}

bool GcodeParser::parseInt(const char code, int32_t *const result,
                           const uint16_t errorBufferSize,
                           char *const errorBuffer) const {
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

bool GcodeParser::parseFloat(const char code, float *const result,
                             const uint16_t errorBufferSize,
                             char *const errorBuffer) const {
  if ('A' <= code && code <= 'Z') {
    const char *str = buckets_[code - 'A'];
    if (str && str[0]) {
      // TODO: check if the float is actually valid
      *result = atof(str);
      return true;
    } else if (errorBuffer) {
      snprintf(errorBuffer, errorBufferSize, "%s: %c",
               Str::UNDEFINED_CODE_LETTER_ERROR, code);
    }
  }
  *result = 0;
  return false;
}

bool GcodeParser::handleG1(const uint16_t errorBufferSize,
                           char *const errorBuffer) {
  float x, y, z, e, f;
  bool hasX, hasY, hasZ, hasE, hasF;

  // Pre-process all parameters
  if ((hasX = hasCodeLetter('X')) &&
      !parseFloat('X', &x, errorBufferSize, errorBuffer)) {
    return false;
  }
  if ((hasY = hasCodeLetter('Y')) &&
      !parseFloat('Y', &y, errorBufferSize, errorBuffer)) {
    return false;
  }
  if ((hasZ = hasCodeLetter('Z')) &&
      !parseFloat('Z', &z, errorBufferSize, errorBuffer)) {
    return false;
  }
  if ((hasE = hasCodeLetter('E')) &&
      !parseFloat('E', &e, errorBufferSize, errorBuffer)) {
    return false;
  }
  if ((hasF = hasCodeLetter('F')) &&
      !parseFloat('F', &f, errorBufferSize, errorBuffer)) {
    return false;
  }

  // Determine number of action and xyePosition slots to allocate
  uint8_t numActions = static_cast<uint8_t>(hasF) + static_cast<uint8_t>(hasZ) +
                       static_cast<uint8_t>(hasX || hasY || hasZ);
  if (actionQueue_.getNumSpacesLeft() < numActions ||
      (hasE && (hasX || hasY) && xyePositionQueue_.getNumSpacesLeft() == 0)) {
    snprintf(errorBuffer, errorBufferSize, "%s",
             Str::INSUFFICIENT_QUEUE_CAPACITY_ERROR);
    return false;
  }

  // Enqueue actions
  if (hasF) {
    actionQueue_.push(
        Clef::Fw::Action::SetFeedrate(actionQueue_.getEndPosition(), f));
  }
  if (hasZ) {
    actionQueue_.push(
        Clef::Fw::Action::MoveZ(actionQueue_.getEndPosition(), z));
  }
  if (hasX || hasY) {
    Clef::If::XAxis::Position<float, Clef::Util::PositionUnit::MM> xMms(x);
    Clef::If::YAxis::Position<float, Clef::Util::PositionUnit::MM> yMms(y);
    if (hasE) {
      ActionQueue::Iterator lastAction = actionQueue_.last();
      Clef::If::EAxis::Position<float, Clef::Util::PositionUnit::MM> eMms(e);
      if (lastAction &&
          (*lastAction)->getType() == Clef::Fw::Action::Type::MOVE_XYE) {
        // If the last action in the queue is MoveXYE, coalesce
        lastAction->getVariant().moveXye.pushPoint(
            actionQueue_, xyePositionQueue_, hasX ? &xMms : nullptr,
            hasY ? &yMms : nullptr, eMms);
      } else {
        // Otherwise, start a new MoveXYE
        Clef::Fw::Action::MoveXYE moveXye(actionQueue_.getEndPosition());
        moveXye.pushPoint(actionQueue_, xyePositionQueue_,
                          hasX ? &xMms : nullptr, hasY ? &yMms : nullptr, eMms);
        if (moveXye.getNumPoints() > 0) {
          actionQueue_.push(moveXye);
        }
      }
    } else {
      actionQueue_.push(Clef::Fw::Action::MoveXY(actionQueue_.getEndPosition(),
                                                 hasX ? &xMms : nullptr,
                                                 hasY ? &yMms : nullptr));
    }
  } else if (hasE) {
    actionQueue_.push(
        Clef::Fw::Action::MoveE(actionQueue_.getEndPosition(), e));
  }
  return true;
}
}  // namespace Clef::Fw
