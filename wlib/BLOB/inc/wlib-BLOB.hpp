#pragma once
#ifndef WLIB_BLOB_HPP_INCLUDED
#define WLIB_BLOB_HPP_INCLUDED

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <limits>
#include <span>

namespace wlib::blob
{
  namespace internal
  {
    inline std::size_t data_shift_right(std::span<std::byte> data, std::size_t offset, std::size_t shift)
    {
      for (std::size_t i = data.size(); offset < i;)
      {
        --i;
        data.data()[i + shift] = data[i];
      }
      return shift;
    }
    inline std::size_t data_shift_left(std::span<std::byte> data, std::size_t offset, std::size_t shift)
    {
      for (std::size_t i = offset; i < (data.size() - shift); i++)
      {
        data[i] = data[i + shift];
      }
      return shift;
    }

    inline std::size_t byte_copy(std::span<std::byte> trg, std::byte const* src)
    {
      for (std::byte& ent : trg)
        ent = *src++;
      return trg.size();
    }

    inline std::size_t byte_copy_reverse(std::span<std::byte> trg, std::byte const* src)
    {
      src += trg.size();
      for (std::byte& ent : trg)
        ent = *--src;
      return trg.size();
    }

    inline std::size_t byte_fill(std::span<std::byte> trg, std::byte src)
    {
      for (std::byte& ent : trg)
        ent = src;
      return trg.size();
    }

    void handle_overwrite_exception();
    void handle_insert_exception();
    void handle_remove_exception();
    void handle_read_exception();
    void handle_position_exception();
  }    // namespace internal

  template <typename T>
  concept ArithmeticOrByte = std::is_arithmetic_v<T> || std::is_same_v<T, std::byte>;

  class ConstMemoryBlob
  {
  public:
    constexpr ConstMemoryBlob(std::span<std::byte const> data) noexcept
        : m_data{ data }
        , m_idx_front(0)
        , m_idx_back(data.size())
    {
    }

    ConstMemoryBlob(ConstMemoryBlob const&)            = delete;
    ConstMemoryBlob(ConstMemoryBlob&&)                 = delete;
    ConstMemoryBlob& operator=(ConstMemoryBlob const&) = delete;
    ConstMemoryBlob& operator=(ConstMemoryBlob&&)      = delete;
    virtual ~ConstMemoryBlob()                         = default;

    [[nodiscard]] constexpr std::size_t get_total_number_of_bytes() const noexcept { return this->m_data.size(); }
    [[nodiscard]] constexpr std::size_t get_number_of_remaining_bytes() const noexcept { return this->m_idx_back - this->m_idx_front; }
    [[nodiscard]] constexpr std::size_t get_number_of_processed_bytes() const noexcept { return this->m_data.size() - this->m_idx_back + this->m_idx_front; }
    [[nodiscard]] constexpr std::span<std::byte const> get_span() const noexcept
    {
      return this->m_data.subspan(this->m_idx_front, this->m_idx_back - this->m_idx_front);
    }

    constexpr void reset() noexcept
    {
      this->m_idx_front = 0;
      this->m_idx_back  = this->m_data.size();
    }

    constexpr bool try_read(std::size_t offset, std::span<std::byte> target) const noexcept
    {
      if (!this->range_check_read(offset, target.size()))
        return false;

      internal::byte_copy(target, &this->m_data[this->m_idx_front + offset]);
      return true;
    }
    constexpr bool try_read_back(std::span<std::byte> target) const noexcept
    {
      if (this->get_number_of_remaining_bytes() < target.size())
        return false;
      return this->try_read(this->get_number_of_remaining_bytes() - target.size(), target);
    }
    constexpr bool try_read_front(std::span<std::byte> target) const noexcept { return try_read(0, target); }

    constexpr bool try_read_reverse(std::size_t offset, std::span<std::byte> target) const noexcept
    {
      if (!this->range_check_read(offset, target.size()))
        return false;

      internal::byte_copy_reverse(target, &this->m_data[this->m_idx_front + offset]);
      return true;
    }
    constexpr bool try_read_back_reverse(std::span<std::byte> target) const noexcept
    {
      if (this->get_number_of_remaining_bytes() < target.size())
        return false;
      return this->try_read_reverse(this->get_number_of_remaining_bytes() - target.size(), target);
    }
    constexpr bool try_read_front_reverse(std::span<std::byte> target) const noexcept { return try_read_reverse(0, target); }

