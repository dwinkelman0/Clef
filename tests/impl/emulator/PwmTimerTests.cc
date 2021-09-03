// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <gtest/gtest.h>
#include <impl/emulator/PwmTimer.h>

namespace Clef::Impl::Emulator {
namespace {
void increment(void* arg) {
  int* counter = reinterpret_cast<int*>(arg);
  *counter = *counter + 1;
}
}  // namespace

TEST(PwmTimerTest, Basic) {
  GenericTimer timer(std::make_shared<std::mutex>());
  int counter = 0;
  timer.setFrequency(1000.0f);
  timer.setRisingEdgeCallback(increment, &counter);
  timer.setFallingEdgeCallback(increment, &counter);
  timer.enable();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  timer.disable();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_GT(counter, 150);
  ASSERT_LT(counter, 200);
}

TEST(PwmTimerTest, ChangeCallback) {
  GenericTimer timer(std::make_shared<std::mutex>());
  int counter1 = 0;
  int counter2 = 0;
  timer.setFrequency(1000.0f);
  timer.setRisingEdgeCallback(increment, &counter1);
  timer.setFallingEdgeCallback(increment, &counter1);
  timer.enable();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  timer.setRisingEdgeCallback(increment, &counter2);
  timer.setFallingEdgeCallback(increment, &counter2);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  timer.disable();
  ASSERT_GT(counter1, 0);
  ASSERT_GT(counter2, 0);
}

TEST(PwmTimerTest, Reenable) {
  GenericTimer timer(std::make_shared<std::mutex>());
  int counter = 0;
  timer.setFrequency(1000.0f);
  timer.setRisingEdgeCallback(increment, &counter);
  timer.setFallingEdgeCallback(increment, &counter);
  timer.enable();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  timer.disable();
  int checkpoint = counter;
  timer.enable();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  timer.disable();
  ASSERT_GT(counter, checkpoint);
}
}  // namespace Clef::Impl::Emulator
