#pragma once
#ifndef EXMATH_CONSTANTS_HPP_INCLUDED
#define EXMATH_CONSTANTS_HPP_INCLUDED

#include <cmath>
#include <cstdint>
#include <limits>
#include <concepts>

namespace exmath::constants
{
  template <typename T>
    requires(std::is_floating_point_v<T>)
  constexpr T pi = static_cast<T>(3.1415926535897932384626433832795028841971693993751058209749445923078164);
}    // namespace exmath::polynominal

#endif