    template <ArithmeticOrByte T> bool try_read(std::size_t offset, T& value, std::endian endian = std::endian::native) const noexcept
    {
      if (endian == std::endian::native)
        return this->try_read(offset, std::span<std::byte>(reinterpret_cast<std::byte*>(&value), sizeof(T)));
      else
        return this->try_read_reverse(offset, std::span<std::byte>(reinterpret_cast<std::byte*>(&value), sizeof(T)));
    }
    template <ArithmeticOrByte T> bool try_read_back(T& value, std::endian endian = std::endian::native) const noexcept
    {
      if (endian == std::endian::native)
        return this->try_read_back(std::span<std::byte>(reinterpret_cast<std::byte*>(&value), sizeof(T)));
      else
        return this->try_read_back_reverse(std::span<std::byte>(reinterpret_cast<std::byte*>(&value), sizeof(T)));
    }
    template <ArithmeticOrByte T> bool try_read_front(T& value, std::endian endian = std::endian::native) const noexcept
    {
      if (endian == std::endian::native)
        return this->try_read_front(std::span<std::byte>(reinterpret_cast<std::byte*>(&value), sizeof(T)));
      else
        return this->try_read_front_reverse(std::span<std::byte>(reinterpret_cast<std::byte*>(&value), sizeof(T)));
    }

    template <ArithmeticOrByte T> [[nodiscard]] T read(std::size_t offset, std::endian endian = std::endian::native) const
    {
      T ret{};
      if (!this->try_read(offset, ret, endian))
        internal::handle_read_exception();
      return ret;
    }
    template <ArithmeticOrByte T> [[nodiscard]] T read_back(std::endian endian = std::endian::native) const
    {
      T ret{};
      if (!this->try_read_back(ret, endian))
        internal::handle_read_exception();
      return ret;
    }
    template <ArithmeticOrByte T> [[nodiscard]] T read_front(std::endian endian = std::endian::native) const
    {
      T ret{};
      if (!this->try_read_front(ret, endian))
        internal::handle_read_exception();
      return ret;
    }

    bool try_remove_back(std::size_t number_of_bytes = 1) noexcept
    {
      if (this->get_number_of_remaining_bytes() < number_of_bytes)
        return false;
      this->m_idx_back -= number_of_bytes;
      return true;
    }
    bool try_remove_front(std::size_t number_of_bytes = 1) noexcept
    {
      if (this->get_number_of_remaining_bytes() < number_of_bytes)
        return false;
      this->m_idx_front += number_of_bytes;
      return true;
    }

    template <ArithmeticOrByte T> bool try_remove_back() noexcept { return this->try_remove_back(sizeof(T)); }
    template <ArithmeticOrByte T> bool try_remove_front() noexcept { return this->try_remove_front(sizeof(T)); }

    template <ArithmeticOrByte T> bool try_extract_back(T& value, std::endian endian = std::endian::native) noexcept
    {
      return this->try_read_back(value, endian) && this->try_remove_back<T>();
    }
    template <ArithmeticOrByte T> bool try_extract_front(T& value, std::endian endian = std::endian::native) noexcept
    {
      return this->try_read_front(value, endian) && this->try_remove_front<T>();
    }

    void remove_back(std::size_t number_of_bytes = 1)
    {
      if (!this->try_remove_back(number_of_bytes))
        return internal::handle_remove_exception();
    }

    void remove_front(std::size_t number_of_bytes = 1)
    {
      if (!this->try_remove_front(number_of_bytes))
        return internal::handle_remove_exception();
    }

    template <ArithmeticOrByte T> [[nodiscard]] T extract_back(std::endian endian = std::endian::native)
    {
      T ret{};
      if (!this->try_extract_back(ret, endian))
        internal::handle_read_exception();
      return ret;
    }
    template <ArithmeticOrByte T> [[nodiscard]] T extract_front(std::endian endian = std::endian::native)
    {
      T ret{};
      if (!this->try_extract_front(ret, endian))
        internal::handle_read_exception();
      return ret;
    }

