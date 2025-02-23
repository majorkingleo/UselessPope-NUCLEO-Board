#include "AnalogValueLogger.hpp"

namespace app
{

  void analog_value_logger::process()
  {
    while (os::this_thread::keep_running())
    {
      bool const     timeout = os::this_thread::try_wait_for_notify_for(std::chrono::milliseconds(300)) == 0;
      os::lock_guard l{ this->m_mtex };
      if (!timeout)
      {
        std::optional<analog_values_t> tmp = this->m_input_buffer.pop_front();
        while (tmp.has_value())
        {
          this->m_circ_buffer.push(tmp.value());
          tmp = this->m_input_buffer.pop_front();
        }
      }
      else
      {
        this->m_circ_buffer.clear();
      }
    }
  }

  void analog_value_logger::new_analog_value(BSP::analog_values_t const& values)
  {
    this->m_input_buffer.push_back(values);
    this->m_worker.notify();
  }

  auto analog_value_logger::print(wlib::StringSink_Interface& sink) const -> void
  {
    auto tmp = this->get_analog_values();
    char buf[2048]{};
    snprintf(buf, 2048,
             "      NTC_max: %10.5f,       NTC_min: %10.5f,       NTC_mean: %10.5f,       NTC_var: %10.9E\n"
             "NTC-BOARD_max: %10.5f, NTC-BOARD_min: %10.5f, NTC-BOARD_mean: %10.5f, NTC-BOARD_var: %10.9E\n"
             "      CPU_max: %10.5f,       CPU_min: %10.5f,       CPU_mean: %10.5f,       CPU_var: %10.9E\n"
             "\n"
             "        i_max: %10.5f,         i_min: %10.5f,         i_mean: %10.5f,         i_var: %10.9E\n"
             "       u1_max: %10.5f,        u1_min: %10.5f,        u1_mean: %10.5f,        u1_var: %10.9E\n"
             "       u2_max: %10.5f,        u2_min: %10.5f,        u2_mean: %10.5f,        u2_var: %10.9E\n"
             "\n"
             "      AIR_max: %10.5f,       AIR_min: %10.5f,       AIR_mean: %10.5f,       AIR_var: %10.9E\n"
             "     VREF_max: %10.5f,      VREF_min: %10.5f,      VREF_mean: %10.5f,      VREF_var: %10.9E\n",
             tmp.ambient_temperature.get_max(), tmp.ambient_temperature.get_min(), tmp.ambient_temperature.get_mean(),
             tmp.ambient_temperature.get_variance(),                                                                                                      //
             tmp.board_temperature.get_max(), tmp.board_temperature.get_min(), tmp.board_temperature.get_mean(), tmp.board_temperature.get_variance(),    //
             tmp.cpu_temperature.get_max(), tmp.cpu_temperature.get_min(), tmp.cpu_temperature.get_mean(), tmp.cpu_temperature.get_variance(),            //
             tmp.current.get_max(), tmp.current.get_min(), tmp.current.get_mean(), tmp.current.get_variance(),                                            //
             tmp.voltage_stage_1.get_max(), tmp.voltage_stage_1.get_min(), tmp.voltage_stage_1.get_mean(), tmp.voltage_stage_1.get_variance(),            //
             tmp.voltage_stage_2.get_max(), tmp.voltage_stage_2.get_min(), tmp.voltage_stage_2.get_mean(), tmp.voltage_stage_2.get_variance(),            //
             tmp.air_pressure.get_max(), tmp.air_pressure.get_min(), tmp.air_pressure.get_mean(), tmp.air_pressure.get_variance(),                        //
             tmp.ref_voltage.get_max(), tmp.ref_voltage.get_min(), tmp.ref_voltage.get_mean(), tmp.ref_voltage.get_variance()                             //
    );
    sink(buf);
  }

  auto analog_value_logger::get_analog_values() const -> analog_values_t
  {
    os::lock_guard  l{ this->m_mtex };
    analog_values_t ret;
    for (analog_values_t const& el : this->m_circ_buffer)
    {
      ret += el;
    }
    return ret;
  }

  analog_value_logger::analog_value_logger(wlib::publisher::Publisher_Interface<analog_values_t>& analog_value_pup, wlib::StringSink_Interface& sink)
      : m_sink(sink)
  {
    this->m_sink("analog_value_logger\n");
    this->m_sub.subscribe(analog_value_pup);
    this->m_worker.start();
  };

}    // namespace app
