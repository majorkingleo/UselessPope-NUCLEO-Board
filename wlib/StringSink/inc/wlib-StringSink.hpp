#pragma once
#ifndef WLIB_STRINGSINK_HPP_INCLUDED
#define WLIB_STRINGSINK_HPP_INCLUDED

#include <cstdint>
#include <wlib-Publisher.hpp>

namespace wlib
{
  using CharPuplisher = wlib::publisher::Publisher_Interface<char>;

  class StringSink_Interface
  {
  public:
    static StringSink_Interface& get_null_sink();
    virtual ~StringSink_Interface() = default;

    virtual bool operator()(char const* c_str);
    virtual bool operator()(char const* c_str, uint32_t len) = 0;
  };
}    // namespace wlib

#endif
