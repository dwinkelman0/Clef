// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Axes.h"

namespace Clef::Fw {
bool Axes::XYEPosition::operator==(const XYEPosition &other) const {
  return x == other.x && y == other.y && e == other.e;
}

bool Axes::XYEPosition::operator!=(const XYEPosition &other) const {
  return !(*this == other);
}

bool Axes::XYZEPosition::operator==(const XYZEPosition &other) const {
  return x == other.x && y == other.y && z == other.z && e == other.e;
}

bool Axes::XYZEPosition::operator!=(const XYZEPosition &other) const {
  return !(*this == other);
}

Axes::XYEPosition Axes::XYZEPosition::asXyePosition() const {
  return {x, y, e};
}
}  // namespace Clef::Fw
