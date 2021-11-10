// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Axes.h"

namespace Clef::Fw {
bool XYEPosition::operator==(const XYEPosition &other) const {
  return x == other.x && y == other.y && e == other.e;
}

bool XYEPosition::operator!=(const XYEPosition &other) const {
  return !(*this == other);
}

bool XYZEPosition::operator==(const XYZEPosition &other) const {
  return x == other.x && y == other.y && z == other.z && e == other.e;
}

bool XYZEPosition::operator!=(const XYZEPosition &other) const {
  return !(*this == other);
}

XYEPosition XYZEPosition::asXyePosition() const { return {x, y, e}; }

bool Axes::init() {
  x_.init();
  y_.init();
  z_.init();
  e_.init();
  return true;
}
}  // namespace Clef::Fw
