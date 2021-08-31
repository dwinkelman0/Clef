// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Action.h"

namespace Clef::Fw {
namespace Action {
Action::Action(const Type type, const Axes::XYZEPosition &startPosition)
    : type_(type), endPosition_(startPosition) {}

Type Action::getType() const { return type_; }

Axes::XYZEPosition Action::getEndPosition() const { return endPosition_; }

Null::Null() : Action(Type::NULL_ACTION, {0, 0, 0, 0}) {}

MoveXY::MoveXY(const Axes::XYZEPosition &startPosition,
               const Axes::XAxis::GcodePosition *const endPositionX,
               const Axes::YAxis::GcodePosition *const endPositionY)
    : Action(Type::MOVE_XY, startPosition) {
  if (endPositionX) {
    endPosition_.x = Axes::XYZEPosition::XPosition(static_cast<int32_t>(
        *Axes::XAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
            *endPositionX)));
  }
  if (endPositionY) {
    endPosition_.y = Axes::XYZEPosition::YPosition(static_cast<int32_t>(
        *Axes::YAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
            *endPositionY)));
  }
}

MoveXYE::MoveXYE(const Axes::XYZEPosition &startPosition)
    : Action(Type::MOVE_XYE, startPosition), numPoints_(0) {}

bool MoveXYE::pushPoint(ActionQueue &actionQueue,
                        XYEPositionQueue &xyePositionQueue,
                        const Axes::XAxis::GcodePosition *const endPositionX,
                        const Axes::YAxis::GcodePosition *const endPositionY,
                        const Axes::EAxis::GcodePosition endPositionE) {
  // Update end position but do not commit to state until a point is
  // successfully pushed to the queue.
  Axes::XYZEPosition tempEndPosition = getEndPosition();
  if (endPositionX) {
    tempEndPosition.x = Axes::XYZEPosition::XPosition(static_cast<int32_t>(
        *Axes::XAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
            *endPositionX)));
  }
  if (endPositionY) {
    tempEndPosition.y = Axes::XYZEPosition::YPosition(static_cast<int32_t>(
        *Axes::YAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
            *endPositionY)));
  }
  tempEndPosition.e = Axes::XYZEPosition::EPosition(static_cast<int32_t>(
      *Axes::EAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
          endPositionE)));
  if (tempEndPosition != getEndPosition()) {
    // Require that the point be distinct to prevent division by zero.
    bool output = xyePositionQueue.push(tempEndPosition.asXyePosition());
    endPosition_ = tempEndPosition;
    if (actionQueue.last() &&
        this == &actionQueue.last()->getVariant().moveXye) {
      actionQueue.updateXyeSegment(*this);
    }
    numPoints_++;
    return output;
  }
  return true;
}

uint16_t MoveXYE::getNumPoints() const { return numPoints_; }

MoveE::MoveE(const Axes::XYZEPosition &startPosition,
             const Axes::EAxis::GcodePosition endPositionE)
    : Action(Type::MOVE_E, startPosition) {
  endPosition_.e = Axes::XYZEPosition::EPosition(static_cast<int32_t>(
      *Axes::EAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
          endPositionE)));
}

MoveZ::MoveZ(const Axes::XYZEPosition &startPosition,
             const Axes::ZAxis::GcodePosition endPositionZ)
    : Action(Type::MOVE_Z, startPosition) {
  endPosition_.z = Axes::XYZEPosition::ZPosition(static_cast<int32_t>(
      *Axes::ZAxis::Position<float, Clef::Util::PositionUnit::USTEP>(
          endPositionZ)));
}

SetFeedrate::SetFeedrate(const Axes::XYZEPosition &startPosition,
                         const float rawFeedrateMMs)
    : Action(Type::SET_FEEDRATE, startPosition),
      rawFeedrateMMs_(rawFeedrateMMs) {}

ActionVariant::ActionVariant() : ActionVariant(theNullAction_) {}

ActionVariant::ActionVariant(const Action &action) : type_(action.getType()) {
  switch (action.getType()) {
    case Type::NULL_ACTION:
      variants_.null = static_cast<const Null &>(action);
      break;
    case Type::MOVE_XY:
      variants_.moveXy = static_cast<const MoveXY &>(action);
      break;
    case Type::MOVE_XYE:
      variants_.moveXye = static_cast<const MoveXYE &>(action);
      break;
    case Type::MOVE_E:
      variants_.moveE = static_cast<const MoveE &>(action);
      break;
    case Type::MOVE_Z:
      variants_.moveZ = static_cast<const MoveZ &>(action);
      break;
    case Type::SET_FEEDRATE:
      variants_.setFeedrate = static_cast<const SetFeedrate &>(action);
      break;
  }
}

ActionVariant &ActionVariant::operator=(const ActionVariant &other) {
  switch (other.type_) {
    case Type::NULL_ACTION:
      variants_.null = other.variants_.null;
      break;
    case Type::MOVE_XY:
      variants_.moveXy = other.variants_.moveXy;
      break;
    case Type::MOVE_XYE:
      variants_.moveXye = other.variants_.moveXye;
      break;
    case Type::MOVE_E:
      variants_.moveE = other.variants_.moveE;
      break;
    case Type::MOVE_Z:
      variants_.moveZ = other.variants_.moveZ;
      break;
    case Type::SET_FEEDRATE:
      variants_.setFeedrate = other.variants_.setFeedrate;
      break;
  }
  type_ = other.type_;
  return *this;
}

Action &ActionVariant::getAction() {
  return const_cast<Action &>(
      const_cast<const ActionVariant *>(this)->getAction());
}

const Action &ActionVariant::getAction() const {
  switch (type_) {
    case Type::NULL_ACTION:
      return variants_.null;
    case Type::MOVE_XY:
      return variants_.moveXy;
    case Type::MOVE_XYE:
      return variants_.moveXye;
    case Type::MOVE_E:
      return variants_.moveE;
    case Type::MOVE_Z:
      return variants_.moveZ;
    case Type::SET_FEEDRATE:
      return variants_.setFeedrate;
  }
}

ActionVariant::Variants &ActionVariant::getVariant() { return variants_; }

Type ActionVariant::getType() const { return type_; }

Null ActionVariant::theNullAction_;
}  // namespace Action

bool ActionQueue::push(const Action::Action &action) {
  if (push(Action::ActionVariant(action))) {
    endPosition_ = (&action)->getEndPosition();
    return true;
  }
  return false;
}

Axes::XYZEPosition ActionQueue::getEndPosition() const { return endPosition_; }

void ActionQueue::updateXyeSegment(const Action::MoveXYE &moveXye) {
  endPosition_ = moveXye.getEndPosition();
}

bool ActionQueue::push(const Action::ActionVariant &action) {
  return PooledQueue::push(action);
}
}  // namespace Clef::Fw
