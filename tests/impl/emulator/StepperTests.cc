// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <gtest/gtest.h>
#include <impl/emulator/Stepper.h>

namespace Clef::Impl::Emulator {
class StepperTest : public testing::Test {
 public:
  StepperTest() { stepper_.init(); }

 protected:
  XAxisStepper stepper_;
};

TEST_F(StepperTest, Pulse) {
  stepper_.pulse();
  ASSERT_TRUE(XAxisStepperConfig::PulseRegister::getCurrentState());
  stepper_.unpulse();
  ASSERT_FALSE(XAxisStepperConfig::PulseRegister::getCurrentState());
}

TEST_F(StepperTest, Resolution) {
  stepper_.setIncreasing();
  stepper_.setResolution(XAxisStepper::Resolution::_32);
  ASSERT_TRUE(XAxisStepperConfig::ResolutionRegister2::getCurrentState());
  ASSERT_FALSE(XAxisStepperConfig::ResolutionRegister1::getCurrentState());
  ASSERT_TRUE(XAxisStepperConfig::ResolutionRegister0::getCurrentState());
  stepper_.pulse();
  stepper_.unpulse();
  ASSERT_EQ(stepper_.getPosition(), 1);
  stepper_.setResolution(XAxisStepper::Resolution::_16);
  ASSERT_TRUE(XAxisStepperConfig::ResolutionRegister2::getCurrentState());
  ASSERT_FALSE(XAxisStepperConfig::ResolutionRegister1::getCurrentState());
  ASSERT_FALSE(XAxisStepperConfig::ResolutionRegister0::getCurrentState());
  stepper_.pulse();
  stepper_.unpulse();
  ASSERT_EQ(stepper_.getPosition(), 3);
  stepper_.setResolution(XAxisStepper::Resolution::_8);
  ASSERT_FALSE(XAxisStepperConfig::ResolutionRegister2::getCurrentState());
  ASSERT_TRUE(XAxisStepperConfig::ResolutionRegister1::getCurrentState());
  ASSERT_TRUE(XAxisStepperConfig::ResolutionRegister0::getCurrentState());
  stepper_.pulse();
  stepper_.unpulse();
  ASSERT_EQ(stepper_.getPosition(), 7);
  stepper_.setResolution(XAxisStepper::Resolution::_4);
  ASSERT_FALSE(XAxisStepperConfig::ResolutionRegister2::getCurrentState());
  ASSERT_TRUE(XAxisStepperConfig::ResolutionRegister1::getCurrentState());
  ASSERT_FALSE(XAxisStepperConfig::ResolutionRegister0::getCurrentState());
  stepper_.pulse();
  stepper_.unpulse();
  ASSERT_EQ(stepper_.getPosition(), 15);
  stepper_.setResolution(XAxisStepper::Resolution::_2);
  ASSERT_FALSE(XAxisStepperConfig::ResolutionRegister2::getCurrentState());
  ASSERT_FALSE(XAxisStepperConfig::ResolutionRegister1::getCurrentState());
  ASSERT_TRUE(XAxisStepperConfig::ResolutionRegister0::getCurrentState());
  stepper_.pulse();
  stepper_.unpulse();
  ASSERT_EQ(stepper_.getPosition(), 31);
  stepper_.setResolution(XAxisStepper::Resolution::_1);
  ASSERT_FALSE(XAxisStepperConfig::ResolutionRegister2::getCurrentState());
  ASSERT_FALSE(XAxisStepperConfig::ResolutionRegister1::getCurrentState());
  ASSERT_FALSE(XAxisStepperConfig::ResolutionRegister0::getCurrentState());
  stepper_.pulse();
  stepper_.unpulse();
  ASSERT_EQ(stepper_.getPosition(), 63);
}

TEST_F(StepperTest, Direction) {
  stepper_.setResolution(XAxisStepper::Resolution::_4);
  stepper_.setTargetPosition(16);
  ASSERT_TRUE(XAxisStepperConfig::DirectionRegister::getCurrentState());
  stepper_.pulse();
  stepper_.unpulse();
  ASSERT_EQ(stepper_.getPosition(), 8);
  ASSERT_FALSE(stepper_.isAtTargetPosition());
  stepper_.pulse();
  stepper_.unpulse();
  ASSERT_EQ(stepper_.getPosition(), 16);
  ASSERT_TRUE(stepper_.isAtTargetPosition());
  stepper_.setTargetPosition(0);
  ASSERT_FALSE(XAxisStepperConfig::DirectionRegister::getCurrentState());
  stepper_.pulse();
  stepper_.unpulse();
  ASSERT_EQ(stepper_.getPosition(), 8);
  ASSERT_FALSE(stepper_.isAtTargetPosition());
  stepper_.pulse();
  stepper_.unpulse();
  ASSERT_EQ(stepper_.getPosition(), 0);
  ASSERT_TRUE(stepper_.isAtTargetPosition());
}

TEST_F(StepperTest, Acquisition) {
  ASSERT_FALSE(XAxisStepperConfig::EnableRegister::getCurrentState());
  stepper_.acquire();
  ASSERT_TRUE(XAxisStepperConfig::EnableRegister::getCurrentState());
  stepper_.release();
  ASSERT_FALSE(XAxisStepperConfig::EnableRegister::getCurrentState());
  stepper_.acquire();
  stepper_.acquire();
  ASSERT_TRUE(XAxisStepperConfig::EnableRegister::getCurrentState());
  stepper_.release();
  ASSERT_TRUE(XAxisStepperConfig::EnableRegister::getCurrentState());
  stepper_.releaseAll();
  ASSERT_FALSE(XAxisStepperConfig::EnableRegister::getCurrentState());
  stepper_.acquire();
  ASSERT_TRUE(XAxisStepperConfig::EnableRegister::getCurrentState());
  stepper_.release();
  ASSERT_FALSE(XAxisStepperConfig::EnableRegister::getCurrentState());
}
}  // namespace Clef::Impl::Emulator
