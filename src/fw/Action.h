// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <fw/Axes.h>
#include <fw/Heater.h>
#include <if/Clock.h>
#include <if/Serial.h>
#include <util/PooledQueue.h>
#include <util/Units.h>

namespace Clef::Fw {
class XYEPositionQueue : public Clef::Util::PooledQueue<XYEPosition, 128> {};

class ActionQueue;
class GcodeParser;
class Context;

namespace Action {
enum class Type {
  MOVE_XY,
  MOVE_XYE,
  MOVE_E,
  MOVE_Z,
  SET_FEEDRATE,
  SET_TEMP,
  WAIT_FOR
};

/**
 * Data structure to store parameters for commands the printer to execute; every
 * action implements this interface. The type of the action is stored in memory
 * so that derivatives of this class can be in a union.
 */
class Action {
  friend class Clef::Fw::ActionQueue;

 public:
  Action(const Type type, const XYZEPosition &startPosition);
  Type getType() const { return type_; }
  XYZEPosition getEndPosition() const;

  /**
   * Executed when the action reaches the front of the queue and becomes active.
   */
  virtual void onStart(Context &context) = 0;

  /**
   * Executed from the main event loop while the action is active.
   */
  virtual void onLoop(Context &context) = 0;

  /**
   * Check whether this action is completed. Called from the main event loop.
   * Should have no side-effects.
   */
  virtual bool isFinished(const Context &context) const = 0;

 protected:
  /**
   * Executed when the action is pushed to the queue.
   */
  virtual void onPush(Context &context) = 0;

  /**
   * Executed when the action is removed from the queue.
   */
  virtual void onPop(Context &context) = 0;

 protected:
  Type type_;
  XYZEPosition endPosition_;
};

class MoveXY : public Action {
 public:
  MoveXY() : MoveXY({0, 0, 0, 0}, nullptr, nullptr) {}
  MoveXY(const XYZEPosition &startPosition,
         const Axes::XAxis::GcodePosition *const endPositionX,
         const Axes::YAxis::GcodePosition *const endPositionY);

  void onStart(Context &context) override;
  void onLoop(Context &context) override {}
  bool isFinished(const Context &context) const override;

 private:
  void onPush(Context &context) override;
  void onPop(Context &context) override;
};

class MoveXYE : public Action {
 public:
  MoveXYE() : MoveXYE({0, 0, 0, 0}) {}
  MoveXYE(const XYZEPosition &startPosition);

  /**
   * Add a point in the extrusion path; returns false if there was not room in
   * the queue to add another point.
   */
  bool pushPoint(Context &context,
                 const Axes::XAxis::GcodePosition *const endPositionX,
                 const Axes::YAxis::GcodePosition *const endPositionY,
                 const Axes::EAxis::GcodePosition endPositionE);

  /**
   * Get the number of XYE points added to this segment.
   */
  uint32_t getNumPointsPushed() const;

  /**
   * Check whether a prospective additional XYE point is going in the same
   * direction as the others.
   */
  bool checkNewPointDirection(const Axes::EAxis::GcodePosition &newE) const;

  void onStart(Context &context) override;
  void onLoop(Context &context) override;
  bool isFinished(const Context &context) const override;

 private:
  void onPush(Context &context) override;
  void onPop(Context &context) override;

 private:
  XYEPosition segmentStart_;
  uint32_t numPointsPushed_;
  uint32_t numPointsCompleted_;
  bool hasNewEndPosition_;
};

class MoveE : public Action {
 public:
  MoveE() : MoveE({0, 0, 0, 0}, 0) {}
  MoveE(const XYZEPosition &startPosition,
        const Axes::EAxis::GcodePosition endPositionE);

  void onStart(Context &context) override;
  void onLoop(Context &context) override {}
  bool isFinished(const Context &context) const override;

 private:
  void onPush(Context &context) override;
  void onPop(Context &context) override;
};

class MoveZ : public Action {
 public:
  MoveZ() : MoveZ({0, 0, 0, 0}, 0) {}
  MoveZ(const XYZEPosition &startPosition,
        const Axes::ZAxis::GcodePosition endPositionZ);

  void onStart(Context &context) override;
  void onLoop(Context &context) override {}
  bool isFinished(const Context &context) const override;

 private:
  void onPush(Context &context) override;
  void onPop(Context &context) override;
};

class SetFeedrate : public Action {
 public:
  SetFeedrate() : SetFeedrate({0, 0, 0, 0}, 1200) {}
  SetFeedrate(const XYZEPosition &startPosition,
              const float rawFeedrateMmPerMin);

  void onStart(Context &context) override;
  void onLoop(Context &context) override {}
  bool isFinished(const Context &context) const override;

 private:
  void onPush(Context &context) override {}
  void onPop(Context &context) override {}

 private:
  float rawFeedrateMmPerMin_;
};

class SetTemp : public Action {
 public:
  SetTemp() : SetTemp({0, 0, 0, 0}, nullptr, 0.0f) {}
  SetTemp(const XYZEPosition &startPosition, Clef::Fw::Heater *heater,
          const float target);

  void onStart(Context &context) override;
  void onLoop(Context &context) override {}
  bool isFinished(const Context &context) const override;

 private:
  void onPush(Context &context) override {}
  void onPop(Context &context) override {}

 private:
  Clef::Fw::Heater *heater_;
  float target_;
};

class WaitFor : public Action {
 public:
  using Predicate = bool (*)(const void *);

  WaitFor() : WaitFor({0, 0, 0, 0}, nullptr, nullptr) {}
  WaitFor(const XYZEPosition &startPosition, const Predicate predicate,
          const void *arg);