  private:
    constexpr bool range_check_read(std::size_t offset, std::size_t number_of_byte_to_read) const noexcept
    {
      if ((std::numeric_limits<std::size_t>::max() - this->m_idx_front) < offset)
        return false;

      if ((std::numeric_limits<std::size_t>::max() - (this->m_idx_front + offset)) < number_of_byte_to_read)
        return false;

      if (this->m_idx_back < (this->m_idx_front + offset + number_of_byte_to_read))
        return false;

      return true;
    }

    std::span<std::byte const> m_data;
    std::size_t                m_idx_front;
    std::size_t                m_idx_back;
  };

  class MemoryBlob
  {
  public:
    constexpr MemoryBlob(std::span<std::byte> data, std::size_t position_idx = 0) noexcept
        : m_data(data)
        , m_pos_idx(position_idx)
    {
    }

    MemoryBlob(MemoryBlob const&)            = delete;
    MemoryBlob(MemoryBlob&&)                 = delete;
    MemoryBlob& operator=(MemoryBlob const&) = delete;
    MemoryBlob& operator=(MemoryBlob&&)      = delete;
    virtual ~MemoryBlob()                    = default;

    [[nodiscard]] constexpr std::size_t                get_total_number_of_bytes() const noexcept { return this->m_data.size(); }
    [[nodiscard]] constexpr std::size_t                get_number_of_free_bytes() const noexcept { return this->m_data.size() - this->m_pos_idx; }
    [[nodiscard]] constexpr std::size_t                get_number_of_used_bytes() const noexcept { return this->m_pos_idx; }
    [[nodiscard]] constexpr std::span<std::byte const> get_span() const noexcept { return std::span<std::byte const>(this->m_data.data(), this->m_pos_idx); }
    [[nodiscard]] constexpr std::span<std::byte>       get_span() noexcept { return std::span<std::byte>(this->m_data.data(), this->m_pos_idx); }
    constexpr void                                     clear() noexcept { this->m_pos_idx = 0; }
    constexpr bool                                     try_adjust_position(std::ptrdiff_t offset) noexcept
    {
      if ((offset > 0) && (this->m_pos_idx + offset) > this->m_data.size())
        return false;
      if ((offset < 0) && (this->m_pos_idx < static_cast<std::size_t>(-offset)))
        return false;

      this->m_pos_idx += offset;
      return true;
    }
    constexpr bool try_set_position(std::size_t position) noexcept
    {
      if (this->m_data.size() < position)
        return false;
      this->m_pos_idx = position;
      return true;
    }
    void adjust_position(std::ptrdiff_t offset)
    {
      if (!this->try_adjust_position(offset))
        return internal::handle_position_exception();
    }
    void set_position(std::size_t position)
    {
      if (!this->try_set_position(position))
        return internal::handle_position_exception();
    }

    constexpr bool try_read(std::size_t offset, std::span<std::byte> target) const noexcept
    {
      if (!this->range_check_read(offset, target.size()))
        return false;

      internal::byte_copy(target, &this->m_data[offset]);
      return true;
    }
    constexpr bool try_read_back(std::span<std::byte> target) const noexcept
    {
      if (this->get_number_of_used_bytes() < target.size())
        return false;
      return this->try_read(this->get_number_of_used_bytes() - target.size(), target);
    }
    constexpr bool try_read_front(std::span<std::byte> target) const noexcept { return try_read(0, target); }

    constexpr bool try_read_reverse(std::size_t offset, std::span<std::byte> target) const noexcept
    {
      if (!this->range_check_read(offset, target.size()))
        return false;

      internal::byte_copy_reverse(target, &this->m_data[offset]);
      return true;
    }
    constexpr bool try_read_back_reverse(std::span<std::byte> target) const noexcept
    {
      if (this->get_number_of_used_bytes() < target.size())
        return false;
      return this->try_read_reverse(this->get_number_of_used_bytes() - target.size(), target);
    }
    constexpr bool try_read_front_reverse(std::span<std::byte> target) const noexcept { return try_read_reverse(0, target); }

