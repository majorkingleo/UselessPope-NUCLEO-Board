#pragma once
#ifndef WLIB_CRC_INTERFACE_HPP_INCLUDED
#define WLIB_CRC_INTERFACE_HPP_INCLUDED

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace wlib::crc
{
  template <typename T> class CRC_Interface
  {
  public:
    using used_type = typename std::remove_cv_t<T>;

    virtual ~CRC_Interface()                            = default;
    virtual used_type get_inital_value() const noexcept = 0;
    virtual void      reset() noexcept                  = 0;
    virtual used_type get() const noexcept              = 0;

    used_type         operator()(std::byte const& value) noexcept { return this->operator()(&value, 1); }
    virtual used_type operator()(std::byte const* beg, std::byte const* end) noexcept = 0;
    used_type         operator()(std::byte const* beg, std::size_t const& len) noexcept { return this->operator()(beg, beg + len); }

    used_type         operator()(std::span<std::byte const> const& span) noexcept { return this->operator()(span.data(), span.size_bytes()); }
    used_type         operator()(std::span<std::byte> const& span) noexcept { return this->operator()(span.data(), span.size_bytes()); }
  };
}    // namespace wlib::crc
#endif    // !WLIB_CRC_INTERFACE_HPP
