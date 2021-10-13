// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Action.h"

namespace Clef::Fw {
namespace Action {
Action::Action(const Type type, const XYZEPosition &startPosition)
    : type_(type), endPosition_(startPosition) {}

XYZEPosition Action::getEndPosition() const { return endPosition_; }

MoveXY::MoveXY(const XYZEPosition &startPosition,
               const Axes::XAxis::GcodePosition *const endPositionX,
               const Axes::YAxis::GcodePosition *const endPositionY)
    : Action(Type::MOVE_XY, startPosition) {
  if (endPositionX) {
    endPosition_.x = *endPositionX;
  }
  if (endPositionY) {
    endPosition_.y = *endPositionY;
  }
}

void MoveXY::onStart(Context &context) {
  context.axes.setXyParams(
      context.actionQueue.getStartPosition().asXyePosition(),
      getEndPosition().asXyePosition(), context.axes.getFeedrate());
}

bool MoveXY::isFinished(const Context &context) const {
  return context.axes.getX().isAtTargetPosition() &&
         context.axes.getY().isAtTargetPosition();
}

void MoveXY::onPush(Context &context) {
  context.axes.getX().acquire();
  context.axes.getY().acquire();
}

void MoveXY::onPop(Context &context) {
  context.axes.getX().release();
  context.axes.getY().release();
}

MoveXYE::MoveXYE(const XYZEPosition &startPosition)
    : Action(Type::MOVE_XYE, startPosition),
      segmentStart_(startPosition.asXyePosition()),
      numPointsPushed_(0),
      numPointsCompleted_(0),
      hasNewEndPosition_(false) {}

bool MoveXYE::pushPoint(Context &context,
                        const Axes::XAxis::GcodePosition *const endPositionX,
                        const Axes::YAxis::GcodePosition *const endPositionY,
                        const Axes::EAxis::GcodePosition endPositionE) {
  // Update end position but do not commit to state until a point is
  // successfully pushed to the queue.
  XYZEPosition tempEndPosition = getEndPosition();
  if (endPositionX) {
    tempEndPosition.x = *endPositionX;
  }
  if (endPositionY) {
    tempEndPosition.y = *endPositionY;
  }
  tempEndPosition.e = endPositionE;
  if (tempEndPosition != getEndPosition()) {
    // Require that the point be distinct to prevent division by zero
    bool output =
        context.xyePositionQueue.push(tempEndPosition.asXyePosition());
    if (output) {
      endPosition_ = tempEndPosition;
      context.actionQueue.updateXyeSegment(*this);
      numPointsPushed_++;
      hasNewEndPosition_ = true;
    }
    return output;
  }
  return true;
}

uint32_t MoveXYE::getNumPointsPushed() const { return numPointsPushed_; }

bool MoveXYE::checkNewPointDirection(
    const Axes::EAxis::GcodePosition &newE) const {
  if (getNumPointsPushed() == 0) {
    return true;
  }
  return ((segmentStart_.e < getEndPosition().e) &&
          (getEndPosition().e < newE)) ||
         ((segmentStart_.e > getEndPosition().e) &&
          (getEndPosition().e > newE));
}

void MoveXYE::onStart(Context &context) {
  // It should be guaranteed that the queue contains at least one point
  XYEPosition segmentEndPosition = *context.xyePositionQueue.first();
  XYEPosition startPosition = segmentStart_;

  context.axes.setXyParams(startPosition, segmentEndPosition, 1.0f);
  context.axes.getE().beginExtrusion();
  context.axes.getE().setExtrusionEndpoint(getEndPosition().e);
}

void MoveXYE::onLoop(Context &context) {
  if (hasNewEndPosition_) {
    context.axes.getE().setExtrusionEndpoint(getEndPosition().e);
    hasNewEndPosition_ = false;
  }
  if (context.axes.getX().isAtTargetPosition() &&
      context.axes.getY().isAtTargetPosition()) {
    if (context.xyePositionQueue.first()) {
      segmentStart_ = *context.xyePositionQueue.first();
      context.xyePositionQueue.pop();
      numPointsCompleted_++;
      if (numPointsCompleted_ < numPointsPushed_) {
        XYEPosition endPosition = *context.xyePositionQueue.first();
        context.axes.setXyParams(segmentStart_, endPosition,
                                 context.axes.getFeedrate());
      }
    }
  }
  if (context.xyePositionQueue.first()) {
    context.axes.getE().throttle(
        segmentStart_, *context.xyePositionQueue.first(),
        context.axes.getCurrentPosition().asXyePosition());
  }
}

bool MoveXYE::isFinished(const Context &context) const {
  return numPointsCompleted_ == numPointsPushed_;
}

void MoveXYE::onPush(Context &context) {
  context.axes.getX().acquire();
  context.axes.getY().acquire();
  context.axes.getE().acquire();
}

void MoveXYE::onPop(Context &context) {
  context.axes.getX().release();
  context.axes.getY().release();
  context.axes.getE().release();
}

MoveE::MoveE(const XYZEPosition &startPosition,
             const Axes::EAxis::GcodePosition endPositionE)
    : Action(Type::MOVE_E, startPosition) {
  endPosition_.e = endPositionE;
}

void MoveE::onStart(Context &context) {
  // Use constant feedrate
  context.axes.getE().setFeedrate(
      Axes::EAxis::GcodeFeedrate(*context.axes.getFeedrate() / 10));
  context.axes.getE().setTargetPosition(getEndPosition().e);
}

bool MoveE::isFinished(const Context &context) const {
  return context.axes.getE().isAtTargetPosition();
}

void MoveE::onPush(Context &context) { context.axes.getE().acquire(); }

void MoveE::onPop(Context &context) { context.axes.getE().release(); }

MoveZ::MoveZ(const XYZEPosition &startPosition,
             const Axes::ZAxis::GcodePosition endPositionZ)
    : Action(Type::MOVE_Z, startPosition) {
  endPosition_.z = endPositionZ;
}

void MoveZ::onStart(Context &context) {
  // Use constant feedrate
  context.axes.getZ().setFeedrate(Axes::ZAxis::GcodeFeedrate(600.0f));
  context.axes.getZ().setTargetPosition(getEndPosition().z);
}

bool MoveZ::isFinished(const Context &context) const {
  return context.axes.getZ().isAtTargetPosition();
}

void MoveZ::onPush(Context &context) { context.axes.getZ().acquire(); }

void MoveZ::onPop(Context &context) { context.axes.getZ().release(); }

SetFeedrate::SetFeedrate(const XYZEPosition &startPosition,
                         const float rawFeedrateMmPerMin)
    : Action(Type::SET_FEEDRATE, startPosition),
      rawFeedrateMmPerMin_(rawFeedrateMmPerMin) {}

void SetFeedrate::onStart(Context &context) {
  context.axes.setFeedrate(rawFeedrateMmPerMin_);
}

bool SetFeedrate::isFinished(const Context &context) const { return true; }
}  // namespace Action

ActionQueue::ActionQueue()
    : PooledQueue(), startPosition_({0, 0, 0, 0}), endPosition_({0, 0, 0, 0}) {}

XYZEPosition ActionQueue::getStartPosition() const { return startPosition_; }

XYZEPosition ActionQueue::getEndPosition() const { return endPosition_; }

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
