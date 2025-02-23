#include "task_status_led.h"

#include <CpputilsDebug.h>
#include <bsp.hpp>

void task_status_led()
{
  wlib::io::DigitalOutput_Interface& pin = BSP::get_output_LED_green();

  while (os::this_thread::keep_running())
  {
    os::this_thread::sleep_for(std::chrono::milliseconds(500));
    pin.toggle();
  }

  CPPDEBUG("done");
}
