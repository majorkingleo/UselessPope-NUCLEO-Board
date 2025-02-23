#pragma once
#ifndef EXMATH_BLAS_HPP_INCLUDED
#define EXMATH_BLAS_HPP_INCLUDED

#include <cstdint>
#include <exmath-constants.hpp>

namespace exmath
{
  using index_t = uint32_t;

  template <index_t N, index_t M> class matrix_size_t;
  template <typename T, index_t N, index_t M> class matrix_t;

  template <index_t N, index_t M> class matrix_size_t
  {
  public:
    static constexpr index_t number_of_rows     = N;
    static constexpr index_t number_of_columns  = M;
    static constexpr index_t number_of_elements = N * M;

    static constexpr index_t p_calc_idx(index_t const& r, index_t const& c) noexcept { return c + r * number_of_columns; }
  };

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  template <typename T, index_t N, index_t M> class matrix_t: public matrix_size_t<N, M>
  {
  public:
    using value_type = std::remove_cv_t<T>;
    using size_t     = matrix_size_t<N, M>;

    constexpr matrix_t() noexcept = default;

    constexpr matrix_t(value_type const (&data)[size_t::number_of_elements]) noexcept
    {
      matrix_t<value_type, N, M>& self = *this;
      index_t                     k    = 0;
      for (index_t i = 0; i < size_t::number_of_rows; ++i)
        for (index_t j = 0; j < size_t::number_of_columns; ++j, ++k)
        {
          self(i, j) = data[k];
        }
    }

    template <typename... Ts>
    constexpr matrix_t(Ts const&... args)
        : m_data{ args... }
    {
    }

    constexpr matrix_t(matrix_t<const T, N, M> const& val) noexcept
    {
      matrix_t<value_type, N, M>& self = *this;
      for (index_t i = 0; i < size_t::number_of_rows; ++i)
        for (index_t j = 0; j < size_t::number_of_columns; ++j)
        {
          self(i, j) = val(i, j);
        }
    }

    [[nodiscard]] constexpr value_type operator()(index_t const& r, index_t const& c) const& noexcept { return this->m_data[size_t::p_calc_idx(r, c)]; }
    [[nodiscard]] constexpr value_type operator[](index_t const& e) const& noexcept { return this->m_data[e]; }

    [[nodiscard]] constexpr value_type& operator()(index_t const& r, index_t const& c) & noexcept { return this->m_data[size_t::p_calc_idx(r, c)]; }
    [[nodiscard]] constexpr value_type& operator[](index_t const& e) & noexcept { return this->m_data[e]; }

    constexpr void row_swap(index_t const& row_a, index_t const& row_b)
    {
      matrix_t& self = *this;
      for (index_t j = 0; j < size_t::number_of_columns; ++j)
        std::swap(self(row_a, j), self(row_b, j));
    }

    constexpr void row_div(index_t const& row, T const& val)
    {
      matrix_t& self = *this;
      for (index_t j = 0; j < size_t::number_of_columns; ++j)
        self(row, j) /= val;
    }

    constexpr void row_sub_n_times_row(index_t const& row_a, T const& fac, index_t const& row_b)
    {
      matrix_t& self = *this;
      for (index_t j = 0; j < size_t::number_of_columns; ++j)
        self(row_a, j) -= self(row_b, j) * fac;
    }

  private:
    value_type m_data[size_t::number_of_elements] = {};
  };

