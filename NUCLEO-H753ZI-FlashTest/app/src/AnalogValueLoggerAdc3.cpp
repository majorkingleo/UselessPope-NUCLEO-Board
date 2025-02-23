#include "AnalogValueLoggerAdc3.hpp"
#include <static_format.h>

using namespace Tools;

namespace app {

static last_state_ntc_t last_state_ntc;
AtomicPointer<last_state_ntc_t> LAST_STATE_NTC = &last_state_ntc;

void analog_value_logger_adc3::process()
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
			const float temperature = tmp->ext_temperature.get_mean();
			LAST_STATE_NTC->set( NTCState( temperature ) );

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

void analog_value_logger_adc3::new_analog_value(BSP::analog_values_adc3_t const& values)
{
	this->m_input_buffer.push_back(values);
	this->m_worker.notify();
}


auto analog_value_logger_adc3::print(wlib::StringSink_Interface& sink) const -> void
{
    auto tmp = this->get_analog_values();

    auto buf = static_format<500>(
            "       NTC_max: %10.5f,        NTC_min: %10.5f,        NTC_mean: %10.5f,        NTC_var: %10.9E %d samples\n"
            "       CPU_max: %10.5f,        CPU_min: %10.5f,        CPU_mean: %10.5f,        CPU_var: %10.9E\n"
    		"      VREF_max: %10.5f,       VREF_min: %10.5f,       VREF_mean: %10.5f,       VREF_var: %10.9E\n",
             tmp.ext_temperature.get_max(), tmp.ext_temperature.get_min(), tmp.ext_temperature.get_mean(),
             tmp.ext_temperature.get_variance(), tmp.ext_temperature.get_number_of_values(),                                                        //
             tmp.cpu_temperature.get_max(), tmp.cpu_temperature.get_min(), tmp.cpu_temperature.get_mean(), tmp.cpu_temperature.get_variance(),            //
             tmp.ref_voltage.get_max(), tmp.ref_voltage.get_min(), tmp.ref_voltage.get_mean(), tmp.ref_voltage.get_variance()                             //
    );
    sink(buf.c_str());
}

auto analog_value_logger_adc3::get_analog_values() const -> analog_values_t
{
  os::lock_guard  l{ this->m_mtex };
  analog_values_t ret;
  for (analog_values_t const& el : this->m_circ_buffer)
  {
    ret += el;
  }
  return ret;
}

analog_value_logger_adc3::analog_value_logger_adc3(wlib::publisher::Publisher_Interface<analog_values_t>& analog_value_pup,
										 std::span<os::internal::stack_t>              stack_span,
                    					 wlib::StringSink_Interface&                   sink )
    : m_worker{ *this, &this_t::process, stack_span, "AnalogAdc3" },
	  m_sink(sink)
{
  this->m_sub.subscribe(analog_value_pup);
  this->m_worker.start();
};

} // namespace app
