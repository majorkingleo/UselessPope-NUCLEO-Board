#pragma once
#ifndef WLIB_STORAGE_HPP_INCLUDED
#define WLIB_STORAGE_HPP_INCLUDED

#include <cstdint>
#include <optional>
#include <span>
#include <wlib-CRC.hpp>
#include <wlib-Provider_Interface.hpp>
#include <wlib-memory.hpp>
#include <wlib-BLOB.hpp>

namespace wlib::storage
{
  template <typename T> class Non_Volatile_Storage_Interface
  {
  public:
    using value_type = std::remove_cvref_t<T>;

    virtual ~Non_Volatile_Storage_Interface() = default;

    virtual value_type load() const            = 0;
    virtual void       save(value_type const&) = 0;
  };
}    // namespace wlib::storage

namespace wlib::storage::strategy
{

  template <typename T> class mirrow_storage_t: public wlib::storage::Non_Volatile_Storage_Interface<T>
  {
    using crc_t      = wlib::crc::CRC_64_go_iso;
    using value_type = typename wlib::storage::Non_Volatile_Storage_Interface<T>::value_type;

  public:
    mirrow_storage_t(wlib::memory::Non_Volatile_Memory_Interface& mem,
                     std::size_t                                  blk_size,
                     std::array<std::size_t, 2> const&            addresses,
                     Shared_Memory_Provider_Interface&            memory_provider)
        : m_mem{ mem }
        , m_blk_sz{ blk_size }
        , m_add{ addresses }
        , m_buffer_pro(memory_provider)
    {
      try
      {
        auto                 obj = this->m_buffer_pro.request();
        std::span<std::byte> tmp = obj.get().subspan(0, this->m_blk_sz);

        this->m_mem.read(this->m_add[0], tmp);
        auto const val_1 = p_check(tmp);

        this->m_mem.read(this->m_add[1], tmp);
        auto const val_2 = p_check(tmp);

        if (val_1.has_value())
        {
          this->m_val = val_1.value();

          if (val_2.has_value() && val_2.value() == this->m_val)
            return;

          this->p_recover_2(tmp);
        }
        else if (val_2.has_value())
        {
          this->m_val = val_2.value();

          this->p_recover_1(tmp);
        }
      }
      catch (...)
      {
      }
    }

    value_type load() const override { return this->m_val; }
    void       save(value_type const& value) override
    {
      if (this->m_val == value)
        return;

      this->m_val = value;

      auto obj = this->m_buffer_pro.request();
      auto tmp = this->p_serialize(obj.get());
      this->m_mem.write(this->m_add[0], tmp);
      this->m_mem.write(this->m_add[1], tmp);
      this->m_mem.flush();
    }

  private:
    std::size_t get_begin_of_crc() const { return this->m_blk_sz - sizeof(crc_t::used_type); }

    std::optional<value_type> p_check(std::span<std::byte const> buffer)
    {
      try
      {
        wlib::blob::ConstMemoryBlob blob{ buffer };
        crc_t::used_type            crc_in   = blob.read<crc_t::used_type>(this->get_begin_of_crc());
        crc_t::used_type            crc_calc = crc_t()(buffer.data(), this->get_begin_of_crc());
        if (crc_in != crc_calc)
          return std::nullopt;

        value_type ret = {};
        blob >> ret;
        return ret;
      }
      catch (...)
      {
        return std::nullopt;
      }
    }

    std::span<std::byte> p_serialize(std::span<std::byte> tmp)
    {
      wlib::blob::MemoryBlob blob{ tmp };
      blob << this->m_val;

      blob.insert_back(std::byte(0x00), this->get_begin_of_crc() - blob.get_number_of_used_bytes());
      blob.insert_back(crc_t()(blob.get_span()));

      return blob.get_span();
    }

    void p_recover_1(std::span<std::byte> buffer)
    {
      auto tmp = this->p_serialize(buffer);
      this->m_mem.write(this->m_add[0], tmp);
      this->m_mem.flush();
    }

    void p_recover_2(std::span<std::byte> buffer)
    {
      auto tmp = this->p_serialize(buffer);
      this->m_mem.write(this->m_add[1], tmp);
      this->m_mem.flush();
    }

    wlib::memory::Non_Volatile_Memory_Interface& m_mem;
    std::size_t                                  m_blk_sz;
    std::array<std::size_t, 2>                   m_add = {};
    Shared_Memory_Provider_Interface&            m_buffer_pro;

    value_type m_val = {};
  };
}    // namespace wlib::storage::strategy

#endif    // WLIB_MEMORY_INTERFACE_HPP_INCLUDED
