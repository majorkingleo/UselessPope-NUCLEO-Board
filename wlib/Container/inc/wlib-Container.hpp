#pragma once
#ifndef WLIB_CONTAINER_HPP_INCLUDED
#define WLIB_CONTAINER_HPP_INCLUDED

#include <concepts>
#include <cstddef>
#include <iterator>

namespace wlib::container
{
  template <typename T, std::size_t N>
  requires(N > 0 && std::is_destructible_v<T> && std::is_default_constructible_v<T> && std::is_copy_assignable_v<T>) class circular_buffer_t
  {
    using payload_t                                = T;
    static constexpr std::size_t number_of_entries = N;
    static constexpr std::size_t buffer_length     = number_of_entries + 1;

    struct sentinel;
    class iterator
    {
    public:
      using difference_type   = std::ptrdiff_t;
      using value_type        = payload_t;
      using reference         = const payload_t&;
      using pointer           = const payload_t*;
      using iterator_category = std::forward_iterator_tag;

      constexpr iterator() noexcept = default;
      constexpr iterator(circular_buffer_t const& obj, std::size_t cur_index, std::size_t end_index) noexcept
          : m_obj{ &obj }
          , m_idx{ cur_index }
          , m_max{ end_index }
      {
      }

      constexpr reference const& operator*() const { return (*this->m_obj)[this->m_idx]; }

      constexpr iterator& operator++()
      {
        ++this->m_idx;
        return *this;
      }

      constexpr iterator operator++(int)
      {
        auto tmp = *this;
        ++*this;
        return tmp;
      }

      constexpr bool operator==(iterator const& other) const { return this->m_idx == other.m_idx; }
      constexpr bool operator!=(iterator const& other) const { return !(*this == other); }

      constexpr bool operator==(sentinel const&) const { return this->m_idx >= this->m_max; }
      constexpr bool operator!=(sentinel const&) const { return this->m_idx < this->m_max; }

    private:
      circular_buffer_t const* m_obj = nullptr;
      std::size_t              m_idx = 0;
      std::size_t              m_max = 0;
    };

    struct sentinel
    {
      constexpr bool operator==(iterator const& iter) const noexcept { return iter == *this; }
      constexpr bool operator!=(iterator const& iter) const noexcept { return iter != *this; }
    };

    static_assert(std::forward_iterator<iterator>);
    static_assert(std::sentinel_for<sentinel, iterator>);

  public:
    circular_buffer_t() = default;

    constexpr void push(payload_t const& value) noexcept
    {
      this->m_values[this->m_w_idx] = value;
      this->increment_index();
    }

    constexpr std::size_t capacity() const noexcept { return number_of_entries; }

    constexpr std::size_t occupied_entries() const noexcept
    {
      if (this->m_r_idx <= this->m_w_idx)
        return this->m_w_idx - this->m_r_idx;
      return number_of_entries;
    }

    payload_t const& operator[](std::size_t idx) const noexcept
    {
      if (idx < this->m_w_idx)
        return this->m_values[this->m_w_idx - idx - 1];
      return this->m_values[this->m_w_idx + buffer_length - idx - 1];
    }

    void clear() noexcept
    {
      this->m_w_idx = 0;
      this->m_r_idx = 0;
    }

    auto begin() const -> iterator { return iterator{ *this, 0, this->occupied_entries() }; }
    auto end() const -> sentinel { return sentinel{}; }

  private:
    static constexpr std::size_t increment_index(std::size_t idx) noexcept
    {
      if (++idx == buffer_length)
        idx = 0;
      return idx;
    }

    void increment_index() noexcept
    {
      this->m_w_idx = this->increment_index(this->m_w_idx);
      if (this->m_w_idx == this->m_r_idx)
        this->m_r_idx = this->increment_index(this->m_r_idx);
    }

    std::size_t m_w_idx                 = 0;
    std::size_t m_r_idx                 = 0;
    payload_t   m_values[buffer_length] = {};
  };

}    // namespace wlib::container

#endif    // !WLIB_CRC_INTERFACE_HPP
