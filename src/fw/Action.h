// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/Axis.h>
#include <util/PooledQueue.h>
#include <util/Units.h>

namespace Clef::Fw {
struct XYEPosition {
  using XPosition =
      Clef::If::XAxis::Position<int32_t, Clef::Util::PositionUnit::USTEP>;
  using YPosition =
      Clef::If::YAxis::Position<int32_t, Clef::Util::PositionUnit::USTEP>;
  using EPosition =
      Clef::If::EAxis::Position<int32_t, Clef::Util::PositionUnit::USTEP>;
  XPosition x = 0;
  YPosition y = 0;
  EPosition e = 0;
};

class XYEPositionQueue : public Clef::Util::PooledQueue<XYEPosition, 128> {};

struct XYZEPosition {
  using XPosition =
      Clef::If::XAxis::Position<int32_t, Clef::Util::PositionUnit::USTEP>;
  using YPosition =
      Clef::If::YAxis::Position<int32_t, Clef::Util::PositionUnit::USTEP>;
  using ZPosition =
      Clef::If::ZAxis::Position<int32_t, Clef::Util::PositionUnit::USTEP>;
  using EPosition =
      Clef::If::EAxis::Position<int32_t, Clef::Util::PositionUnit::USTEP>;
  XPosition x = 0;
  YPosition y = 0;
  ZPosition z = 0;
  EPosition e = 0;

  XYEPosition asXyePosition() const;
};

extern const XYEPosition originXye;
extern const XYZEPosition originXyze;

class ActionQueue;

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
  Action(const Type type, const XYZEPosition &startPosition);
  Type getType() const;
  XYZEPosition getEndPosition() const;

 protected:
  Type type_;
  XYZEPosition endPosition_;
};

class Null : public Action {
 public:
  Null();
};

class MoveXY : public Action {
 public:
  MoveXY(const XYZEPosition &startPosition,
         const Clef::If::XAxis::Position<float, Clef::Util::PositionUnit::MM>
             *const endPositionX,
         const Clef::If::YAxis::Position<float, Clef::Util::PositionUnit::MM>
             *const endPositionY);
};

class MoveXYE : public Action {
 public:
  MoveXYE(const XYZEPosition &startPosition);

  /**
   * Add a point in the extrusion path; returns false if there was not room in
   * the queue to add another point.
   */
  bool pushPoint(
      ActionQueue &actionQueue, XYEPositionQueue &xyePositionQueue,
      const Clef::If::XAxis::Position<float, Clef::Util::PositionUnit::MM>
          *const endPositionX,
      const Clef::If::YAxis::Position<float, Clef::Util::PositionUnit::MM>
          *const endPositionY,
      const Clef::If::EAxis::Position<float, Clef::Util::PositionUnit::MM>
          endPositionE);
  uint16_t getNumPoints() const;

 private:
  uint16_t numPoints_;
};

class MoveE : public Action {
 public:
  MoveE(const XYZEPosition &startPosition,
        const Clef::If::EAxis::Position<float, Clef::Util::PositionUnit::MM>
            endPositionE);
};

class MoveZ : public Action {
 public:
  MoveZ(const XYZEPosition &startPosition,
        const Clef::If::ZAxis::Position<float, Clef::Util::PositionUnit::MM>
            endPositionZ);
};

class SetFeedrate : public Action {
 public:
  SetFeedrate(const XYZEPosition &startPosition, const float rawFeedrateMMs);

 private:
  float rawFeedrateMMs_;
};

class ActionVariant {
 public:
  union Variants {
    Variants(const Action &action);
    Null null;
    MoveXY moveXy;
    MoveXYE moveXye;
    MoveE moveE;
    MoveZ moveZ;
    SetFeedrate setFeedrate;
  };

  ActionVariant();
  ActionVariant(const Action &action);
  Variants &getVariant();
  const Variants &getVariant() const;
  Action &operator*();
  const Action &operator*() const;
  Action *operator->();
  const Action *operator->() const;

 private:
  Variants variants_;
  Type type_;

  static Null theNullAction_;
};
}  // namespace Action

class ActionQueue : public Clef::Util::PooledQueue<Action::ActionVariant, 16> {
 public:
  bool push(const Action::Action &action);
  XYZEPosition getEndPosition() const;

  /**
   * If a point is added to an XYE segment, this queue needs to know about it so
   * it can update endPosition_.
   */
  void updateXyeSegment(const Action::MoveXYE &moveXye);

 private:
  bool push(const Action::ActionVariant &action);

 private:
  XYZEPosition endPosition_; /*!< Remember end position of the last action. */
};
}  // namespace Clef::Fw