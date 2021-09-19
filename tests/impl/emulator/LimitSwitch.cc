// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <gtest/gtest.h>
#include <impl/emulator/LimitSwitch.h>

namespace Clef::Impl::Emulator {
class LimitSwitchTest : public Clef::Impl::Emulator::LimitSwitch,
                        public testing::Test {
 public:
  LimitSwitchTest() : callbackWasFired_(false) {
    setTriggerCallback(callback, this);
  }

  static void callback(void *arg) {
    LimitSwitchTest *test = reinterpret_cast<LimitSwitchTest *>(arg);
    test->callbackWasFired_ = true;
  }

  void checkCallbackFired() {
    ASSERT_TRUE(callbackWasFired_);
    callbackWasFired_ = false;
  }

 protected:
  bool callbackWasFired_;
};

TEST_F(LimitSwitchTest, Basic) {
  ASSERT_FALSE(isTriggered());
  setInputState(true);
  ASSERT_TRUE(isTriggered());
  checkCallbackFired();
  setInputState(false);
  ASSERT_TRUE(isTriggered());
  ASSERT_FALSE(callbackWasFired_);
  setInputState(true);
  ASSERT_TRUE(isTriggered());
  ASSERT_FALSE(callbackWasFired_);
  setInputState(false);
  reset();
  ASSERT_FALSE(isTriggered());
  ASSERT_FALSE(callbackWasFired_);
}

TEST_F(LimitSwitchTest, ResetWhileTriggered) {
  setInputState(true);
  checkCallbackFired();
  reset();
  ASSERT_FALSE(callbackWasFired_);
  ASSERT_TRUE(isTriggered());
}
}  // namespace Clef::Impl::Emulator