  void onStart(Context &context) override {}
  void onLoop(Context &context) override {}
  bool isFinished(const Context &context) const override;

  static bool temperaturesHaveReachedTargets(const void *arg);

 private:
  void onPush(Context &context) override {}
  void onPop(Context &context) override {}

 private:
  Predicate predicate_;
  const void *arg_;
};
}  // namespace Action

struct Context {
  Clef::Fw::Axes &axes;
  Clef::Fw::GcodeParser &gcodeParser;
  Clef::If::Clock &clock;
  Clef::If::RWSerial &serial;
  Clef::Fw::ActionQueue &actionQueue;
  Clef::Fw::XYEPositionQueue &xyePositionQueue;
};

class ActionQueue : public Clef::Util::PooledQueue<Action::Action *, 32> {
 public:
  ActionQueue();
  bool push(Context &context, const Action::Action &action) {
    bool success = false;
    switch (action.getType()) {
      case Action::Type::MOVE_XY:
        success =
            moveXyQueue_.push(static_cast<const Action::MoveXY &>(action)) &&
            push(&*moveXyQueue_.last());
        break;
      case Action::Type::MOVE_XYE:
        success =
            moveXyeQueue_.push(static_cast<const Action::MoveXYE &>(action)) &&
            push(&*moveXyeQueue_.last());
        break;
      case Action::Type::MOVE_E:
        success =
            moveEQueue_.push(static_cast<const Action::MoveE &>(action)) &&
            push(&*moveEQueue_.last());
        break;
      case Action::Type::MOVE_Z:
        success =
            moveZQueue_.push(static_cast<const Action::MoveZ &>(action)) &&
            push(&*moveZQueue_.last());
        break;
      case Action::Type::SET_FEEDRATE:
        success = setFeedrateQueue_.push(
                      static_cast<const Action::SetFeedrate &>(action)) &&
                  push(&*setFeedrateQueue_.last());
        break;
      case Action::Type::SET_TEMP:
        success =
            setTempQueue_.push(static_cast<const Action::SetTemp &>(action)) &&
            push(&*setTempQueue_.last());
        break;
      case Action::Type::WAIT_FOR:
        success =
            waitForQueue_.push(static_cast<const Action::WaitFor &>(action)) &&
            push(&*waitForQueue_.last());
        break;
    }
    if (success) {
      Iterator it = last();
      endPosition_ = (*it)->getEndPosition();
      (*it)->onPush(context);
    }
    return success;
  }
  void pop(Context &context) {
    Iterator it = first();
    (*it)->onPop(context);
    startPosition_ = (*it)->getEndPosition();
    switch ((*it)->getType()) {
      case Action::Type::MOVE_XY:
        moveXyQueue_.pop();
        break;
      case Action::Type::MOVE_XYE:
        moveXyeQueue_.pop();
        break;
      case Action::Type::MOVE_E:
        moveEQueue_.pop();
        break;
      case Action::Type::MOVE_Z:
        moveZQueue_.pop();
        break;
      case Action::Type::SET_FEEDRATE:
        setFeedrateQueue_.pop();
        break;
      case Action::Type::SET_TEMP:
        setTempQueue_.pop();
        break;
      case Action::Type::WAIT_FOR:
        waitForQueue_.pop();
        break;
    }
    pop();
  }
  XYZEPosition getStartPosition() const;
  XYZEPosition getEndPosition() const;
  uint16_t getCapacityFor(const Action::Type type) const {
    switch (type) {
      case Action::Type::MOVE_XY:
        return moveXyQueue_.getNumSpacesLeft();
      case Action::Type::MOVE_XYE:
        return moveXyeQueue_.getNumSpacesLeft();
      case Action::Type::MOVE_E:
        return moveEQueue_.getNumSpacesLeft();
      case Action::Type::MOVE_Z:
        return moveZQueue_.getNumSpacesLeft();
      case Action::Type::SET_FEEDRATE:
        return setFeedrateQueue_.getNumSpacesLeft();
      case Action::Type::SET_TEMP:
        return setTempQueue_.getNumSpacesLeft();
      case Action::Type::WAIT_FOR:
        return waitForQueue_.getNumSpacesLeft();
    }
    return 0;
  }
  bool hasCapacityFor(const Action::Type type) const {
    return getCapacityFor(type) > 0;
  }

  /**
   * If a point is added to an XYE segment, this queue needs to know about it so
   * it can update endPosition_.
   */
  void updateXyeSegment(const Action::MoveXYE &moveXye);

  /**
   * Debugging method for making sure that the total size of this queue is equal
   * to the sum of the sizes of the subqueues.
   */
  bool checkConservation() const;

 private:
  bool push(Action::Action *action);
  void pop();

 private:
  XYZEPosition startPosition_; /*!< Remember start position of the current
                                        first action. */
  XYZEPosition endPosition_;   /*!< Remember end position of the last action. */

  Clef::Util::PooledQueue<Action::MoveXY, 8> moveXyQueue_;
  Clef::Util::PooledQueue<Action::MoveXYE, 8> moveXyeQueue_;
  Clef::Util::PooledQueue<Action::MoveE, 4> moveEQueue_;
  Clef::Util::PooledQueue<Action::MoveZ, 4> moveZQueue_;
  Clef::Util::PooledQueue<Action::SetFeedrate, 4> setFeedrateQueue_;
  Clef::Util::PooledQueue<Action::SetTemp, 4> setTempQueue_;
  Clef::Util::PooledQueue<Action::WaitFor, 4> waitForQueue_;
};
}  // namespace Clef::Fw
