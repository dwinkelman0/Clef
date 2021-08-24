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
  XPosition x;
  YPosition y;
  EPosition e;
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
  XPosition x;
  YPosition y;
  ZPosition z;
  EPosition e;

  XYEPosition asXyePosition() const;
};

class XYZEPositionQueue : public Clef::Util::PooledQueue<XYZEPosition, 16> {};

extern const XYEPosition originXye;
extern const XYZEPosition originXyze;

namespace Action {
enum class ActionType {
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
 * action implements this interface.
 */
class Action {
 public:
  static const ActionType type = ActionType::NULL_ACTION;
  Action(const XYZEPosition *const startPosition);
  XYZEPosition getEndPosition() const;

 protected:
  XYZEPosition endPosition_;
};

class MoveXY : public Action {
 public:
  static const ActionType type = ActionType::MOVE_XY;
  MoveXY(const XYZEPosition *const startPosition,
         const Clef::If::XAxis::Position<float, Clef::Util::PositionUnit::MM>
             *const endPositionX,
         const Clef::If::YAxis::Position<float, Clef::Util::PositionUnit::MM>
             *const endPositionY);
};

class MoveXYE : public Action {
 public:
  static const ActionType type = ActionType::MOVE_XYE;
  MoveXYE(const XYZEPosition *const startPosition);

  /**
   * Add a point in the extrusion path; returns false if there was not room in
   * the queue to add another point.
   */
  bool pushPoint(
      XYEPositionQueue &xyePositionQueue,
      const Clef::If::XAxis::Position<float, Clef::Util::PositionUnit::MM>
          *const endPositionX,
      const Clef::If::YAxis::Position<float, Clef::Util::PositionUnit::MM>
          *const endPositionY,
      const Clef::If::EAxis::Position<float, Clef::Util::PositionUnit::MM>
          *const endPositionE);
};

class MoveE : public Action {
 public:
  static const ActionType type = ActionType::MOVE_E;
  MoveE(const XYZEPosition *const startPosition,
        const Clef::If::EAxis::Position<float, Clef::Util::PositionUnit::MM>
            endPositionE);
};

class MoveZ : public Action {
 public:
  static const ActionType type = ActionType::MOVE_Z;
  MoveZ(const XYZEPosition *const startPosition,
        const Clef::If::ZAxis::Position<float, Clef::Util::PositionUnit::MM>
            endPositionZ);
};

class SetFeedrate : public Action {
 public:
  static const ActionType type = ActionType::SET_FEEDRATE;
  SetFeedrate(const XYZEPosition *const startPosition,
              const float rawFeedrateMMs);

 private:
  float rawFeedrateMMs_;
};

class ActionVariant {
  ActionVariant(Action &action);
  Action *operator->();

 private:
  union Variants {
    Variants(Action &action);
    MoveXY moveXy;
    MoveXYE moveXye;
    MoveE moveE;
    MoveZ moveZ;
    SetFeedrate setFeedrate;
  };
  Variants variants_;
  ActionType type_;
};
}  // namespace Action
}  // namespace Clef::Fw
