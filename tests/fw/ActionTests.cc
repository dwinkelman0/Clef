// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <fw/Action.h>

#include <iostream>

#include "IntegrationFixture.h"

namespace Clef::Fw {
class ActionTest : public IntegrationFixture {};

TEST_F(ActionTest, MoveXYE) {
  Axes::XAxis::GcodePosition xPos(10);
  Axes::EAxis::GcodePosition ePos(10);
  Action::MoveXYE action({0, 0, 0, 0});
  action.pushPoint(context_, &xPos, nullptr, ePos);
  ASSERT_EQ(context_.xyePositionQueue.size(), 1);
  ASSERT_EQ(action.getEndPosition().x, xPos);
  ASSERT_EQ(action.getEndPosition().e, ePos);
  action.onStart(context_);
  ASSERT_EQ(*axes_.getX().getTargetStepperPosition(), 10 * USTEPS_PER_MM_X);
  ASSERT_EQ(*axes_.getE().getExtrusionEndpoint(), 10);
  while (!action.isFinished(context_)) {
    xAxisTimer_.pulseOnce();
    eAxisTimer_.pulseOnce();
    action.onLoop(context_);
  }
  ASSERT_EQ(*axes_.getX().getPosition(), 10 * USTEPS_PER_MM_X);
  ASSERT_EQ(context_.xyePositionQueue.size(), 0);
}
}  // namespace Clef::Fw
