#include <wlib-BLOB.hpp>

//
#include <stdexcept>

namespace wlib::blob
{
  void internal::handle_overwrite_exception() { throw std::out_of_range("not enouth room to insert object"); }
  void internal::handle_insert_exception() { throw std::out_of_range("not enouth room to insert object"); }
  void internal::handle_remove_exception() { throw std::out_of_range("not enouth bytes left"); }
  void internal::handle_read_exception() { throw std::out_of_range("not enouth bytes left to read"); }
  void internal::handle_position_exception() { throw std::out_of_range("not enouth bytes left to read"); }
}    // namespace wlib::blob
