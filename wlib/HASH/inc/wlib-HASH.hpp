#pragma once
#ifndef WLIB_HASH_HPP_INCLUDED
#define WLIB_HASH_HPP_INCLUDED

#include <array>
#include <bit>
#include <cstdint>
#include <span>

namespace wlib::hash
{
  class sha_256
  {
  public:
    using hash_t = std::array<std::byte, 32>;

  private:
    using chunk_t = std::array<std::byte, 64>;

    class internal_state_t
    {
    public:
      constexpr internal_state_t() noexcept = default;

      [[nodiscard]] uint32_t&        operator[](uint32_t idx) noexcept;
      [[nodiscard]] uint32_t const&  operator[](uint32_t idx) const noexcept;
      internal_state_t&              operator+=(internal_state_t const& rhs) noexcept;
      [[nodiscard]] internal_state_t operator+(internal_state_t const& rhs) const noexcept;
      [[nodiscard]] hash_t           to_hash() const noexcept;

    private:
      std::array<uint32_t, 8> m_value{ 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    };

  public:
    sha_256& operator()(std::span<std::byte const> const& data) noexcept;

    void reset() noexcept;

    [[nodiscard]] hash_t get() const noexcept;

  private:
    [[nodiscard]] static internal_state_t process_blk(internal_state_t state, chunk_t const& blk) noexcept;

    uint64_t         m_len = 0;
    uint32_t         m_idx = 0;
    chunk_t          m_blk{};
    internal_state_t m_internal_state{};
  };
}    // namespace wlib::hash

#endif    // !WLIB_CRC_INTERFACE_HPP
