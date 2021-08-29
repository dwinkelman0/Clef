// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <fw/Config.h>
#include <if/PwmTimer.h>
#include <if/Stepper.h>
#include <stdint.h>
#include <util/Initialized.h>
#include <util/Units.h>

namespace Clef::Fw {
template <uint32_t USTEPS_PER_MM>
class Axis : public Clef::Util::Initialized {
 public:
  static const uint32_t UstepsPerMm = USTEPS_PER_MM;
  template <typename DType, Clef::Util::PositionUnit PositionU>
  using Position = Clef::Util::Position<DType, PositionU, USTEPS_PER_MM>;
  using GcodePosition =
      Clef::Util::Position<float, Clef::Util::PositionUnit::MM, USTEPS_PER_MM>;
  using StepperPosition = typename Clef::If::Stepper<USTEPS_PER_MM>::Position;
  using Feedrate =
      Clef::Util::Feedrate<float, Clef::Util::PositionUnit::MM,
                           Clef::Util::TimeUnit::MIN, USTEPS_PER_MM>;

  Axis(Clef::If::Stepper<USTEPS_PER_MM> &stepper, Clef::If::PwmTimer &pwmTimer);

  bool init() override;

  void acquire();
  void release();
  void releaseAll();

  void setTargetPosition(const StepperPosition pos);
  StepperPosition getCurrentPos() const;
  void setFeedrate(const Feedrate feedrate);

 private:
  Clef::If::Stepper<USTEPS_PER_MM> &stepper_;
  Clef::If::PwmTimer &pwmTimer_;
};

class Axes {
 public:
  class XAxis : public Axis<USTEPS_PER_MM_X> {};
  class YAxis : public Axis<USTEPS_PER_MM_Y> {};
  class ZAxis : public Axis<USTEPS_PER_MM_Z> {};
  class EAxis : public Axis<USTEPS_PER_MM_E> {};

  struct XYEPosition {
    using XPosition = XAxis::StepperPosition;
    using YPosition = YAxis::StepperPosition;
    using EPosition = EAxis::StepperPosition;
    XPosition x = 0;
    YPosition y = 0;
    EPosition e = 0;

    bool operator==(const XYEPosition &other) const;
    bool operator!=(const XYEPosition &other) const;
  };

  struct XYZEPosition {
    using XPosition = XAxis::StepperPosition;
    using YPosition = YAxis::StepperPosition;
    using ZPosition = ZAxis::StepperPosition;
    using EPosition = EAxis::StepperPosition;
    XPosition x = 0;
    YPosition y = 0;
    ZPosition z = 0;
    EPosition e = 0;

    XYEPosition asXyePosition() const;

    bool operator==(const XYZEPosition &other) const;
    bool operator!=(const XYZEPosition &other) const;
  };

  static const XYEPosition originXye;
  static const XYZEPosition originXyze;

  Axes(XAxis &x, YAxis &y, ZAxis &z, EAxis &e);
  XAxis &getX();
  const XAxis &getX() const;
  XAxis &getY();
  const XAxis &getY() const;
  XAxis &getZ();
  const XAxis &getZ() const;
  XAxis &getE();
  const XAxis &getE() const;

 private:
  XAxis &x_;
  YAxis &y_;
  ZAxis &z_;
  EAxis &e_;
};
}  // namespace Clef::Fw
