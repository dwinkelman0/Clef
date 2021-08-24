// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Action.h"

namespace Clef::Fw {
XYEPosition XYZEPosition::asXyePosition() const { return {x, y, e}; }

const XYEPosition originXye = {0, 0, 0};
const XYZEPosition originXyze = {0, 0, 0, 0};

namespace Action {
Action::Action(const XYZEPosition *const startPosition)
    : endPosition_(*startPosition) {}

XYZEPosition Action::getEndPosition() const { return endPosition_; }

MoveXY::MoveXY(
    const XYZEPosition *const startPosition,
    const Clef::If::XAxis::Position<float, Clef::Util::PositionUnit::MM>
        *const endPositionX,
    const Clef::If::YAxis::Position<float, Clef::Util::PositionUnit::MM>
        *const endPositionY)
    : Action(startPosition) {
  if (endPositionX) {
    endPosition_.x = XYZEPosition::XPosition(static_cast<int32_t>(
        *Clef::If::XAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
            *endPositionX)));
  }
  if (endPositionY) {
    endPosition_.y = XYZEPosition::YPosition(static_cast<int32_t>(
        *Clef::If::YAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
            *endPositionY)));
  }
}

MoveXYE::MoveXYE(const XYZEPosition *const startPosition)
    : Action(startPosition) {}

bool MoveXYE::pushPoint(
    XYEPositionQueue &xyePositionQueue,
    const Clef::If::XAxis::Position<float, Clef::Util::PositionUnit::MM>
        *const endPositionX,
    const Clef::If::YAxis::Position<float, Clef::Util::PositionUnit::MM>
        *const endPositionY,
    const Clef::If::EAxis::Position<float, Clef::Util::PositionUnit::MM>
        *const endPositionE) {
  // Update end position but do not commit to state until a point is
  // successfully pushed to the queue.
  XYZEPosition tempEndPosition = getEndPosition();
  if (endPositionX) {
    tempEndPosition.x = XYZEPosition::XPosition(static_cast<int32_t>(
        *Clef::If::XAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
            *endPositionX)));
  }
  if (endPositionY) {
    tempEndPosition.y = XYZEPosition::YPosition(static_cast<int32_t>(
        *Clef::If::YAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
            *endPositionY)));
  }
  if (endPositionE) {
    tempEndPosition.e = XYZEPosition::EPosition(static_cast<int32_t>(
        *Clef::If::EAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
            *endPositionE)));
  }
  bool output = xyePositionQueue.push(tempEndPosition.asXyePosition());
  endPosition_ = tempEndPosition;
  return output;
}

MoveE::MoveE(
    const XYZEPosition *const startPosition,
    const Clef::If::EAxis::Position<float, Clef::Util::PositionUnit::MM>
        endPositionE)
    : Action(startPosition) {
  endPosition_.e = XYZEPosition::EPosition(static_cast<int32_t>(
      *Clef::If::EAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
          *endPositionE)));
}

MoveZ::MoveZ(
    const XYZEPosition *const startPosition,
    const Clef::If::ZAxis::Position<float, Clef::Util::PositionUnit::MM>
        endPositionZ)
    : Action(startPosition) {
  endPosition_.z = XYZEPosition::ZPosition(static_cast<int32_t>(
      *Clef::If::ZAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
          *endPositionZ)));
}

SetFeedrate::SetFeedrate(const XYZEPosition *const startPosition,
                         const float rawFeedrateMMs)
    : Action(startPosition), rawFeedrateMMs_(rawFeedrateMMs) {}

ActionVariant::ActionVariant(Action &action)
    : variants_(action), type_(action.type) {}

Action *ActionVariant::operator->() {
  switch (type_) {
    case ActionType::MOVE_XY:
      return dynamic_cast<Action *>(&variants_.moveXy);
    case ActionType::MOVE_XYE:
      return dynamic_cast<Action *>(&variants_.moveXye);
    case ActionType::MOVE_E:
      return dynamic_cast<Action *>(&variants_.moveE);
    case ActionType::MOVE_Z:
      return dynamic_cast<Action *>(&variants_.moveZ);
    case ActionType::SET_FEEDRATE:
      return dynamic_cast<Action *>(&variants_.setFeedrate);
    default:
      return nullptr;
  }
}

ActionVariant::Variants::Variants(Action &action) {
  switch (action.type) {
    case ActionType::MOVE_XY:
      moveXy = static_cast<MoveXY &>(action);
      break;
    case ActionType::MOVE_XYE:
      moveXye = static_cast<MoveXYE &>(action);
      break;
    case ActionType::MOVE_E:
      moveE = static_cast<MoveE &>(action);
      break;
    case ActionType::MOVE_Z:
      moveZ = static_cast<MoveZ &>(action);
      break;
    case ActionType::SET_FEEDRATE:
      setFeedrate = static_cast<SetFeedrate &>(action);
      break;
  }
}
}  // namespace Action
}  // namespace Clef::Fw
