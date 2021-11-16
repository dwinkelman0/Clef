// Copyright 2021 by Daniel Winkelman. All rights reserved.
// This file is autogenerated by kalman.py.

#include <fw/KalmanFilter.h>

namespace Clef::Fw::Kalman {
using BaseDegenFilter = Clef::Fw::ExtendedKalmanFilter<9, 1, 2>;
class DegenFilter : public BaseDegenFilter {
 public:
  DegenFilter();
  void evolve(
      /* Control Variables */ const float xe,
      /* Observation Variables */ const float xs_in, const float Ph_in,
      /* Time Step */ const float deltat);
  void init() override;

 private:
  void calculateStateTrans(
      const typename BaseDegenFilter::XVector &xk,
      const typename BaseDegenFilter::UVector &uk, const float deltat,
      typename BaseDegenFilter::XVector &output) const override;
  void calculateStateTransGradient(
      const typename BaseDegenFilter::XVector &xk,
      const typename BaseDegenFilter::UVector &uk, const float deltat,
      typename BaseDegenFilter::FMatrix &output) const override;
  void calculateObservationTrans(
      const typename BaseDegenFilter::XVector &xk,
      typename BaseDegenFilter::ZVector &output) const override;
  void calculateObserationTransGradient(
      const typename BaseDegenFilter::XVector &xk,
      typename BaseDegenFilter::HMatrix &output) const override;
};
}  // namespace Clef::Fw::Kalman