  template <typename T, index_t N, index_t M> class matrix_t<const T, N, M>: public matrix_size_t<N, M>
  {
  public:
    using value_type = std::remove_cv_t<T>;
    using size_t     = matrix_size_t<N, M>;

    constexpr matrix_t(value_type const (&data)[size_t::number_of_elements]) noexcept
        : m_data{ data }
    {
    }

    constexpr value_type operator()(index_t const& r, index_t const& c) const& noexcept { return this->m_data[size_t::p_calc_idx(r, c)]; }
    constexpr value_type operator[](index_t const& e) const& noexcept { return this->m_data[e]; }

  private:
    value_type const (&m_data)[size_t::number_of_elements];
  };

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  template <typename T> class matrix_t<T, 1, 1>: public matrix_size_t<1, 1>
  {
  public:
    using value_type = std::remove_cv_t<T>;
    using size_t     = matrix_size_t<1, 1>;

    constexpr matrix_t() noexcept = default;

    constexpr matrix_t(value_type const& data) noexcept
        : m_data{ data }
    {
    }

    constexpr matrix_t(value_type (&data)[size_t::number_of_elements]) noexcept
        : m_data{ data }
    {
    }

    constexpr value_type operator()(index_t const&, index_t const&) const& noexcept { return this->m_data; }
    constexpr value_type operator[](index_t const&) const& noexcept { return this->m_data; }

    constexpr value_type& operator()(index_t const&, index_t const&) & noexcept { return this->m_data; }
    constexpr value_type& operator[](index_t const&) & noexcept { return this->m_data; }

    constexpr matrix_t& operator=(value_type const& val)
    {
      matrix_t& self = *this;
      self(0, 0)     = val;
      return self;
    }

    constexpr operator value_type() const& { return this->m_data; }

  private:
    value_type m_data{};
  };

  template <typename T> class matrix_t<const T, 1, 1>: public matrix_size_t<1, 1>
  {
  public:
    using value_type = std::remove_cv_t<T>;
    using size_t     = matrix_size_t<1, 1>;

    constexpr matrix_t(value_type const (&data)[size_t::number_of_elements]) noexcept
        : m_data{ data }
    {
    }

    constexpr matrix_t(value_type const& data) noexcept
        : m_data{ data }
    {
    }

    constexpr value_type operator()(index_t const&, index_t const&) const& noexcept { return this->m_data; }
    constexpr value_type operator[](index_t const&) const& noexcept { return this->m_data; }

    constexpr operator value_type() const& { return this->m_data; }

  private:
    value_type const& m_data;
  };

  namespace Internal
  {
    template <typename TL, typename TR> [[nodiscard]] constexpr bool equal(TL const& lhs, TR const& rhs) noexcept
    {
      for (index_t i = 0; i < lhs.number_of_rows; ++i)
        for (index_t j = 0; j < lhs.number_of_columns; ++j)
        {
          if (lhs(i, j) != rhs(i, j))
            return false;
        }
      return true;
    }

    template <typename TL, typename TR> [[nodiscard]] constexpr bool not_equal(TL const& lhs, TR const& rhs) noexcept
    {
      for (index_t i = 0; i < lhs.number_of_rows; ++i)
        for (index_t j = 0; j < lhs.number_of_columns; ++j)
        {
          if (lhs(i, j) != rhs(i, j))
            return true;
        }
      return false;
    }

    template <typename TL, typename TR, typename TE> [[nodiscard]] constexpr TE matrix_add(TL const& lhs, TR const& rhs) noexcept
    {
      TE ret;
      for (index_t i = 0; i < ret.number_of_rows; ++i)
        for (index_t j = 0; j < ret.number_of_columns; ++j)
        {
          ret(i, j) = lhs(i, j) + rhs(i, j);
        }
      return ret;
    }

    template <typename TL, typename TR, typename TE> [[nodiscard]] constexpr TE matrix_sub(TL const& lhs, TR const& rhs) noexcept
    {
      TE ret;
      for (index_t i = 0; i < ret.number_of_rows; ++i)
        for (index_t j = 0; j < ret.number_of_columns; ++j)
        {
          ret(i, j) = lhs(i, j) - rhs(i, j);
        }
      return ret;
    }

    template <typename TM, typename TS, typename TE> [[nodiscard]] constexpr TE matrix_skalar_product(TM const& mat, TS const& sca) noexcept
    {
      TE ret;
      for (index_t i = 0; i < ret.number_of_rows; ++i)
        for (index_t j = 0; j < ret.number_of_columns; ++j)
        {
          ret(i, j) = mat(i, j) * sca;
        }
      return ret;
    }