    template <ArithmeticOrByte T> bool try_read(std::size_t offset, T& value, std::endian endian = std::endian::native) const noexcept
    {
      if (endian == std::endian::native)
        return this->try_read(offset, std::span<std::byte>(reinterpret_cast<std::byte*>(&value), sizeof(T)));
      else
        return this->try_read_reverse(offset, std::span<std::byte>(reinterpret_cast<std::byte*>(&value), sizeof(T)));
    }
    template <ArithmeticOrByte T> bool try_read_back(T& value, std::endian endian = std::endian::native) const noexcept
    {
      if (endian == std::endian::native)
        return this->try_read_back(std::span<std::byte>(reinterpret_cast<std::byte*>(&value), sizeof(T)));
      else
        return this->try_read_back_reverse(std::span<std::byte>(reinterpret_cast<std::byte*>(&value), sizeof(T)));
    }
    template <ArithmeticOrByte T> bool try_read_front(T& value, std::endian endian = std::endian::native) const noexcept
    {
      if (endian == std::endian::native)
        return this->try_read_front(std::span<std::byte>(reinterpret_cast<std::byte*>(&value), sizeof(T)));
      else
        return this->try_read_front_reverse(std::span<std::byte>(reinterpret_cast<std::byte*>(&value), sizeof(T)));
    }

    template <ArithmeticOrByte T> [[nodiscard]] T read(std::size_t offset, std::endian endian = std::endian::native) const
    {
      T ret{};
      if (!this->try_read(offset, ret, endian))
        internal::handle_read_exception();
      return ret;
    }
    template <ArithmeticOrByte T> [[nodiscard]] T read_back(std::endian endian = std::endian::native) const
    {
      T ret{};
      if (!this->try_read_back(ret, endian))
        internal::handle_read_exception();
      return ret;
    }
    template <ArithmeticOrByte T> [[nodiscard]] T read_front(std::endian endian = std::endian::native) const
    {
      T ret{};
      if (!this->try_read_front(ret, endian))
        internal::handle_read_exception();
      return ret;
    }

    bool try_overwrite(std::size_t offset, std::span<std::byte const> data) noexcept
    {
      if (!this->range_check_read(offset, data.size()))
        return false;

      internal::byte_copy(this->m_data.subspan(offset, data.size()), data.data());
      return true;
    }
    bool try_overwrite_back(std::span<std::byte const> data) noexcept { return this->try_overwrite(this->m_pos_idx - data.size(), data); }
    bool try_overwrite_front(std::span<std::byte const> data) noexcept { return this->try_overwrite(0, data); }

    bool try_overwrite_reverse(std::size_t offset, std::span<std::byte const> data) noexcept
    {
      if (!this->range_check_read(offset, data.size()))
        return false;

      internal::byte_copy_reverse(this->m_data.subspan(offset, data.size()), data.data());
      return true;
    }
    bool try_overwrite_back_reverse(std::span<std::byte const> data) noexcept { return this->try_overwrite_reverse(this->m_pos_idx - data.size(), data); }
    bool try_overwrite_front_reverse(std::span<std::byte const> data) noexcept { return this->try_overwrite_reverse(0, data); }

    template <ArithmeticOrByte T> bool try_overwrite(std::size_t offset, T const& value, std::endian endian = std::endian::native) noexcept
    {
      if (endian == std::endian::native)
        return this->try_overwrite(offset, { reinterpret_cast<std::byte const*>(&value), sizeof(T) });
      else
        return this->try_overwrite_reverse(offset, { reinterpret_cast<std::byte const*>(&value), sizeof(T) });
    }
    template <ArithmeticOrByte T> bool try_overwrite_back(T const& value, std::endian endian = std::endian::native) noexcept
    {
      if (endian == std::endian::native)
        return this->try_overwrite_back({ reinterpret_cast<std::byte const*>(&value), sizeof(T) });
      else
        return this->try_overwrite_back_reverse({ reinterpret_cast<std::byte const*>(&value), sizeof(T) });
    }
    template <ArithmeticOrByte T> bool try_overwrite_front(T const& value, std::endian endian = std::endian::native) noexcept
    {
      if (endian == std::endian::native)
        return this->try_overwrite_front({ reinterpret_cast<std::byte const*>(&value), sizeof(T) });
      else
        return this->try_overwrite_front_reverse({ reinterpret_cast<std::byte const*>(&value), sizeof(T) });
    }

