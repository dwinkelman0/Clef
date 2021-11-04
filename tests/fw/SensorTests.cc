// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <fw/Sensor.h>
#include <gtest/gtest.h>
#include <impl/emulator/Clock.h>

#include <thread>

namespace Clef::Fw {
Clef::Impl::Emulator::Clock clock;

class SensorTest : public testing::Test, public Sensor<float> {
 public:
  SensorTest() : testing::Test(), Sensor<float>(clock), receivedUpdate_(false) {
    clock.init();
  }

  void onCurrentUpdate(const DataPoint dataPoint) override {
    receivedUpdate_ = true;
  }

  void checkReceivedUpdate() {
    ASSERT_TRUE(receivedUpdate_);
    receivedUpdate_ = false;
  }

 protected:
  bool receivedUpdate_;
};

TEST_F(SensorTest, SingleSubscriber) {
  uint8_t token = subscribe();

  // Start in NULL state
  ASSERT_FALSE(checkOut(token));

  // Transition to DATA_READY state
  inject(1.0f);
  checkReceivedUpdate();

  // Transition to CHECKED_OUT state
  ASSERT_TRUE(checkOut(token));
  ASSERT_EQ(read().data, 1.0f);
  Time t0 = read().time;

  // Transition to NULL state
  release(token);
  ASSERT_FALSE(checkOut(token));

  // Transition to DATA_READY state
  inject(-10.0f);
  checkReceivedUpdate();
  inject(2.0f);
  checkReceivedUpdate();

  // Transition to CHECKED_OUT state
  ASSERT_TRUE(checkOut(token));
  ASSERT_FALSE(receivedUpdate_);
  ASSERT_EQ(read().data, 2.0f);
  Time t1 = read().time;
  ASSERT_GE(t1, t0);

  // Transition to CHECKED_OUT_AND_STAGED state
  inject(10.0f);
  ASSERT_FALSE(receivedUpdate_);
  inject(3.0f);
  ASSERT_FALSE(receivedUpdate_);
  ASSERT_EQ(read().data, 2.0f);

  // Transition to DATA_READY state
  release(token);
  checkReceivedUpdate();

  // Transition to CHECKED_OUT state
  ASSERT_TRUE(checkOut(token));
  ASSERT_EQ(read().data, 3.0f);
  Time t2 = read().time;
  ASSERT_GE(t2, t1);

  // Transition to NULL state
  release(token);
  ASSERT_FALSE(checkOut(token));
}

TEST_F(SensorTest, MultipleSubscribers) {
  uint8_t token1 = subscribe();
  uint8_t token2 = subscribe();

  // Start in NULL state
  ASSERT_FALSE(checkOut(token1));
  ASSERT_FALSE(checkOut(token2));

  // Transition to DATA_READY state
  inject(1.0f);
  checkReceivedUpdate();

  // Transition to CHECKED_OUT state
  ASSERT_TRUE(checkOut(token1));
  ASSERT_EQ(read().data, 1.0f);

  // Transition to DATA_READY state and not be able to check out same token
  release(token1);
  ASSERT_FALSE(checkOut(token1));
  
  // Transition to CHECKED_OUT state
  ASSERT_TRUE(checkOut(token2));
  ASSERT_EQ(read().data, 1.0f);

  // Transition to NULL state
  release(token2);
  ASSERT_FALSE(checkOut(token1));

  // Transition to DATA_READY state
  inject(-10.0f);
  checkReceivedUpdate();
  inject(2.0f);
  checkReceivedUpdate();

  // Transition to CHECKED_OUT state
  ASSERT_TRUE(checkOut(token1));
  ASSERT_TRUE(checkOut(token2));
  ASSERT_EQ(read().data, 2.0f);

  // Transition to NULL state
  release(token1);
  release(token2);
  ASSERT_FALSE(checkOut(token1));

  // Transition to DATA_READY state
  inject(3.0f);
  checkReceivedUpdate();

  // Transition to CHECKED_OUT state
  ASSERT_TRUE(checkOut(token1));
  ASSERT_EQ(read().data, 3.0f);

  // Transition to CHECKED_OUT_AND_STAGED state
  inject(10.0f);
  ASSERT_FALSE(receivedUpdate_);
  inject(4.0f);
  ASSERT_FALSE(receivedUpdate_);

  // Transition to DATA_READY state
  release(token1);
  checkReceivedUpdate();

  // Transition to CHECKED_OUT state
  ASSERT_TRUE(checkOut(token1));
  ASSERT_EQ(read().data, 4.0f);

  // Transition to CHECKED_OUT_AND_STAGED state
  inject(5.0f);
  ASSERT_FALSE(receivedUpdate_);

  // Stay in CHECKED_OUT_AND_STAGED state
  checkOut(token2);
  release(token1);
  ASSERT_FALSE(receivedUpdate_);
  ASSERT_EQ(read().data, 4.0f);

  // Transition to DATA_READY state
  release(token2);
  checkReceivedUpdate();

  // Transition to CHECKED_OUT state
  checkOut(token1);
  checkOut(token2);
  ASSERT_EQ(read().data, 5.0f);

  // Transition to NULL state
  release(token2);
  release(token1);
  ASSERT_FALSE(checkOut(token1));
}

class DisplacementSensorTest
    : public testing::Test,
      public DisplacementSensor<USTEPS_PER_MM_DISPLACEMENT, USTEPS_PER_MM_E> {
 public:
  DisplacementSensorTest()
      : testing::Test(),
        DisplacementSensor<USTEPS_PER_MM_DISPLACEMENT, USTEPS_PER_MM_E>(clock,
                                                                        0.1) {
    clock.init();
  }
};

TEST_F(DisplacementSensorTest, Filtering) {
  Time t0(1000);
  Time t1(2000);
  SensorAnalogPosition x0(1.0f);
  SensorAnalogPosition x1(2.0f);
  AxisFeedrate speed(
      AxisPosition(*SensorUstepsPosition(SensorAnalogPosition(x1 - x0))),
      Clef::Util::Time<float, Clef::Util::TimeUnit::MIN>(
          Clef::Util::Time<float, Clef::Util::TimeUnit::USEC>(*(t1 - t0))));
  this->onCurrentUpdate({t0, x0});
  this->onCurrentUpdate({t1, x1});
  EXPECT_LT(abs(*readFeedrate() - *speed / 10), 1);
}
}  // namespace Clef::Fw