    template <typename TM, typename TS, typename TE> [[nodiscard]] constexpr TE matrix_skalar_quotient(TM const& mat, TS const& sca) noexcept
    {
      TE ret;
      for (index_t i = 0; i < ret.number_of_rows; ++i)
        for (index_t j = 0; j < ret.number_of_columns; ++j)
        {
          ret(i, j) = mat(i, j) / sca;
        }
      return ret;
    }

    template <typename TL, typename TR, typename TE> [[nodiscard]] constexpr TE matrix_matrix_product(TL const& lhs, TR const& rhs) noexcept
    {
      TE ret;
      for (index_t i = 0; i < ret.number_of_rows; ++i)
        for (index_t j = 0; j < ret.number_of_columns; ++j)
        {
          typename TE::value_type val = 0;
          for (index_t k = 0; k < lhs.number_of_columns; ++k)
            val += lhs(i, k) * rhs(k, j);
          ret(i, j) = val;
        }
      return ret;
    }

    template <typename TL, typename TR> constexpr TL& matrix_add_assign(TL& lhs, TR const& rhs) noexcept
    {
      for (index_t i = 0; i < lhs.number_of_rows; ++i)
        for (index_t j = 0; j < lhs.number_of_columns; ++j)
        {
          lhs(i, j) += rhs(i, j);
        }
      return lhs;
    }

    template <typename TL, typename TR> constexpr TL& matrix_sub_assign(TL& lhs, TR const& rhs) noexcept
    {
      for (index_t i = 0; i < lhs.number_of_rows; ++i)
        for (index_t j = 0; j < lhs.number_of_columns; ++j)
        {
          lhs(i, j) -= rhs(i, j);
        }
      return lhs;
    }

    template <typename TM, typename TS> constexpr TM& matrix_skalar_product_assign(TM& lhs, TS const& sca) noexcept
    {
      for (index_t i = 0; i < lhs.number_of_rows; ++i)
        for (index_t j = 0; j < lhs.number_of_columns; ++j)
        {
          lhs(i, j) *= sca;
        }
      return lhs;
    }

    template <typename TM, typename TS> constexpr TM& matrix_skalar_quotient_assign(TM& lhs, TS const& sca) noexcept
    {
      for (index_t i = 0; i < lhs.number_of_rows; ++i)
        for (index_t j = 0; j < lhs.number_of_columns; ++j)
        {
          lhs(i, j) /= sca;
        }
      return lhs;
    }

    template <typename TI, typename TE> [[nodiscard]] constexpr TE transpose(TI const& val)
    {
      TE ret;
      for (index_t i = 0; i < val.number_of_rows; ++i)
        for (index_t j = 0; j < val.number_of_columns; ++j)
          ret(j, i) = val(i, j);
      return ret;
    }

    template <typename T, index_t N, index_t M> [[nodiscard]] constexpr matrix_t<T, N, M> matrix_solve(matrix_t<T, N, N> lhs, matrix_t<T, N, M> tmp)
    {
      for (index_t i = 0; i < tmp.number_of_rows; ++i)
      {
        {    // Find entry with highest abs value and bring it to the top scale it to 1
          index_t idx     = i;
          T       tmp_fac = 0;
          for (std::size_t k = i; k < tmp.number_of_rows; ++k)
          {
            T const tmp_abs_val = std::abs(lhs(k, i));
            if (tmp_abs_val > tmp_fac)
            {
              idx     = k;
              tmp_fac = tmp_abs_val;
            }
          }

          if (idx != i)
          {
            lhs.row_swap(i, idx);
            tmp.row_swap(i, idx);
          }
        }
        {
          T const tmp_div = lhs(i, i);
          lhs.row_div(i, tmp_div);
          tmp.row_div(i, tmp_div);
        }
        {
          for (std::size_t k = 0; k < tmp.number_of_rows; ++k)
          {
            if (k == i)
              continue;

            T const tmp_fac = lhs(k, i);
            lhs.row_sub_n_times_row(k, tmp_fac, i);
            tmp.row_sub_n_times_row(k, tmp_fac, i);
          }
        }
      }
      return tmp;
    }
  }    // namespace Internal