    template <ArithmeticOrByte T> void overwrite(std::size_t offset, T const& value, std::endian endian = std::endian::native)
    {
      if (!this->try_overwrite(offset, value, endian))
        internal::handle_overwrite_exception();
    }
    template <ArithmeticOrByte T> void overwrite_back(T const& value, std::endian endian = std::endian::native)
    {
      if (!this->try_overwrite_back(value, endian))
        internal::handle_overwrite_exception();
    }
    template <ArithmeticOrByte T> void overwrite_front(T const& value, std::endian endian = std::endian::native)
    {
      if (!this->try_overwrite_front(value, endian))
        internal::handle_overwrite_exception();
    }

    bool try_insert(std::size_t offset, std::byte data, std::size_t count) noexcept
    {
      if (count == 0)
        return true;

      if (!this->range_check_insert(offset, count))
        return false;

      internal::data_shift_right(this->get_span(), offset, count);
      this->m_pos_idx += internal::byte_fill(this->m_data.subspan(offset, count), data);
      return true;
    }
    bool try_insert_back(std::byte data, std::size_t count) noexcept { return this->try_insert(this->m_pos_idx, data, count); }
    bool try_insert_front(std::byte data, std::size_t count) noexcept { return this->try_insert(0, data, count); }

    bool try_insert(std::size_t offset, std::span<std::byte const> data) noexcept
    {
      if (!this->range_check_insert(offset, data.size()))
        return false;

      internal::data_shift_right(this->get_span(), offset, data.size());
      this->m_pos_idx += internal::byte_copy(this->m_data.subspan(offset, data.size()), data.data());
      return true;
    }
    bool try_insert_back(std::span<std::byte const> data) noexcept { return this->try_insert(this->m_pos_idx, data); }
    bool try_insert_front(std::span<std::byte const> data) noexcept { return this->try_insert(0, data); }

    bool try_insert_reverse(std::size_t offset, std::span<std::byte const> data) noexcept
    {
      if (!this->range_check_insert(offset, data.size()))
        return false;

      internal::data_shift_right(this->get_span(), offset, data.size());
      this->m_pos_idx += internal::byte_copy_reverse(this->m_data.subspan(offset, data.size()), data.data());
      return true;
    }
    bool try_insert_back_reverse(std::span<std::byte const> data) noexcept { return this->try_insert_reverse(this->m_pos_idx, data); }
    bool try_insert_front_reverse(std::span<std::byte const> data) noexcept { return this->try_insert_reverse(0, data); }

    template <ArithmeticOrByte T> bool try_insert(std::size_t offset, T const& value, std::endian endian = std::endian::native) noexcept
    {
      if (endian == std::endian::native)
        return this->try_insert(offset, { reinterpret_cast<std::byte const*>(&value), sizeof(T) });
      else
        return this->try_insert_reverse(offset, { reinterpret_cast<std::byte const*>(&value), sizeof(T) });
    }
    template <ArithmeticOrByte T> bool try_insert_back(T const& value, std::endian endian = std::endian::native) noexcept
    {
      if (endian == std::endian::native)
        return this->try_insert_back({ reinterpret_cast<std::byte const*>(&value), sizeof(T) });
      else
        return this->try_insert_back_reverse({ reinterpret_cast<std::byte const*>(&value), sizeof(T) });
    }
    template <ArithmeticOrByte T> bool try_insert_front(T const& value, std::endian endian = std::endian::native) noexcept
    {
      if (endian == std::endian::native)
        return this->try_insert_front({ reinterpret_cast<std::byte const*>(&value), sizeof(T) });
      else
        return this->try_insert_front_reverse({ reinterpret_cast<std::byte const*>(&value), sizeof(T) });
    }


