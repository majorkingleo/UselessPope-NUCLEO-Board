#include <wlib-StringSink.hpp>
#include <cstring>

namespace wlib
{
  namespace
  {
    class null_sink_t: public StringSink_Interface
    {
      bool operator()(char const*, uint32_t len ) override { return true; }
    };
  }    // namespace

  StringSink_Interface& StringSink_Interface::get_null_sink()
  {
    static null_sink_t obj;
    return obj;
  }

  bool StringSink_Interface::operator()(char const* c_str) { 
    return operator()(c_str, std::strlen(c_str)); 
  }

}    // namespace wlib
