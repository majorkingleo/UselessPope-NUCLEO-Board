#pragma once

#include <bslib.hpp>
#include <optional>

namespace BSP
{
  void init_uart_usb();

  int usb_uart_put_char(int ch);

  wlib::StringSink_Interface& get_usb_uart_output_debug();

  std::optional<char> usb_uart_get_char();
}    // namespace BSP

