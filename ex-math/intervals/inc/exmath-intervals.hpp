#pragma once
#ifndef EXMATH_INTERVALS_HPP_INCLUDED
#define EXMATH_INTERVALS_HPP_INCLUDED

#include <cmath>
#include <cstdint>
#include <limits>
#include <span>

namespace exmath::intervals
{
  enum class Interval_Types
  {
    open,
    left_open,
    right_open,
    closed,
  };

  template <typename T> class Interval_Interface
  {
  public:
    using value_type = std::remove_cvref_t<T>;
    using Type       = Interval_Types;

    virtual ~Interval_Interface() = default;

    virtual value_type get_left_value() const noexcept                  = 0;
    virtual value_type get_right_value() const noexcept                 = 0;
    virtual Type       get_type() const noexcept                        = 0;
    virtual bool       includes(value_type const& value) const noexcept = 0;
    virtual value_type saturate(value_type const& value) const noexcept = 0;
  };

  template <typename T, Interval_Types type> class Interval: public Interval_Interface<T>
  {
  public:
    using value_type = std::remove_cvref_t<T>;
    using Type       = Interval_Types;

    constexpr Interval(value_type const& lhs, value_type const& rhs) noexcept
        : m_left(lhs)
        , m_right(rhs)
    {
    }

    constexpr value_type get_left_value() const noexcept override { return this->m_left; }
    constexpr value_type get_right_value() const noexcept override { return this->m_right; }
    constexpr Type       get_type() const noexcept override { return type; }

    constexpr bool includes(value_type const& value) const noexcept override
    {
      if constexpr (type == Type::open)
        return Interval::check_open(this->m_left, this->m_right, value);
      else if constexpr (type == Type::left_open)
        return Interval::check_open_closed(this->m_left, this->m_right, value);
      else if constexpr (type == Type::right_open)
        return Interval::check_closed_open(this->m_left, this->m_right, value);
      else
        return Interval::check_closed(this->m_left, this->m_right, value);
    }

    constexpr value_type saturate(value_type const& value) const noexcept override
    {
      if (value < this->m_left)
        return this->m_left;
      if (this->m_right < value)
        return this->m_right;
      return value;
    }

  private:
    static constexpr auto check_open(value_type const& lhs, value_type const& rhs, value_type const& value) -> bool { return lhs < value && value < rhs; }
    static constexpr auto check_closed_open(value_type const& lhs, value_type const& rhs, value_type const& value) -> bool
    {
      return lhs <= value && value < rhs;
    }
    static constexpr auto check_open_closed(value_type const& lhs, value_type const& rhs, value_type const& value) -> bool
    {
      return lhs < value && value <= rhs;
    }
    static constexpr auto check_closed(value_type const& lhs, value_type const& rhs, value_type const& value) -> bool { return lhs <= value && value <= rhs; }

    value_type m_left;
    value_type m_right;
  };

  template <typename T> using Interval_open       = Interval<T, Interval_Types::open>;
  template <typename T> using Interval_left_open  = Interval<T, Interval_Types::left_open>;
  template <typename T> using Interval_right_open = Interval<T, Interval_Types::right_open>;
  template <typename T> using Interval_closed     = Interval<T, Interval_Types::closed>;

}    // namespace exmath::intervals

#endif
