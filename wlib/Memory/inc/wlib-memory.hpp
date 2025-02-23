#pragma once
#ifndef WLIB_MEMORY_INTERFACE_HPP_INCLUDED
#define WLIB_MEMORY_INTERFACE_HPP_INCLUDED

#include <cstdint>
#include <span>

namespace wlib
{
  namespace memory
  {
    class Non_Volatile_Memory_Interface
    {
    public:
      virtual ~Non_Volatile_Memory_Interface() = default;

      virtual void write(std::size_t add, std::span<std::byte const> data) = 0;
      virtual void flush()                                                 = 0;
      virtual void read(std::size_t add, std::span<std::byte> data)        = 0;
    };
  }    // namespace memory
}    // namespace wlib

#endif    // WLIB_MEMORY_INTERFACE_HPP_INCLUDED
