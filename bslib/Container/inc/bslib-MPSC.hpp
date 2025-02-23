#pragma once
#ifndef BSLIB_CONTAINER_MPSC_HPP_INCLUDED
#define BSLIB_CONTAINER_MPSC_HPP_INCLUDED

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <new>
#include <optional>
#include <span>
#include <type_traits>

namespace bslib::container
{
  template <typename T> requires(std::is_destructible_v<T> && (std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>)) class mpsc_queue_ex_mem
  {

  public:
    using payload_t     = std::remove_cv_t<T>;
    using mem_payload_t = std::aligned_storage_t<sizeof(payload_t), alignof(payload_t)>;

  private:
    static constexpr bool is_copy_constructible_v         = std::is_copy_constructible_v<payload_t>;
    static constexpr bool is_nothrow_copy_constructible_v = std::is_nothrow_copy_constructible_v<payload_t>;
    static constexpr bool is_move_constructible_v         = std::is_move_constructible_v<payload_t>;
    static constexpr bool is_nothrow_move_constructible_v = std::is_nothrow_move_constructible_v<payload_t>;
    static constexpr bool is_nothrow_destructible_v       = std::is_nothrow_destructible_v<payload_t>;

  public:
    template <std::size_t N>
    requires(N > 1) inline mpsc_queue_ex_mem(mem_payload_t (&mem)[N])
        : mpsc_queue_ex_mem(mem, N)
    {
    }

    inline mpsc_queue_ex_mem(std::span<mem_payload_t> mem)
        : mpsc_queue_ex_mem(mem.data(), mem.size())
    {
    }

    inline mpsc_queue_ex_mem(mem_payload_t* mem, std::size_t const& number_of_elements)
        : m_mem{ mem }
        , m_mem_len{ number_of_elements }
    {
    }

    inline ~mpsc_queue_ex_mem() noexcept(is_nothrow_destructible_v)
    {
      std::size_t const w = this->m_idx_wr;
      std::size_t       r = this->m_idx_r;
      while (r != w)
      {
        payload_t& tmp = *std::launder(reinterpret_cast<payload_t*>(&this->m_mem[r]));
        tmp.~payload_t();
        r = advance_idx(r, 1);
      }
      this->m_idx_r = r;
    }

    constexpr std::size_t get_number_of_entries() const noexcept { return this->m_mem_len - 1; }

    constexpr std::size_t get_number_of_used_entries() const noexcept
    {
      std::size_t const r = this->m_idx_r;
      std::size_t const w = this->m_idx_ww;

      if (r <= w)
        return w - r;
      return this->m_mem_len + w - r;
    }

    constexpr std::size_t get_number_of_free_entries() const noexcept
    {
      std::size_t const r = this->m_idx_r;
      std::size_t const w = this->m_idx_ww;
      return this->calc_number_of_free_entries(r, w);
    }
    template <typename = void>
    requires(is_copy_constructible_v) constexpr bool push_back(payload_t const* v, std::size_t const& len) noexcept(is_nothrow_copy_constructible_v)
    {
      this->m_idx_tmp += len;
      bool const ret = this->p_push_back(v, len);

      std::size_t const w = this->m_idx_ww;
      if (this->m_idx_tmp == len)
      {
        this->m_idx_wr = w;
      }
      this->m_idx_tmp -= len;
      return ret;
    }

    template <typename = void> requires(is_copy_constructible_v) constexpr bool push_back(payload_t const& v) noexcept(is_nothrow_copy_constructible_v)
    {
      return this->push_back(&v, 1);
    }

    template <typename = void> requires(is_move_constructible_v) constexpr bool push_back(payload_t&& v) noexcept(is_nothrow_move_constructible_v)
    {
      this->m_idx_tmp += 1;
      bool const ret           = this->p_push_back(std::move(v));
      bool       wr_idx_update = false;

      std::size_t const w = this->m_idx_ww;
      if (this->m_idx_tmp == 1)
      {
        this->m_idx_wr = w;
      }
      this->m_idx_tmp -= 1;
      return ret;
    }

    std::span<payload_t> peak_span()
    {
      std::size_t const r_idx = this->m_idx_r;
      std::size_t const w_idx = this->m_idx_wr;

      if (r_idx <= w_idx)
        return { reinterpret_cast<payload_t*>(&this->m_mem[r_idx]), w_idx - r_idx };
      return { reinterpret_cast<payload_t*>(&this->m_mem[r_idx]), this->m_mem_len - r_idx };
    }

    void drop(std::size_t const& len)
    {
      std::size_t r = this->m_idx_r;
      for (std::size_t i = 0; i < len; i++)
      {
        payload_t& tmp = *std::launder(reinterpret_cast<payload_t*>(&this->m_mem[r]));
        tmp.~payload_t();
        r = advance_idx(r, 1);
      }

      this->m_idx_r = this->advance_idx(this->m_idx_r, len);
    }

    constexpr std::optional<payload_t> pop_front() noexcept(is_move_constructible_v ? is_nothrow_move_constructible_v : is_nothrow_copy_constructible_v)
    {
      std::size_t const w = this->m_idx_wr;
      std::size_t const r = this->m_idx_r;

      if (r == w)
        return std::nullopt;

      payload_deleter_t tmp{ *std::launder(reinterpret_cast<payload_t*>(&this->m_mem[r])), this->m_r_idx, advance_idx(r, 1) };
      if constexpr (is_move_constructible_v)
      {
        return { std::move(tmp.obj) };
      }
      else
      {
        return { tmp.obj };
      }
    }

  private:
    template <typename = void>
    requires(is_copy_constructible_v) constexpr bool p_push_back(payload_t const* v, std::size_t const& len) noexcept(is_nothrow_copy_constructible_v)
    {
      std::size_t const r_idx = this->m_idx_r;
      std::size_t       w_idx = this->m_idx_ww;
      do
      {
        if (this->calc_number_of_free_entries(r_idx, w_idx) < len)
        {
          return false;
        }
      } while (!this->m_idx_ww.compare_exchange_weak(w_idx, this->advance_idx(w_idx, len)));

      for (std::size_t i = 0; i < len; i++)
      {
        ::new (&this->m_mem[w_idx]) payload_t(v[i]);
        w_idx = this->advance_idx(w_idx, 1);
      }
      return true;
    }

    template <typename = void> requires(is_move_constructible_v) constexpr bool p_push_back(payload_t&& v) noexcept(is_nothrow_move_constructible_v)
    {
      std::size_t const r_idx = this->m_idx_r;
      std::size_t       w_idx = this->m_idx_ww;
      do
      {
        if (this->calc_number_of_free_entries(r_idx, w_idx) < 1)
        {
          return false;
        }
      } while (!this->m_idx_ww.compare_exchange_weak(w_idx, this->advance_idx(w_idx, 1)));

      ::new (&this->m_mem[w_idx]) payload_t(std::move(v));

      return true;
    }

    inline constexpr std::size_t calc_number_of_free_entries(std::size_t const& r_idx, std::size_t const& w_idx) const noexcept
    {
      if (r_idx <= w_idx)
        return this->m_mem_len + r_idx - w_idx - 1;
      return r_idx - w_idx - 1;
    }

    inline constexpr std::size_t advance_idx(std::size_t idx, std::size_t val) noexcept
    {
      std::size_t ret = idx + val;
      while (ret >= this->m_mem_len)
      {
        ret -= this->m_mem_len;
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

    std::atomic<std::size_t> m_idx_tmp = 0;
    std::atomic<std::size_t> m_idx_ww  = 0;
    std::atomic<std::size_t> m_idx_wr  = 0;
    std::atomic<std::size_t> m_idx_r   = 0;
    mem_payload_t*           m_mem     = {};
    std::size_t const        m_mem_len = {};
  };
}    // namespace wlib::container

#endif
