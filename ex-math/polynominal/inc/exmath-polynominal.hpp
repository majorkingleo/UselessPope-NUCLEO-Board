#pragma once
#ifndef EXMATH_POLYNOMINAL_HPP_INCLUDED
#define EXMATH_POLYNOMINAL_HPP_INCLUDED

#include <cmath>
#include <cstdint>
#include <limits>
#include <span>

namespace exmath::polynominal
{
  template <typename T, std::size_t Order> class polynominal_t
  {
    using value_type = T;

  public:
    constexpr polynominal_t() = default;

    template <typename... U>
    constexpr polynominal_t(U&&... args)
        : m_para{ args... }
    {
    }

    value_type operator()(value_type const& value) const& noexcept
    {
      value_type tmp = this->m_para[0];
      for (std::size_t i = 1; i < this->m_para.size(); i++)
      {
        tmp *= value;
        tmp += this->m_para[i];
      }
      return tmp;
    }

    value_type& operator[](uint32_t const& idx) & noexcept { return this->m_para[idx]; }
    value_type  operator[](uint32_t const& idx) const& noexcept { return this->m_para[idx]; }

    constexpr bool operator==(polynominal_t<T, Order> const&) const = default;

  private:
    std::array<value_type, Order + 1> m_para = {};
  };

}    // namespace exmath::polynominal

#endif
