// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "GcodeParser.h"

#include <if/Memory.h>
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
STRING(INVALID_M_CODE_ERROR, "invalid_m_code_error");
STRING(INSUFFICIENT_QUEUE_CAPACITY_ERROR, "alloc_error");
STRING(MISSING_ARGUMENT_ERROR, "missing_argument_error");
STRING(INVALID_ARGUMENT_ERROR, "invalid_argument_error");
}  // namespace Str

GcodeParser::GcodeParser() { reset(); }

void GcodeParser::ingest(Context &context) {
  const uint16_t errorBufferSize = 64;
  char errorBuffer[errorBufferSize];
  char newChar;
  while (context.serial.read(&newChar)) {
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
          if (interpret(context, errorBufferSize, errorBuffer)) {
            context.serial.writeLine(Str::OK);
          } else {
            context.serial.writeLine(errorBuffer);
          }
        }
      } else {
        context.serial.writeLine(errorBuffer);
      }
      reset();
    } else if (newChar == ';') {
      // If there is a semicolon, ignore everything until the next new line.
      commentMode_ = true;
    } else if (!commentMode_) {
      // Add the char to the buffer
      if (!append(newChar)) {
        // Flush the buffer until a new line is detected
        while ((newChar = context.serial.read(&newChar))) {
          if (newChar == '\n') {
            break;
          }
        }
        context.serial.writeLine(Str::BUFFER_OVERFLOW_ERROR);
        reset();
      }
    }
  }
}

