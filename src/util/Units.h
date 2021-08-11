// Copyright 2021 by Daniel Winkelman. All rights reserved.

namespace Clef::Util {
/**
 * Supply an interface for operations that work on all units. `Unique` is a
 * product of prime numbers which depends on the dimension of the represented
 * quantity. This template parameter statically checks that quantities operating
 * on each other are mutually compatible, i.e. if every possible combination of
 * units has its own number, then the unique numbers of the  must be equal for
 * the operation to make sense.
 */
template <typename DType, uint32_t Unique>
class GenericUnit {
 public:
  GenericUnit(const DType value) : value_(value) {}
  inline constexpr DType operator*() const { return value_; }
  inline constexpr GenericUnit operator+(const GenericUnit &other) const {
    return GenericUnit(**this + *other);
  }
  inline constexpr GenericUnit operator-(const GenericUnit &other) const {
    return GenericUnit(**this + *other);
  }
  inline constexpr GenericUnit operator*(const DType scalar) const {
    return GenericUnit(**this * scalar);
  }
  inline constexpr GenericUnit operator/(const DType scalar) const {
    return GenericUnit(**this / scalar);
  }
  inline constexpr DType operator/(const GenericUnit &other) const {
    return **this / *other;
  }
  inline constexpr bool operator==(const GenericUnit &other) const {
    return **this == *other;
  }
  inline constexpr bool operator!=(const GenericUnit &other) const {
    return **this != *other;
  }
  inline constexpr bool operator<(const GenericUnit &other) const {
    return **this < *other;
  }
  inline constexpr bool operator>(const GenericUnit &other) const {
    return **this > *other;
  }
  inline constexpr bool operator<=(const GenericUnit &other) const {
    return **this <= *other;
  }
  inline constexpr bool operator>=(const GenericUnit &other) const {
    return **this >= *other;
  }

 private:
  DType value_;
};

/**
 * Possible units in which to represent time.
 */
enum class TimeUnit { MIN = 2, SEC = 3, USEC = 5 };

/**
 * Unit representing time.
 */
template <typename DType, TimeUnit TimeU>
class Time;

/**
 * Possible units in which to represent frequency (not really necessary for
 * templating except reserving a prime number).
 */
enum class FrequencyUnit { HZ = 7 };

/**
 * Unit representing frequency (as Hz).
 */
template <typename DType>
class Frequency;

template <typename DType, TimeUnit TimeU>
class Time : public GenericUnit<DType, static_cast<uint32_t>(TimeU)> {
 public:
  constexpr Time(const DType value);