    void insert(std::size_t offset, std::byte data, std::size_t count)
    {
      if (!this->try_insert(offset, data, count))
        internal::handle_insert_exception();
    }
    void insert_back(std::byte data, std::size_t count)
    {
      if (!this->try_insert_back(data, count))
        internal::handle_insert_exception();
    }
    void insert_front(std::byte data, std::size_t count)
    {
      if (!this->try_insert_front(data, count))
        internal::handle_insert_exception();
    }


    void insert(std::size_t offset, std::span<std::byte const> data)
    {
      if (!this->try_insert(offset, data))
        internal::handle_insert_exception();
    }
    void insert_back(std::span<std::byte const> data)
    {
      if (!this->try_insert_back(data))
        internal::handle_insert_exception();
    }
    void insert_front(std::span<std::byte const> data)
    {
      if (!this->try_insert_front(data))
        internal::handle_insert_exception();
    }

    template <ArithmeticOrByte T> void insert(std::size_t offset, T const& value, std::endian endian = std::endian::native)
    {
      if (!this->try_insert(offset, value, endian))
        internal::handle_insert_exception();
    }
    template <ArithmeticOrByte T> void insert_back(T const& value, std::endian endian = std::endian::native)
    {
      if (!this->try_insert_back(value, endian))
        internal::handle_insert_exception();
    }
    template <ArithmeticOrByte T> void insert_front(T const& value, std::endian endian = std::endian::native)
    {
      if (!this->try_insert_front(value, endian))
        internal::handle_insert_exception();
    }

    bool try_remove(std::size_t offset, std::size_t number_of_bytes = 1) noexcept
    {
      if (!this->range_check_read(offset, number_of_bytes))
        return false;

      this->m_pos_idx -= internal::data_shift_left(this->get_span(), offset, number_of_bytes);
      return true;
    }
    bool try_remove_back(std::size_t number_of_bytes = 1) noexcept { return this->try_remove(this->m_pos_idx - number_of_bytes, number_of_bytes); }
    bool try_remove_front(std::size_t number_of_bytes = 1) noexcept { return this->try_remove(0, number_of_bytes); }

    void remove(std::size_t offset, std::size_t number_of_bytes)
    {
      if (!this->try_remove(offset, number_of_bytes))
        return internal::handle_remove_exception();
    }
    void remove_back(std::size_t number_of_bytes)
    {
      if (!this->try_remove_back(number_of_bytes))
        return internal::handle_remove_exception();
    }
    void remove_front(std::size_t number_of_bytes)
    {
      if (!this->try_remove_front(number_of_bytes))
        return internal::handle_remove_exception();
    }

    template <ArithmeticOrByte T> bool try_remove(std::size_t offset) noexcept { return this->try_remove(offset, sizeof(T)); }
    template <ArithmeticOrByte T> bool try_remove_back() noexcept { return this->try_remove_back(sizeof(T)); }
    template <ArithmeticOrByte T> bool try_remove_front() noexcept { return this->try_remove_front(sizeof(T)); }

    template <ArithmeticOrByte T> void remove(std::size_t offset)
    {
      if (!this->try_remove<T>(offset))
        return internal::handle_remove_exception();
    }
    template <ArithmeticOrByte T> void remove_back()
    {
      if (!this->try_remove_back<T>())
        return internal::handle_remove_exception();
    }
    template <ArithmeticOrByte T> void remove_front()
    {
      if (!this->try_remove_front<T>())
        return internal::handle_remove_exception();
    }

    template <ArithmeticOrByte T> bool try_extract(std::size_t offset, T& value, std::endian endian = std::endian::native) noexcept
    {
      return this->try_read(offset, value, endian) && this->try_remove<T>(offset);
    }
    template <ArithmeticOrByte T> bool try_extract_back(T& value, std::endian endian = std::endian::native) noexcept
    {
      return this->try_read_back(value, endian) && this->try_remove_back<T>();
    }
    template <ArithmeticOrByte T> bool try_extract_front(T& value, std::endian endian = std::endian::native) noexcept
    {
      return this->try_read_front(value, endian) && this->try_remove_front<T>();
    }