  // operator ==
  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr bool operator==(matrix_t<const T, N, M> const& lhs, matrix_t<const T, N, M> const& rhs)
  {
    return Internal::equal(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr bool operator==(matrix_t<T, N, M> const& lhs, matrix_t<const T, N, M> const& rhs)
  {
    return Internal::equal(lhs, rhs);
  }
  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr bool operator==(matrix_t<const T, N, M> const& lhs, matrix_t<T, N, M> const& rhs)
  {
    return Internal::equal(lhs, rhs);
  }
  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr bool operator==(matrix_t<T, N, M> const& lhs, matrix_t<T, N, M> const& rhs)
  {
    return Internal::equal(lhs, rhs);
  }

  // operator !=
  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr bool operator!=(matrix_t<const T, N, M> const& lhs, matrix_t<const T, N, M> const& rhs)
  {
    return Internal::not_equal(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr bool operator!=(matrix_t<T, N, M> const& lhs, matrix_t<const T, N, M> const& rhs)
  {
    return Internal::not_equal(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr bool operator!=(matrix_t<const T, N, M> const& lhs, matrix_t<T, N, M> const& rhs)
  {
    return Internal::not_equal(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr bool operator!=(matrix_t<T, N, M> const& lhs, matrix_t<T, N, M> const& rhs)
  {
    return Internal::not_equal(lhs, rhs);
  }

  // operator +
  template <typename T, index_t N, index_t M>
  [[nodiscard]] constexpr matrix_t<T, N, M> operator+(matrix_t<const T, N, M> const& lhs, matrix_t<const T, N, M> const& rhs)
  {
    return Internal::matrix_add<matrix_t<const T, N, M>, matrix_t<const T, N, M>, matrix_t<T, N, M>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M>
  [[nodiscard]] constexpr matrix_t<T, N, M> operator+(matrix_t<T, N, M> const& lhs, matrix_t<const T, N, M> const& rhs)
  {
    return Internal::matrix_add<matrix_t<T, N, M>, matrix_t<const T, N, M>, matrix_t<T, N, M>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M>
  [[nodiscard]] constexpr matrix_t<T, N, M> operator+(matrix_t<const T, N, M> const& lhs, matrix_t<T, N, M> const& rhs)
  {
    return Internal::matrix_add<matrix_t<const T, N, M>, matrix_t<T, N, M>, matrix_t<T, N, M>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr matrix_t<T, N, M> operator+(matrix_t<T, N, M> const& lhs, matrix_t<T, N, M> const& rhs)
  {
    return Internal::matrix_add<matrix_t<T, N, M>, matrix_t<T, N, M>, matrix_t<T, N, M>>(lhs, rhs);
  }

  // operator +=
  template <typename T, index_t N, index_t M> constexpr matrix_t<T, N, M>& operator+=(matrix_t<T, N, M>& lhs, matrix_t<const T, N, M> const& rhs)
  {
    return Internal::matrix_add_assign<matrix_t<T, N, M>, matrix_t<const T, N, M>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> constexpr matrix_t<T, N, M>& operator+=(matrix_t<T, N, M>& lhs, matrix_t<T, N, M> const& rhs)
  {
    return Internal::matrix_add_assign<matrix_t<T, N, M>, matrix_t<T, N, M>>(lhs, rhs);
  }

  // operator -
  template <typename T, index_t N, index_t M>
  [[nodiscard]] constexpr matrix_t<T, N, M> operator-(matrix_t<const T, N, M> const& lhs, matrix_t<const T, N, M> const& rhs)
  {
    return Internal::matrix_sub<matrix_t<const T, N, M>, matrix_t<const T, N, M>, matrix_t<T, N, M>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M>
  [[nodiscard]] constexpr matrix_t<T, N, M> operator-(matrix_t<T, N, M> const& lhs, matrix_t<const T, N, M> const& rhs)
  {
    return Internal::matrix_sub<matrix_t<T, N, M>, matrix_t<const T, N, M>, matrix_t<T, N, M>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M>
  [[nodiscard]] constexpr matrix_t<T, N, M> operator-(matrix_t<const T, N, M> const& lhs, matrix_t<T, N, M> const& rhs)
  {
    return Internal::matrix_sub<matrix_t<const T, N, M>, matrix_t<T, N, M>, matrix_t<T, N, M>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr matrix_t<T, N, M> operator-(matrix_t<T, N, M> const& lhs, matrix_t<T, N, M> const& rhs)
  {
    return Internal::matrix_sub<matrix_t<T, N, M>, matrix_t<T, N, M>, matrix_t<T, N, M>>(lhs, rhs);
  }

  // operator -=
  template <typename T, index_t N, index_t M> constexpr matrix_t<T, N, M>& operator-=(matrix_t<T, N, M>& lhs, matrix_t<const T, N, M> const& rhs)
  {
    return Internal::matrix_sub_assign<matrix_t<T, N, M>, matrix_t<const T, N, M>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> constexpr matrix_t<T, N, M>& operator-=(matrix_t<T, N, M>& lhs, matrix_t<T, N, M> const& rhs)
  {
    return Internal::matrix_sub_assign<matrix_t<T, N, M>, matrix_t<T, N, M>>(lhs, rhs);
  }

  // operator *
  template <typename T, index_t N, index_t M, index_t K>
  [[nodiscard]] constexpr matrix_t<T, N, K> operator*(matrix_t<const T, N, M> const& lhs, matrix_t<const T, M, K> const& rhs)
  {
    return Internal::matrix_matrix_product<matrix_t<const T, N, M>, matrix_t<const T, M, K>, matrix_t<T, N, K>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M, index_t K>
  [[nodiscard]] constexpr matrix_t<T, N, K> operator*(matrix_t<const T, N, M> const& lhs, matrix_t<T, M, K> const& rhs)
  {
    return Internal::matrix_matrix_product<matrix_t<const T, N, M>, matrix_t<T, M, K>, matrix_t<T, N, K>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M, index_t K>
  [[nodiscard]] constexpr matrix_t<T, N, K> operator*(matrix_t<T, N, M> const& lhs, matrix_t<const T, M, K> const& rhs)
  {
    return Internal::matrix_matrix_product<matrix_t<T, N, M>, matrix_t<const T, M, K>, matrix_t<T, N, K>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M, index_t K>
  [[nodiscard]] constexpr matrix_t<T, N, K> operator*(matrix_t<T, N, M> const& lhs, matrix_t<T, M, K> const& rhs)
  {
    return Internal::matrix_matrix_product<matrix_t<T, N, M>, matrix_t<T, M, K>, matrix_t<T, N, K>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr matrix_t<T, N, M> operator*(matrix_t<const T, N, M> const& lhs, T const& rhs)
  {
    return Internal::matrix_skalar_product<matrix_t<const T, N, M>, T, matrix_t<T, N, M>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr matrix_t<T, N, M> operator*(matrix_t<T, N, M> const& lhs, T const& rhs)
  {
    return Internal::matrix_skalar_product<matrix_t<T, N, M>, T, matrix_t<T, N, M>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr matrix_t<T, N, M> operator*(T const& lhs, matrix_t<const T, N, M> const& rhs)
  {
    return Internal::matrix_skalar_product<matrix_t<const T, N, M>, T, matrix_t<T, N, M>>(rhs, lhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr matrix_t<T, N, M> operator*(T const& lhs, matrix_t<T, N, M> const& rhs)
  {
    return Internal::matrix_skalar_product<matrix_t<T, N, M>, T, matrix_t<T, N, M>>(rhs, lhs);
  }

  // operator *=
  template <typename T, index_t N, index_t M> constexpr matrix_t<T, N, M> operator*=(matrix_t<T, N, M>& lhs, T const& rhs)
  {
    return Internal::matrix_skalar_product_assign<matrix_t<T, N, M>, T>(lhs, rhs);
  }

  // operator /
  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr matrix_t<T, N, M> operator/(matrix_t<const T, N, M> const& lhs, T const& rhs)
  {
    return Internal::matrix_skalar_quotient<matrix_t<const T, N, M>, T, matrix_t<T, N, M>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr matrix_t<T, N, M> operator/(matrix_t<T, N, M> const& lhs, T const& rhs)
  {
    return Internal::matrix_skalar_quotient<matrix_t<T, N, M>, T, matrix_t<T, N, M>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M>
  [[nodiscard]] constexpr matrix_t<T, N, M> operator/(matrix_t<const T, N, M> const& lhs, matrix_t<const T, 1, 1> const& rhs)
  {
    return Internal::matrix_skalar_quotient<matrix_t<const T, N, M>, T, matrix_t<T, N, M>>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr matrix_t<T, N, M> operator/(matrix_t<T, N, M> const& lhs, matrix_t<T, 1, 1> const& rhs)
  {
    return Internal::matrix_skalar_quotient<matrix_t<T, N, M>, T, matrix_t<T, N, M>>(lhs, rhs);
  }

  // operator /=
  template <typename T, index_t N, index_t M> constexpr matrix_t<T, N, M> operator/=(matrix_t<T, N, M>& lhs, T const& rhs)
  {
    return Internal::matrix_skalar_quotient_assign<matrix_t<T, N, M>, T>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr matrix_t<T, M, N> transpose(matrix_t<const T, N, M> const& val)
  {
    return Internal::transpose<matrix_t<const T, N, M>, matrix_t<T, M, N>>(val);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr matrix_t<T, M, N> transpose(matrix_t<T, N, M> const& val)
  {
    return Internal::transpose<matrix_t<T, N, M>, matrix_t<T, M, N>>(val);
  }

  template <typename T, index_t N, index_t M>
  [[nodiscard]] constexpr matrix_t<T, N, M> solve(matrix_t<const T, N, N> const& lhs, matrix_t<const T, N, M> const& rhs)
  {
    return Internal::matrix_solve<T, N, M>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr matrix_t<T, N, M> solve(matrix_t<const T, N, N> const& lhs, matrix_t<T, N, M> const& rhs)
  {
    return Internal::matrix_solve<T, N, M>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr matrix_t<T, N, M> solve(matrix_t<T, N, N> const& lhs, matrix_t<const T, N, M> const& rhs)
  {
    return Internal::matrix_solve<T, N, M>(lhs, rhs);
  }

  template <typename T, index_t N, index_t M> [[nodiscard]] constexpr matrix_t<T, N, M> solve(matrix_t<T, N, N> const& lhs, matrix_t<T, N, M> const& rhs)
  {
    return Internal::matrix_solve<T, N, M>(lhs, rhs);
  }

  template <typename T, index_t N> [[nodiscard]] constexpr matrix_t<T, N, N> inv(matrix_t<T, N, N> const& val)
  {
    exmath::matrix_t<T, N, N> rhs = {};
    for (index_t i = 0; i < N; i++)
      rhs(i, i) = T(1.0);

    return Internal::matrix_solve<T, N, N>(val, rhs);
  }
}    // namespace exmath

namespace std
{
  template <typename T, exmath::index_t N, exmath::index_t M> [[nodiscard]] constexpr exmath::matrix_t<T, N, M> abs(exmath::matrix_t<const T, N, M> const& val)
  {
    exmath::matrix_t<T, N, M> ret;
    for (exmath::index_t k = 0; k < ret.number_of_elements; ++k)
      ret[k] = std::abs(val[k]);
    return ret;
  }
  template <typename T, exmath::index_t N, exmath::index_t M> [[nodiscard]] constexpr exmath::matrix_t<T, N, M> abs(exmath::matrix_t<T, N, M> const& val)
  {
    exmath::matrix_t<T, N, M> ret;
    for (exmath::index_t k = 0; k < ret.number_of_elements; ++k)
      ret[k] = std::abs(val[k]);
    return ret;
  }
}    // namespace std

#endif
