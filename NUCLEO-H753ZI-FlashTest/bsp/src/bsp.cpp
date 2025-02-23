#include <atomic>
#include <bsp.hpp>
#include <bsp_uart_usb.hpp>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exmath.hpp>
#include <stm32h753xx.h>
#include <system_stm32h7xx.h>
#include <uC.hpp>

namespace BSP
{
  uC::DMA_Buffer_Allocator_Interface& get_dma_buffer_allocator()
  {
    using T = uC::DMA_Buffer_Allocator;
    static std::byte __attribute__((section(".reserved_for_DMA_BUFFER"))) buffer[1024 * 150]{};
    static T                                                              obj(buffer);
    return obj;
  }
}    // namespace BSP

namespace
{
  auto& get_uart_debug()
  {
    using T = uC::UART__TX_DMA__RX_IRQ;
    static T obj(T::UART_1__TX_A_09__RX__A_10, uC::DMA_Streams::DMA_1_Stream_0, 1024 * 5);
    return obj;
  }
}    // namespace

namespace BSP
{
  void init() {
	  init_uart_usb();
  }
  void enter_save_state() {}

  wlib::io::DigitalOutput_Interface& get_output_LED_green()
  {
    using T = uC::Output_Pin;
    static T pin{ uC::GPIOs::B_00, T::Speed::High, T::Output_Mode::Push_Pull, T::Pull_Mode::No_Pull, false };
    return pin;
  }

  wlib::CharPuplisher&        get_uart_input_debug() { return get_uart_debug().get_input_publisher(); }
  wlib::StringSink_Interface& get_uart_output_debug()
  {
    return get_usb_uart_output_debug();
	  //return get_uart_debug();
  }

}    // namespace BSP

