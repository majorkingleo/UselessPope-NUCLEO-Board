#pragma once
#ifndef BSLIB_CONTAINER_SPSC_HPP_INCLUDED
#define BSLIB_CONTAINER_SPSC_HPP_INCLUDED

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <new>
#include <optional>
#include <type_traits>

namespace bslib::container
{
  template <typename T, std::size_t N>
  requires(N > 0 && std::is_destructible_v<T> && (std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>)) class SPSC
  {
  public:
    using payload_t = std::remove_cv_t<T>;

  private:
    using mem_payload_t                            = std::aligned_storage_t<sizeof(payload_t), alignof(payload_t)>;
    static constexpr std::size_t number_of_entries = N;
    static constexpr std::size_t max_idx           = number_of_entries + 1;

    static constexpr bool is_copy_constructible_v         = std::is_copy_constructible_v<payload_t>;
    static constexpr bool is_nothrow_copy_constructible_v = std::is_nothrow_copy_constructible_v<payload_t>;
    static constexpr bool is_move_constructible_v         = std::is_move_constructible_v<payload_t>;
    static constexpr bool is_nothrow_move_constructible_v = std::is_nothrow_move_constructible_v<payload_t>;
    static constexpr bool is_nothrow_destructible_v       = std::is_nothrow_destructible_v<payload_t>;

  public:
    inline constexpr SPSC() noexcept = default;

    inline ~SPSC() noexcept(is_nothrow_destructible_v)
    {
      std::size_t const w = this->m_w_idx;
      std::size_t       r = this->m_r_idx;
      while (r != w)
      {
        payload_t& tmp = *std::launder(reinterpret_cast<payload_t*>(&this->m_mem[r]));
        tmp.~payload_t();
        r = advance(r, 1);
      }
      this->m_r_idx = r;
    }

    template <typename = void> requires(is_copy_constructible_v) constexpr bool push_back(payload_t const& v) noexcept(is_nothrow_copy_constructible_v)
    {
      std::size_t const r      = this->m_r_idx;
      std::size_t const w      = this->m_w_idx;
      std::size_t const w_next = advance(w, 1);

      if (w_next == r)
        return false;

      ::new (&this->m_mem[w]) payload_t(v);

      this->m_w_idx = w_next;
      return true;
    }

    template <typename = void> requires(is_move_constructible_v) constexpr bool push_back(payload_t&& v) noexcept(is_nothrow_move_constructible_v)
    {
      std::size_t const r      = this->m_r_idx;
      std::size_t const w      = this->m_w_idx;
      std::size_t const w_next = advance(w, 1);

      if (w_next == r)
        return false;

      ::new (&this->m_mem[w]) payload_t(std::move(v));

      this->m_w_idx = w_next;
      return true;
    }

    constexpr std::optional<payload_t> pop_front() noexcept(is_move_constructible_v ? is_nothrow_move_constructible_v : is_nothrow_copy_constructible_v)
    {
      std::size_t const w = this->m_w_idx;
      std::size_t const r = this->m_r_idx;

      if (r == w)
        return std::nullopt;

      payload_deleter_t tmp{ *std::launder(reinterpret_cast<payload_t*>(&this->m_mem[r])), this->m_r_idx, advance(r, 1) };
      if constexpr (is_move_constructible_v)
      {
        return { std::move(tmp.obj) };
      }
      else
      {
        return { tmp.obj };
      }
    }

    constexpr std::size_t get_number_of_entries() const noexcept { return number_of_entries; }

    constexpr std::size_t get_number_of_used_entries() const noexcept
    {
      std::size_t const r = this->m_r_idx;
      std::size_t const w = this->m_w_idx;

      if (r <= w)
        return w - r;
      return max_idx + w - r;
    }

    constexpr std::size_t get_number_of_free_entries() const noexcept
    {
      std::size_t const r = this->m_r_idx;
      std::size_t const w = this->m_w_idx;

      if (r <= w)
        return number_of_entries + r - w;
      return r - w - 1;
    }

  private:
    static inline constexpr std::size_t advance(std::size_t idx, std::size_t val) noexcept
    {
      std::size_t ret = idx + val;
      while (ret >= max_idx)
      {
        ret -= max_idx;
      }
      return ret;
    }

    struct payload_deleter_t
    {
    public:
      inline constexpr payload_deleter_t(payload_t& obj, std::atomic<std::size_t>& r_idx, std::size_t next_idx_value) noexcept
          : obj(obj)
          , r_idx(r_idx)
          , next_idx(next_idx_value)
      {
      }

      inline ~payload_deleter_t() noexcept(is_nothrow_destructible_v)
      {
        this->obj.~payload_t();
        this->r_idx = this->next_idx;
      }

      payload_t&                obj;
      std::atomic<std::size_t>& r_idx;
      std::size_t               next_idx;
    };

    std::atomic<std::size_t> m_w_idx = 0;
    mem_payload_t            m_mem[max_idx]{};
    std::atomic<std::size_t> m_r_idx = 0;
  };
}    // namespace wlib::container

#endif
