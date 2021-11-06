// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <fw/kalman/Velocity.h>
#include <gtest/gtest.h>
#include <math.h>

#include <iostream>

namespace Clef::Fw {
class KalmanTest : public testing::Test {
 public:
 protected:
  Kalman::VelocityFilter filter_;
};

TEST_F(KalmanTest, VelocityEvolution) {
  for (int i = 0; i < 400; ++i) {
    float xs = 100 * sin(i / 10.0f);
    filter_.evolve(xs, xs, 0.1);
    std::cout << "xs: " << xs << ", xhat: " << filter_.getState().get(0, 0)
              << ", vhat: " << filter_.getState().get(1, 0) << std::endl;
  }
}
}  // namespace Clef::Fw
