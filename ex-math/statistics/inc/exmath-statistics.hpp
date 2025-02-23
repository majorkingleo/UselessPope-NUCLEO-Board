#pragma once
#ifndef EXMATH_STATISTICS_HPP_INCLUDED
#define EXMATH_STATISTICS_HPP_INCLUDED

#include <cmath>
#include <cstdint>
#include <limits>
#include <span>

namespace exmath::statistics
{

  class BatchStatistics
  {
  public:
    using count_type = std::size_t;
    using value_type = double;

    BatchStatistics() = default;

    BatchStatistics(value_type const& value) { this->operator()(value); }

    BatchStatistics(std::span<value_type const> const& values)
    {
      for (value_type const value : values)
      {
        if (std::isnan(value))
          continue;

        if (this->m_max < value)
          this->m_max = value;
        if (this->m_min > value)
          this->m_min = value;

        this->m_mean += value;
        this->m_x_sqr += value * value;
      }

      this->m_count          = values.size();
      value_type const scale = 1.0 / static_cast<value_type>(this->m_count);
      this->m_mean *= scale;
      this->m_x_sqr *= scale;
    }

    template <typename functor> BatchStatistics(functor const& ftor, count_type max_idx)
    {
      for (count_type i = 0; i < max_idx; i++)
      {
        value_type const value = ftor(i);
        if (std::isnan(value))
          continue;

        if (this->m_max < value)
          this->m_max = value;
        if (this->m_min > value)
          this->m_min = value;

        this->m_mean += value;
        this->m_x_sqr += value * value;
      }

      this->m_count          = max_idx;
      value_type const scale = 1.0 / static_cast<value_type>(this->m_count);
      this->m_mean *= scale;
      this->m_x_sqr *= scale;
    }

    auto operator()(value_type const& value) noexcept -> BatchStatistics&
    {
      if (std::isnan(value))
        return *this;

      if (this->m_max < value)
        this->m_max = value;
      if (this->m_min > value)
        this->m_min = value;

      if (this->m_count < std::numeric_limits<count_type>::max())
        this->m_count++;

      value_type const scale = 1.0 / static_cast<value_type>(this->m_count);
      this->m_mean += (value - this->m_mean) * scale;
      this->m_x_sqr += (value * value - this->m_x_sqr) * scale;

      return *this;
    }

    template <typename functor> auto operator()(functor const& ftor, count_type max_idx) noexcept -> BatchStatistics&
    {
      return this->operator()(BatchStatistics{ ftor, max_idx });
    }
    auto operator()(std::span<value_type const> const& values) noexcept -> BatchStatistics& { return this->operator()(BatchStatistics{ values }); }

    auto operator()(BatchStatistics const& value) noexcept -> BatchStatistics&
    {
      if (this->m_max < value.m_max)
        this->m_max = value.m_max;
      if (this->m_min > value.m_min)
        this->m_min = value.m_min;

      if ((std::numeric_limits<count_type>::max() - this->m_count) > value.m_count)
        this->m_count += value.m_count;
      else
        this->m_count = std::numeric_limits<count_type>::max();

      value_type const scale = static_cast<value_type>(value.m_count) / static_cast<value_type>(this->m_count);
      this->m_mean += (value.m_mean - this->m_mean) * scale;
      this->m_x_sqr += (value.m_x_sqr - this->m_x_sqr) * scale;
      return *this;
    }

    count_type get_number_of_values() const noexcept { return this->m_count; }
    value_type get_max() const noexcept { return this->m_max; }
    value_type get_min() const noexcept { return this->m_min; }
    value_type get_mean() const noexcept
    {
      if (this->m_count == 0)
        return std::numeric_limits<value_type>::quiet_NaN();
      return this->m_mean;
    }
    value_type get_variance() const noexcept
    {
      if (this->m_count < 2)
        return std::numeric_limits<value_type>::quiet_NaN();
      value_type const scale = static_cast<value_type>(this->m_count) / static_cast<value_type>(this->m_count - 1);
      return std::abs(this->m_x_sqr - this->m_mean * this->m_mean) * scale;
    }
    value_type get_standard_deviation() const noexcept { return std::sqrt(this->get_variance()); }

  private:
    count_type m_count = 0;
    value_type m_max   = -std::numeric_limits<value_type>::infinity();
    value_type m_min   = std::numeric_limits<value_type>::infinity();
    value_type m_mean  = 0.0f;
    value_type m_x_sqr = 0.0f;
  };

  inline BatchStatistics operator+(BatchStatistics const& lhs, BatchStatistics const& rhs) noexcept
  {
    BatchStatistics ret{ lhs };
    return ret(rhs);
  }
  inline BatchStatistics& operator+=(BatchStatistics& lhs, BatchStatistics const& rhs) noexcept { return lhs(rhs); }
}    // namespace exmath::statistics

#endif