void GcodeParser::reset() {
  head_ = buffer_;
  *head_ = '\0';
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

bool GcodeParser::interpret(Context &context, const uint16_t errorBufferSize,
                            char *const errorBuffer) {
  // Check for a 'G' code
  int32_t gcode, mcode;
  if (parseInt('G', &gcode, 0, nullptr)) {
    switch (gcode) {
      case 0:
      case 1:
        return handleG1(context, errorBufferSize, errorBuffer);
      default:
        snprintf(errorBuffer, errorBufferSize, "%s: %d",
                 Str::INVALID_G_CODE_ERROR, gcode);
        return false;
    }
  } else if (parseInt('M', &mcode, 0, nullptr)) {
    switch (mcode) {
      case 104:
        return handleM104(context, errorBufferSize, errorBuffer);
      case 116:
        return handleM116(context, errorBufferSize, errorBuffer);
      default:
        snprintf(errorBuffer, errorBufferSize, "%s: %d",
                 Str::INVALID_M_CODE_ERROR);
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

bool GcodeParser::handleG1(Context &context, const uint16_t errorBufferSize,
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
  if (context.actionQueue.getNumSpacesLeft() < numActions ||
      (hasZ && !context.actionQueue.hasCapacityFor(Action::Type::MOVE_E)) ||
      (hasE && !context.actionQueue.hasCapacityFor(Action::Type::MOVE_E)) ||
      (hasF &&
       !context.actionQueue.hasCapacityFor(Action::Type::SET_FEEDRATE)) ||
      (((hasX || hasY) && !hasE) &&
       !context.actionQueue.hasCapacityFor(Action::Type::MOVE_XY)) ||
      (((hasX || hasY) && hasE) &&
       (!context.actionQueue.hasCapacityFor(Action::Type::MOVE_XYE) ||
        context.xyePositionQueue.getNumSpacesLeft() == 0))) {
    snprintf(errorBuffer, errorBufferSize, "%s",
             Str::INSUFFICIENT_QUEUE_CAPACITY_ERROR);
    return false;
  }

  // Enqueue actions
  if (hasF) {
    context.actionQueue.push(
        context, Action::SetFeedrate(context.actionQueue.getEndPosition(), f));
  }
  if (hasZ) {
    context.actionQueue.push(
        context, Action::MoveZ(context.actionQueue.getEndPosition(), z));
  }
  if (hasX || hasY) {
    Axes::XAxis::GcodePosition xMms(x);
    Axes::YAxis::GcodePosition yMms(y);
    if (hasE) {
      ActionQueue::Iterator lastAction = context.actionQueue.last();
      Axes::EAxis::GcodePosition eMms(e);
      if (lastAction && (*lastAction)->getType() == Action::Type::MOVE_XYE &&
          static_cast<Action::MoveXYE *>(*lastAction)
              ->checkNewPointDirection(eMms)) {
        // If the last action in the queue is MoveXYE and the extrusion
        // destination is in the same direction as the current extrusion,
        // coalesce
        context.serial.writeLine(";Push XYE point");
        static_cast<Action::MoveXYE *>(*lastAction)
            ->pushPoint(context, hasX ? &xMms : nullptr, hasY ? &yMms : nullptr,
                        eMms);
      } else {
        // Otherwise, start a new MoveXYE
        context.serial.writeLine(";Push XYE fresh");
        Action::MoveXYE moveXye(context.actionQueue.getEndPosition());
        moveXye.pushPoint(context, hasX ? &xMms : nullptr,
                          hasY ? &yMms : nullptr, eMms);
        if (moveXye.getNumPointsPushed() > 0) {
          context.actionQueue.push(context, moveXye);
        }
      }
    } else {
      context.serial.writeLine(";Push XY");
      context.actionQueue.push(
          context,
          Action::MoveXY(context.actionQueue.getEndPosition(),
                         hasX ? &xMms : nullptr, hasY ? &yMms : nullptr));
    }
  } else if (hasE) {
    context.serial.writeLine(";Push E");
    context.actionQueue.push(
        context, Action::MoveE(context.actionQueue.getEndPosition(), e));
  }
  return true;
}

bool GcodeParser::handleM104(Context &context, const uint16_t errorBufferSize,
                             char *const errorBuffer) {
  int32_t p; /*!< Group (i.e. bed = 0, first extruder = 1, etc.). */
  int32_t a; /*!< Required if P != 0; 0 is syringe, 1 is needle. */
  float s;   /*!< Target temperature. */
  bool hasP, hasA, hasS;

  // Pre-process all parameters
  if ((hasP = hasCodeLetter('P')) &&
      !parseInt('P', &p, errorBufferSize, errorBuffer)) {
    return false;
  }
  if ((hasA = hasCodeLetter('A')) &&
      !parseInt('A', &a, errorBufferSize, errorBuffer)) {
    return false;
  }
  if ((hasS = hasCodeLetter('S')) &&
      !parseFloat('S', &s, errorBufferSize, errorBuffer)) {
    return false;
  }

  // Must have P and S or this is an invalid command
  if (!hasP || !hasS) {
    snprintf(errorBuffer, errorBufferSize, "%s: no P and/or S",
             Str::MISSING_ARGUMENT_ERROR);
    return false;
  } else if (p > 1) {
    snprintf(errorBuffer, errorBufferSize,
             "%s: P greater than number of extruders",
             Str::INVALID_ARGUMENT_ERROR);
  }
  if (p == 0 && hasA) {
    snprintf(errorBuffer, errorBufferSize, "%s: not expecting A when P == 0",
             Str::INVALID_ARGUMENT_ERROR);
    return false;
  }

  // TODO: support bed heating
  if (p == 0) {
    snprintf(errorBuffer, errorBufferSize, "%s: bed heating is not supported",
             Str::INVALID_ARGUMENT_ERROR);
    return false;
  }

  // Check for space in the queue
  uint16_t numActions = static_cast<uint16_t>(!hasA || a == 0) +
                        static_cast<uint16_t>(!hasA || a == 1);
  if (context.actionQueue.getCapacityFor(Action::Type::SET_TEMP) < numActions) {
    snprintf(errorBuffer, errorBufferSize, "%s",
             Str::INSUFFICIENT_QUEUE_CAPACITY_ERROR);
    return false;
  }

  if (!hasA || a == 0) {
    Action::SetTemp action(context.actionQueue.getEndPosition(),
                           &context.axes.getE().getSyringeHeater(), s);
    context.actionQueue.push(context, action);
    context.serial.writeLine(";set syringe temp");
  }
  if (!hasA || a == 1) {
    Action::SetTemp action(context.actionQueue.getEndPosition(),
                           &context.axes.getE().getNeedleHeater(), s);
    context.actionQueue.push(context, action);
    context.serial.writeLine(";set needle temp");
  }
  return true;
}

bool GcodeParser::handleM116(Context &context, const uint16_t errorBufferSize,
                             char *const errorBuffer) {
  if (context.actionQueue.getCapacityFor(Action::Type::WAIT_FOR) < 1) {
    snprintf(errorBuffer, errorBufferSize, "%s",
             Str::INSUFFICIENT_QUEUE_CAPACITY_ERROR);
    return false;
  }
  Action::WaitFor action(context.actionQueue.getEndPosition(),
                         Action::WaitFor::temperaturesHaveReachedTargets,
                         &context);
  context.actionQueue.push(context, action);
  context.serial.writeLine(";wait for temps");
  return true;
}
}  // namespace Clef::Fw
