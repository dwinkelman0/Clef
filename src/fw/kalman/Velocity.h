// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <fw/KalmanFilter.h>

namespace Clef::Fw::Kalman {
using BaseVelocityFilter = Clef::Fw::ExtendedKalmanFilter<2, 1, 1>;

class VelocityFilter : public BaseVelocityFilter {
 public:
  VelocityFilter();
  void evolve(/* Control */ const float xe, /* Observation */ const float xs,
              /* Time step */ const float deltat);

 private:
  void init() override;

  void calculateStateTrans(
      const typename BaseVelocityFilter::XVector &xk,
      const typename BaseVelocityFilter::UVector &uk, const float deltat,
      typename BaseVelocityFilter::XVector &output) const override;
  void calculateStateTransGradient(
      const typename BaseVelocityFilter::XVector &xk,
      const typename BaseVelocityFilter::UVector &uk, const float deltat,
      typename BaseVelocityFilter::FMatrix &output) const override;
  void calculateObservationTrans(
      const typename BaseVelocityFilter::XVector &xk,
      typename BaseVelocityFilter::ZVector &output) const override;
  void calculateObserationTransGradient(
      const typename BaseVelocityFilter::XVector &xk,
      typename BaseVelocityFilter::HMatrix &output) const override;
};
}  // namespace Clef::Fw::Kalman