  inline constexpr explicit operator Time<DType, TimeUnit::MIN>() const;
  inline constexpr explicit operator Time<DType, TimeUnit::SEC>() const;
  inline constexpr explicit operator Time<DType, TimeUnit::USEC>() const;
};

template <typename DType>
class Time<DType, TimeUnit::MIN>
    : public GenericUnit<DType, static_cast<uint32_t>(TimeUnit::MIN)> {
 private:
  using MatchingGeneric =
      GenericUnit<DType, static_cast<uint32_t>(TimeUnit::MIN)>;

 public:
  constexpr Time(const DType value) : MatchingGeneric(value) {}
  constexpr Time(const MatchingGeneric gen) : MatchingGeneric(gen) {}

  inline constexpr explicit operator Time<DType, TimeUnit::SEC>() const {
    return **this * static_cast<DType>(60);
  }
  inline constexpr explicit operator Time<DType, TimeUnit::USEC>() const {
    return **this * static_cast<DType>(60000000);
  }
};

template <typename DType>
class Time<DType, TimeUnit::SEC>
    : public GenericUnit<DType, static_cast<uint32_t>(TimeUnit::SEC)> {
 private:
  using MatchingGeneric =
      GenericUnit<DType, static_cast<uint32_t>(TimeUnit::SEC)>;

 public:
  constexpr Time(const DType value) : MatchingGeneric(value) {}
  constexpr Time(const MatchingGeneric gen) : MatchingGeneric(gen) {}

  inline constexpr explicit operator Time<DType, TimeUnit::MIN>() const {
    return **this / static_cast<DType>(60);
  }
  inline constexpr explicit operator Time<DType, TimeUnit::USEC>() const {
    return **this * static_cast<DType>(1000000);
  }
  inline constexpr Frequency<DType> asFrequency() const {
    return Frequency<DType>(static_cast<DType>(1) / **this);
  }
};

template <typename DType>
class Time<DType, TimeUnit::USEC>
    : public GenericUnit<DType, static_cast<uint32_t>(TimeUnit::USEC)> {
 private:
  using MatchingGeneric =
      GenericUnit<DType, static_cast<uint32_t>(TimeUnit::USEC)>;

 public:
  constexpr Time(const DType value) : MatchingGeneric(value) {}
  constexpr Time(const MatchingGeneric gen) : MatchingGeneric(gen) {}

  inline constexpr explicit operator Time<DType, TimeUnit::MIN>() const {
    return **this / static_cast<DType>(60000000);
  }
  inline constexpr explicit operator Time<DType, TimeUnit::SEC>() const {
    return **this / static_cast<DType>(1000000);
  }
};

template <typename DType>
class Frequency
    : public GenericUnit<DType, static_cast<uint32_t>(FrequencyUnit::HZ)> {
 private:
  using MatchingGeneric =
      GenericUnit<DType, static_cast<uint32_t>(FrequencyUnit::HZ)>;

 public:
  constexpr Frequency(const DType value) : MatchingGeneric(value) {}
  constexpr Frequency(const MatchingGeneric gen) : MatchingGeneric(gen) {}

  inline constexpr Time<DType, TimeUnit::SEC> asTime() const {
    return Time<DType, TimeUnit::SEC>(static_cast<DType>(1) / **this);
  }
};

/**
 * Possible units in which to represent position.
 */
enum class PositionUnit { USTEP = 11, MM = 13 };

/**
 * Unit representing position (i.e. displacement).
 */
template <typename DType, PositionUnit PositionU, uint32_t USTEPS_PER_MM>
class Position : public GenericUnit<DType, static_cast<uint32_t>(PositionU) *
                                               USTEPS_PER_MM> {
 public:
  constexpr Position(const DType value);

  inline constexpr explicit
  operator Position<DType, PositionUnit::USTEP, USTEPS_PER_MM>() const;
  inline constexpr explicit
  operator Position<DType, PositionUnit::MM, USTEPS_PER_MM>() const;
};

template <typename DType, uint32_t USTEPS_PER_MM>
class Position<DType, PositionUnit::USTEP, USTEPS_PER_MM>
    : public GenericUnit<DType, static_cast<uint32_t>(PositionUnit::USTEP) *
                                    USTEPS_PER_MM> {
 private:
  using MatchingGeneric =
      GenericUnit<DType,
                  static_cast<uint32_t>(PositionUnit::USTEP) * USTEPS_PER_MM>;

 public:
  constexpr Position(const DType value) : MatchingGeneric(value) {}
  constexpr Position(const MatchingGeneric gen) : MatchingGeneric(gen) {}

  inline constexpr explicit
  operator Position<DType, PositionUnit::MM, USTEPS_PER_MM>() const {
    return **this / static_cast<DType>(USTEPS_PER_MM);
  }
};

template <typename DType, uint32_t USTEPS_PER_MM>
class Position<DType, PositionUnit::MM, USTEPS_PER_MM>
    : public GenericUnit<DType, static_cast<uint32_t>(PositionUnit::MM) *
                                    USTEPS_PER_MM> {
 private:
  using MatchingGeneric =
      GenericUnit<DType,
                  static_cast<uint32_t>(PositionUnit::MM) * USTEPS_PER_MM>;

 public:
  constexpr Position(const DType value) : MatchingGeneric(value) {}
  constexpr Position(const MatchingGeneric gen) : MatchingGeneric(gen) {}

  inline constexpr explicit
  operator Position<DType, PositionUnit::USTEP, USTEPS_PER_MM>() const {
    return **this * static_cast<DType>(USTEPS_PER_MM);
  }
};

/**
 * For simplicity, feedrate is represented exclusively as USTEPS per MIN (this
 * gives the largest possible values for greatest safety using integers, and
 * this is how G-Code files generally encode feedrate anyway).
 */
template <typename DType, PositionUnit PositionU, TimeUnit TimeU,
          uint32_t USTEPS_PER_MM>
class Feedrate : public GenericUnit<DType, static_cast<uint32_t>(PositionU) *
                                               static_cast<uint32_t>(TimeU) *
                                               USTEPS_PER_MM * 17> {
 private:
  using MatchingGeneric =
      GenericUnit<DType, static_cast<uint32_t>(PositionU) *
                             static_cast<uint32_t>(TimeU) * USTEPS_PER_MM * 17>;
  using PositionType = Position<DType, PositionU, USTEPS_PER_MM>;
  using TimeType = Time<DType, TimeU>;
  using FrequencyType = Frequency<DType>;

 public:
  constexpr Feedrate(const DType value) : MatchingGeneric(value) {}
  constexpr Feedrate(const PositionType dx, const TimeType dt)
      : MatchingGeneric(*dx / *dt) {}
  constexpr Feedrate(const PositionType dx, const FrequencyType freq)
      : MatchingGeneric(*dx * *freq) {}
  constexpr Feedrate(const MatchingGeneric gen) : MatchingGeneric(gen) {}
};

template <typename DType, PositionUnit PositionU, TimeUnit TimeU,
          uint32_t USTEPS_PER_MM>
inline constexpr Position<DType, PositionU, USTEPS_PER_MM> operator*(
    const Feedrate<DType, PositionU, TimeU, USTEPS_PER_MM> feedrate,
    const Time<DType, TimeU> dt) {
  return Position<DType, PositionU, USTEPS_PER_MM>(*feedrate * *dt);
}

template <typename DType, PositionUnit PositionU, TimeUnit TimeU,
          uint32_t USTEPS_PER_MM>
inline constexpr Position<DType, PositionU, USTEPS_PER_MM> operator*(
    const Time<DType, TimeU> dt,
    const Feedrate<DType, PositionU, TimeU, USTEPS_PER_MM> feedrate) {
  return Position<DType, PositionU, USTEPS_PER_MM>(*feedrate * *dt);
}

template <typename DType, PositionUnit PositionU, TimeUnit TimeU,
          uint32_t USTEPS_PER_MM>
inline constexpr Time<DType, TimeU> operator/(
    const Position<DType, PositionU, USTEPS_PER_MM> dx,
    const Feedrate<DType, PositionU, TimeU, USTEPS_PER_MM> feedrate) {
  return Time<DType, TimeU>(*dx / *feedrate);
}
}  // namespace Clef::Util
