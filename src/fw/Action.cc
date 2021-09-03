// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Action.h"

namespace Clef::Fw {
namespace Action {
Action::Action(const Type type, const Axes::XYZEPosition &startPosition)
    : type_(type), endPosition_(startPosition) {}

Axes::XYZEPosition Action::getEndPosition() const { return endPosition_; }

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

void MoveXY::onPush(Context &context) {
  context.axes.getX().acquire();
  context.axes.getY().acquire();
}

void MoveXY::onPop(Context &context) {
  context.axes.getX().release();
  context.axes.getY().release();
}

void MoveXY::onStart(Context &context) {
  const Axes::XYZEPosition &endPosition = getEndPosition();
  const Axes::XYZEPosition difference =
      endPosition - context.actionQueue.getStartPosition();
  const float magnitude = difference.magnitude();
  context.axes.getX().setFeedrate(context.axes.getFeedrate() *
                                  fabs(*difference.x / magnitude));
  context.axes.getY().setFeedrate(context.axes.getFeedrate() *
                                  fabs(*difference.y / magnitude));
  context.axes.getX().setTargetPosition(endPosition.x);
  context.axes.getY().setTargetPosition(endPosition.y);
}

bool MoveXY::isFinished(Context &context) {
  return context.axes.getX().isAtTargetPosition() &&
         context.axes.getY().isAtTargetPosition();
}

MoveXYE::MoveXYE(const Axes::XYZEPosition &startPosition)
    : Action(Type::MOVE_XYE, startPosition), numPoints_(0) {}

bool MoveXYE::pushPoint(Context &context,
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
    bool output =
        context.xyePositionQueue.push(tempEndPosition.asXyePosition());
    endPosition_ = tempEndPosition;
    context.actionQueue.updateXyeSegment(*this);
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
}  // namespace Action

ActionQueue::ActionQueue()
    : PooledQueue(), startPosition_({0, 0, 0, 0}), endPosition_({0, 0, 0, 0}) {}

Axes::XYZEPosition ActionQueue::getStartPosition() const {
  return startPosition_;
}

Axes::XYZEPosition ActionQueue::getEndPosition() const { return endPosition_; }

void ActionQueue::updateXyeSegment(const Action::MoveXYE &moveXye) {
  endPosition_ = moveXye.getEndPosition();
}

bool ActionQueue::checkConservation() const {
  return size() ==
         (moveXyQueue_.size() + moveXyeQueue_.size() + moveEQueue_.size() +
          moveZQueue_.size() + setFeedrateQueue_.size());
}

bool ActionQueue::push(Action::Action *const action) {
  return PooledQueue::push(action);
}

void ActionQueue::pop() { PooledQueue::pop(); }
}  // namespace Clef::Fw
