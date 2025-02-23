#include <wlib-Publisher.hpp>

//
#include <stdexcept>
namespace wlib::publisher::error
{
  void handle_subscription_exception() { throw std::out_of_range("unable to subscribe to publisher"); }
}    // namespace wlib::blob
