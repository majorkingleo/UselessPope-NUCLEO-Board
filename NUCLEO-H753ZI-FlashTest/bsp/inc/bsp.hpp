#pragma once
#ifndef BSP_HPP
#define BSP_HPP

#include <bslib.hpp>
#include <cstdint>
#include <exmath.hpp>
#include <os.hpp>

namespace BSP
{
  struct analog_values_t
  {
    exmath::statistics::BatchStatistics ambient_temperature;
    exmath::statistics::BatchStatistics board_temperature;
    exmath::statistics::BatchStatistics cpu_temperature;

    exmath::statistics::BatchStatistics current;
    exmath::statistics::BatchStatistics voltage_stage_1;
    exmath::statistics::BatchStatistics voltage_stage_2;

    exmath::statistics::BatchStatistics air_pressure;
    exmath::statistics::BatchStatistics ref_voltage;

    analog_values_t& operator+=(analog_values_t const& rhs) noexcept
    {
      this->ambient_temperature += rhs.ambient_temperature;
      this->board_temperature += rhs.board_temperature;
      this->cpu_temperature += rhs.cpu_temperature;

      this->current += rhs.current;
      this->voltage_stage_1 += rhs.voltage_stage_1;
      this->voltage_stage_2 += rhs.voltage_stage_2;

      this->air_pressure += rhs.air_pressure;
      this->ref_voltage += rhs.ref_voltage;
      return *this;
    }
  };
}    // namespace BSP

// #include <canopen.hpp>
// #include <wlib.hpp>

namespace BSP
{
  inline bslib::publisher::LF_Publisher<void, 5>& get_fault_publisher()
  {
    static bslib::publisher::LF_Publisher<void, 5> obj;
    return obj;
  }

  inline bslib::publisher::LF_Publisher<void, 5>& get_cutoff_publisher()
  {
    static bslib::publisher::LF_Publisher<void, 5> obj;
    return obj;
  }
}    // namespace BSP

namespace BSP
{
  wlib::io::DigitalOutput_Interface& get_output_LED_green();
  // bslib::gpio::DigitalOutput_Interface& get_output_LED_yellow();
  // bslib::gpio::DigitalOutput_Interface& get_output_LED_red();

  wlib::CharPuplisher&        get_uart_input_debug();
  wlib::StringSink_Interface& get_uart_output_debug();


}    // namespace BSP

#endif
