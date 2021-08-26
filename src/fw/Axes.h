// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <fw/Config.h>
#include <if/Axis.h>

namespace Clef::Fw {
class Axes {
 public:
  class XAxis : public Clef::If::Axis<USTEPS_PER_MM_X> {};
  class YAxis : public Clef::If::Axis<USTEPS_PER_MM_Y> {};
  class ZAxis : public Clef::If::Axis<USTEPS_PER_MM_Z> {};
  class EAxis : public Clef::If::Axis<USTEPS_PER_MM_E> {};

  struct XYEPosition {
    using XPosition = XAxis::Position<int32_t, Clef::Util::PositionUnit::USTEP>;
    using YPosition = YAxis::Position<int32_t, Clef::Util::PositionUnit::USTEP>;
    using EPosition = EAxis::Position<int32_t, Clef::Util::PositionUnit::USTEP>;
    XPosition x = 0;
    YPosition y = 0;
    EPosition e = 0;

    bool operator==(const XYEPosition &other) const;
    bool operator!=(const XYEPosition &other) const;
  };

  struct XYZEPosition {
    using XPosition = XAxis::Position<int32_t, Clef::Util::PositionUnit::USTEP>;
    using YPosition = YAxis::Position<int32_t, Clef::Util::PositionUnit::USTEP>;
    using ZPosition = ZAxis::Position<int32_t, Clef::Util::PositionUnit::USTEP>;
    using EPosition = EAxis::Position<int32_t, Clef::Util::PositionUnit::USTEP>;
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
