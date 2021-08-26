// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Action.h"

namespace Clef::Fw {

bool XYEPosition::operator==(const XYEPosition &other) const {
  return x == other.x && y == other.y && e == other.e;
}

bool XYZEPosition::operator==(const XYZEPosition &other) const {
  return x == other.x && y == other.y && z == other.z && e == other.e;
}

XYEPosition XYZEPosition::asXyePosition() const { return {x, y, e}; }

const XYEPosition originXye = {0, 0, 0};
const XYZEPosition originXyze = {0, 0, 0, 0};

namespace Action {
Action::Action(const Type type, const XYZEPosition &startPosition)
    : type_(type), endPosition_(startPosition) {}

Type Action::getType() const { return type_; }

XYZEPosition Action::getEndPosition() const { return endPosition_; }

Null::Null() : Action(Type::NULL_ACTION, originXyze) {}

MoveXY::MoveXY(
    const XYZEPosition &startPosition,
    const Clef::If::XAxis::Position<float, Clef::Util::PositionUnit::MM>
        *const endPositionX,
    const Clef::If::YAxis::Position<float, Clef::Util::PositionUnit::MM>
        *const endPositionY)
    : Action(Type::MOVE_XY, startPosition) {
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

MoveXYE::MoveXYE(const XYZEPosition &startPosition)
    : Action(Type::MOVE_XYE, startPosition), numPoints_(0) {}

bool MoveXYE::pushPoint(
    ActionQueue &actionQueue, XYEPositionQueue &xyePositionQueue,
    const Clef::If::XAxis::Position<float, Clef::Util::PositionUnit::MM>
        *const endPositionX,
    const Clef::If::YAxis::Position<float, Clef::Util::PositionUnit::MM>
        *const endPositionY,
    const Clef::If::EAxis::Position<float, Clef::Util::PositionUnit::MM>
        endPositionE) {
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
  tempEndPosition.e = XYZEPosition::EPosition(static_cast<int32_t>(
      *Clef::If::EAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
          endPositionE)));
  bool output = xyePositionQueue.push(tempEndPosition.asXyePosition());
  endPosition_ = tempEndPosition;
  if (actionQueue.last() && this == &actionQueue.last()->getVariant().moveXye) {
    actionQueue.updateXyeSegment(*this);
  }
  numPoints_++;
  return output;
}

uint16_t MoveXYE::getNumPoints() const { return numPoints_; }

MoveE::MoveE(
    const XYZEPosition &startPosition,
    const Clef::If::EAxis::Position<float, Clef::Util::PositionUnit::MM>
        endPositionE)
    : Action(Type::MOVE_E, startPosition) {
  endPosition_.e = XYZEPosition::EPosition(static_cast<int32_t>(
      *Clef::If::EAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
          endPositionE)));
}

MoveZ::MoveZ(
    const XYZEPosition &startPosition,
    const Clef::If::ZAxis::Position<float, Clef::Util::PositionUnit::MM>
        endPositionZ)
    : Action(Type::MOVE_Z, startPosition) {
  endPosition_.z = XYZEPosition::ZPosition(static_cast<int32_t>(
      *Clef::If::ZAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
          endPositionZ)));
}

SetFeedrate::SetFeedrate(const XYZEPosition &startPosition,
                         const float rawFeedrateMMs)
    : Action(Type::SET_FEEDRATE, startPosition),
      rawFeedrateMMs_(rawFeedrateMMs) {}

ActionVariant::ActionVariant() : ActionVariant(theNullAction_) {}

ActionVariant::ActionVariant(const Action &action)
    : variants_(action), type_(action.getType()) {}

ActionVariant::Variants &ActionVariant::getVariant() { return variants_; }

const ActionVariant::Variants &ActionVariant::getVariant() const {
  return variants_;
}

Action &ActionVariant::operator*() { return *(operator->()); }

const Action &ActionVariant::operator*() const { return *(operator->()); }

Action *ActionVariant::operator->() {
  switch (type_) {
    case Type::NULL_ACTION:
      return nullptr;
    case Type::MOVE_XY:
      return dynamic_cast<Action *>(&variants_.moveXy);
    case Type::MOVE_XYE:
      return dynamic_cast<Action *>(&variants_.moveXye);
    case Type::MOVE_E:
      return dynamic_cast<Action *>(&variants_.moveE);
    case Type::MOVE_Z:
      return dynamic_cast<Action *>(&variants_.moveZ);
    case Type::SET_FEEDRATE:
      return dynamic_cast<Action *>(&variants_.setFeedrate);
    default:
      return nullptr;
  }
}

const Action *ActionVariant::operator->() const {
  switch (type_) {
    case Type::NULL_ACTION:
      return nullptr;
    case Type::MOVE_XY:
      return dynamic_cast<const Action *>(&variants_.moveXy);
    case Type::MOVE_XYE:
      return dynamic_cast<const Action *>(&variants_.moveXye);
    case Type::MOVE_E:
      return dynamic_cast<const Action *>(&variants_.moveE);
    case Type::MOVE_Z:
      return dynamic_cast<const Action *>(&variants_.moveZ);
    case Type::SET_FEEDRATE:
      return dynamic_cast<const Action *>(&variants_.setFeedrate);
    default:
      return nullptr;
  }
}

ActionVariant::Variants::Variants(const Action &action) {
  switch (action.getType()) {
    case Type::NULL_ACTION:
      null = static_cast<const Null &>(action);
      break;
    case Type::MOVE_XY:
      moveXy = static_cast<const MoveXY &>(action);
      break;
    case Type::MOVE_XYE:
      moveXye = static_cast<const MoveXYE &>(action);
      break;
    case Type::MOVE_E:
      moveE = static_cast<const MoveE &>(action);
      break;
    case Type::MOVE_Z:
      moveZ = static_cast<const MoveZ &>(action);
      break;
    case Type::SET_FEEDRATE:
      setFeedrate = static_cast<const SetFeedrate &>(action);
      break;
  }
}

Null ActionVariant::theNullAction_;
}  // namespace Action

bool ActionQueue::push(const Action::Action &action) {
  if (push(Action::ActionVariant(action))) {
    endPosition_ = (&action)->getEndPosition();
    return true;
  }
  return false;
}

XYZEPosition ActionQueue::getEndPosition() const { return endPosition_; }

void ActionQueue::updateXyeSegment(const Action::MoveXYE &moveXye) {
  endPosition_ = moveXye.getEndPosition();
}

bool ActionQueue::push(const Action::ActionVariant &action) {
  return PooledQueue::push(action);
}
}  // namespace Clef::Fw
