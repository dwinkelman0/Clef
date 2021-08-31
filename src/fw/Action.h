// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <fw/Axes.h>
#include <if/Clock.h>
#include <if/Serial.h>
#include <util/PooledQueue.h>
#include <util/Units.h>

namespace Clef::Fw {
class XYEPositionQueue
    : public Clef::Util::PooledQueue<Axes::XYEPosition, 128> {};

class ActionQueue;
class GcodeParser;

struct Context {
  Clef::Fw::Axes &axes;
  Clef::Fw::GcodeParser &gcodeParser;
  Clef::Fw::ActionQueue &actionQueue;
  Clef::Fw::XYEPositionQueue &xyePositionQueue;
  Clef::If::Clock &clock;
  Clef::If::RWSerial &serial;
};

namespace Action {
enum class Type {
  NULL_ACTION,
  MOVE_XY,
  MOVE_XYE,
  MOVE_E,
  MOVE_Z,
  HOME_XY,
  HOME_Z,
  HOME_E,
  SET_FEEDRATE
};

/**
 * Data structure to store parameters for commands the printer to execute; every
 * action implements this interface. The type of the action is stored in memory
 * so that derivatives of this class can be in a union.
 */
class Action {
 public:
  Action(const Type type, const Axes::XYZEPosition &startPosition);
  Type getType() const;
  Axes::XYZEPosition getEndPosition() const;

  virtual void onStart(Context &context) = 0;

 protected:
  Type type_;
  Axes::XYZEPosition endPosition_;
};

class Null : public Action {
 public:
  Null();

  void onStart(Context &context) override {}
};

class MoveXY : public Action {
 public:
  MoveXY(const Axes::XYZEPosition &startPosition,
         const Axes::XAxis::GcodePosition *const endPositionX,
         const Axes::YAxis::GcodePosition *const endPositionY);

  void onStart(Context &context) override {}
};

class MoveXYE : public Action {
 public:
  MoveXYE(const Axes::XYZEPosition &startPosition);

  /**
   * Add a point in the extrusion path; returns false if there was not room in
   * the queue to add another point.
   */
  bool pushPoint(ActionQueue &actionQueue, XYEPositionQueue &xyePositionQueue,
                 const Axes::XAxis::GcodePosition *const endPositionX,
                 const Axes::YAxis::GcodePosition *const endPositionY,
                 const Axes::EAxis::GcodePosition endPositionE);
  uint16_t getNumPoints() const;

  void onStart(Context &context) override {}

 private:
  uint16_t numPoints_;
};

class MoveE : public Action {
 public:
  MoveE(const Axes::XYZEPosition &startPosition,
        const Axes::EAxis::GcodePosition endPositionE);

  void onStart(Context &context) override {}
};

class MoveZ : public Action {
 public:
  MoveZ(const Axes::XYZEPosition &startPosition,
        const Axes::ZAxis::GcodePosition endPositionZ);

  void onStart(Context &context) override {}
};

class SetFeedrate : public Action {
 public:
  SetFeedrate(const Axes::XYZEPosition &startPosition,
              const float rawFeedrateMMs);

  void onStart(Context &context) override {}

 private:
  float rawFeedrateMMs_;
};

class ActionVariant {
 public:
  union Variants {
    Variants() {}
    Null null;
    MoveXY moveXy;
    MoveXYE moveXye;
    MoveE moveE;
    MoveZ moveZ;
    SetFeedrate setFeedrate;
  };

  ActionVariant();
  ActionVariant(const Action &action);
  ActionVariant &operator=(const ActionVariant &other);

  Action &getAction();
  const Action &getAction() const;
  Variants &getVariant();
  const Variants &getVariant() const;
  Type getType() const;

 private:
  Variants variants_;
  Type type_;

  static Null theNullAction_;
};
}  // namespace Action

class ActionQueue : public Clef::Util::PooledQueue<Action::ActionVariant, 16> {
 public:
  bool push(const Action::Action &action);
  Axes::XYZEPosition getEndPosition() const;

  /**
   * If a point is added to an XYE segment, this queue needs to know about it so
   * it can update endPosition_.
   */
  void updateXyeSegment(const Action::MoveXYE &moveXye);

 private:
  bool push(const Action::ActionVariant &action);

 private:
  Axes::XYZEPosition
      endPosition_; /*!< Remember end position of the last action. */
};
}  // namespace Clef::Fw