    template <ArithmeticOrByte T> [[nodiscard]] T extract(std::size_t offset, std::endian endian = std::endian::native)
    {
      T ret;
      if (!this->try_extract(offset, ret, endian))
        internal::handle_read_exception();
      return ret;
    }
    template <ArithmeticOrByte T> [[nodiscard]] T extract_back(std::endian endian = std::endian::native)
    {
      T ret;
      if (!this->try_extract_back(ret, endian))
        internal::handle_read_exception();
      return ret;
    }
    template <ArithmeticOrByte T> [[nodiscard]] T extract_front(std::endian endian = std::endian::native)
    {
      T ret;
      if (!this->try_extract_front(ret, endian))
        internal::handle_read_exception();
      return ret;
    }

  private:
    constexpr bool range_check_read(std::size_t offset, std::size_t number_of_bytes_to_read) const noexcept
    {
      if ((std::numeric_limits<std::size_t>::max() - offset) < number_of_bytes_to_read)
        return false;

      if (this->m_pos_idx < (offset + number_of_bytes_to_read))
        return false;

      return true;
    }

    constexpr bool range_check_insert(std::size_t offset, std::size_t number_of_bytes_to_read) const noexcept
    {
      if (this->m_pos_idx < offset)
        return false;

      if ((std::numeric_limits<std::size_t>::max() - this->m_pos_idx) < number_of_bytes_to_read)
        return false;

      if (this->m_data.size() < (this->m_pos_idx + number_of_bytes_to_read))
        return false;

      return true;
    }

    std::span<std::byte> m_data;
    std::size_t          m_pos_idx = 0;
  };

  template <std::size_t N> class StaticBlob final: public MemoryBlob
  {
  public:
    StaticBlob()
        : MemoryBlob(this->m_mem)
    {
    }

    StaticBlob(StaticBlob const& other)
        : StaticBlob()
    {
      for (std::byte val : other.get_span())
        this->insert_back(val);
    }

    StaticBlob& operator=(StaticBlob const& other)
    {
      this->clear();
      for (std::byte val : other.get_span())
        this->insert_back(val);
      return *this;
    }

  private:
    std::array<std::byte, N> m_mem{};
  };

  template <ArithmeticOrByte T> MemoryBlob& operator<<(MemoryBlob& blob, T const& obj)
  {
    blob.insert_back(obj);
    return blob;
  }
  template <typename T> MemoryBlob& operator<<(MemoryBlob& blob, std::span<T const> obj_span)
  {
    for (auto const& obj : obj_span)
      blob << obj;
    return blob;
  }
  template <typename T, std::size_t N> MemoryBlob& operator<<(MemoryBlob& blob, std::array<T, N> const& obj_arr) { return blob << std::span<T const>(obj_arr); }

  template <ArithmeticOrByte T> MemoryBlob& operator>>(MemoryBlob& blob, T& obj)
  {
    obj = blob.extract_front<T>();
    return blob;
  }
  template <typename T> MemoryBlob& operator>>(MemoryBlob& blob, std::span<T> obj_span)
  {
    for (auto& obj : obj_span)
      blob >> obj;
    return blob;
  }
  template <typename T, std::size_t N> MemoryBlob& operator>>(MemoryBlob& blob, std::array<T, N>& obj_arr) { return blob >> std::span<T>(obj_arr); }

  template <ArithmeticOrByte T> ConstMemoryBlob& operator>>(ConstMemoryBlob& blob, T& obj)
  {
    obj = blob.extract_front<T>();
    return blob;
  }
  template <typename T> ConstMemoryBlob& operator>>(ConstMemoryBlob& blob, std::span<T> obj_span)
  {
    for (auto& obj : obj_span)
      blob >> obj;
    return blob;
  }
  template <typename T, std::size_t N> ConstMemoryBlob& operator>>(ConstMemoryBlob& blob, std::array<T, N>& obj_arr) { return blob >> std::span<T>(obj_arr); }
}    // namespace wlib::blob

#endif    // !WLIB_CRC_INTERFACE_HPP
