#pragma once
#ifndef WLIB_CRC_16_CCITT_FALSE_HPP_INCLUDED
#define WLIB_CRC_16_CCITT_FALSE_HPP_INCLUDED

#include <wlib-CRC_Interface.hpp>
#include <cstddef>
#include <cstdint>

namespace wlib::crc
{
  class CRC_16_ccitt_false final: public CRC_Interface<uint16_t>
  {
  public:
    using base_t = CRC_Interface<uint16_t>;

    virtual used_type get_inital_value() const noexcept override { return CRC_16_ccitt_false::init_value; }
    virtual void      reset() noexcept override { this->m_crc = CRC_16_ccitt_false::init_value; }
    virtual used_type get() const noexcept override { return this->m_crc; }

    using base_t::operator();

    virtual used_type operator()(std::byte const* beg, std::byte const* end) noexcept override;

  private:
    static constexpr used_type init_value = 0xFFFF;
    used_type                  m_crc      = init_value;
  };
  class CRC_16_ccitt_zero final: public CRC_Interface<uint16_t>
  {
  public:
    using base_t = CRC_Interface<uint16_t>;

    virtual used_type get_inital_value() const noexcept override { return CRC_16_ccitt_zero::init_value; }
    virtual void      reset() noexcept override { this->m_crc = CRC_16_ccitt_zero::init_value; }
    virtual used_type get() const noexcept override { return this->m_crc; }

    using base_t::operator();

    virtual used_type operator()(std::byte const* beg, std::byte const* end) noexcept override;

  private:
    static constexpr used_type init_value = 0x0000;
    used_type                  m_crc      = init_value;
  };
}    // namespace wlib::crc
#endif
