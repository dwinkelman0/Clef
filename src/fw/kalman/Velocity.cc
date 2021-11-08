// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Velocity.h"

namespace Clef::Fw::Kalman {
namespace {
ARRAY(float, memQ, {/* x */ 1, /* v */ 1});
ARRAY(float, memR, {1});
ARRAY(float, memWx, {1, 0.5});

typename BaseVelocityFilter::QMatrix Q(memQ);
typename BaseVelocityFilter::RMatrix R(memR);
typename BaseVelocityFilter::WxMatrix Wx(memWx);
}  // namespace

VelocityFilter::VelocityFilter() : BaseVelocityFilter(Q, R, Wx) { init(); }

void VelocityFilter::evolve(const float xe, const float xs,
                            const float deltat) {
  float uMem[1];
  float zMem[1];
  typename BaseVelocityFilter::UVector u(uMem);
  typename BaseVelocityFilter::ZVector z(zMem);
  u.set(0, 0, xe);
  z.set(0, 0, xs);
  BaseVelocityFilter::evolve(u, z, deltat);
}

void VelocityFilter::init() {
  x_.set(0, 0, 0);
  x_.set(1, 0, 0);
  P_.set(0, 0, 5);
  P_.set(0, 1, 0);
  P_.set(1, 0, 0);
  P_.set(1, 1, 5);
}

void VelocityFilter::calculateStateTrans(
    const typename BaseVelocityFilter::XVector &xk,
    const typename BaseVelocityFilter::UVector &uk, const float deltat,
    typename BaseVelocityFilter::XVector &output) const {
  // x(k+1) = x(k) + v(k) * deltat
  output.set(0, 0, xk.get(0, 0) + xk.get(1, 0) * deltat);

  // v(k+1) = v(k)
  output.set(1, 0, xk.get(1, 0));
}

void VelocityFilter::calculateStateTransGradient(
    const typename BaseVelocityFilter::XVector &xk,
    const typename BaseVelocityFilter::UVector &uk, const float deltat,
    typename BaseVelocityFilter::FMatrix &output) const {
  // dx/dx = 1
  output.set(0, 0, 1);

  // dx/dv = deltat
  output.set(0, 1, deltat);

  // dv/dx = 0
  output.set(1, 0, 0);

  // dv/dv = 1
  output.set(1, 1, 1);
}

void VelocityFilter::calculateObservationTrans(
    const typename BaseVelocityFilter::XVector &xk,
    typename BaseVelocityFilter::ZVector &output) const {
  // xs = x
  output.set(0, 0, xk.get(0, 0));
}

void VelocityFilter::calculateObserationTransGradient(
    const typename BaseVelocityFilter::XVector &xk,
    typename BaseVelocityFilter::HMatrix &output) const {
  // dxs/dx = 1
  output.set(0, 0, 1);

  // dxs/dv = 0
  output.set(0, 1, 0);
}
}  // namespace Clef::Fw::Kalman
