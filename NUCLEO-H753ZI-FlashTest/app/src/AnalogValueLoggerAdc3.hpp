#pragma once

#include <bsp_adc.hpp>
#include <wlib.hpp>
#include <os.hpp>
#include <AtomicPointer.hpp>
#include <LastStateInfo.hpp>

namespace app {

struct NTCState
{
	float temp_C = 0;
	float compare_precision = 0.01;

	bool operator==( const NTCState & other ) const {
		float diff = std::fabs( temp_C - other.temp_C );
		return diff < compare_precision;
	}
};

using last_state_ntc_t = LastStateInfo<NTCState>;
extern AtomicPointer<last_state_ntc_t> LAST_STATE_NTC;


class analog_value_logger_adc3
{
public:
  using analog_values_t = BSP::analog_values_adc3_t;

  analog_value_logger_adc3(wlib::publisher::Publisher_Interface<analog_values_t>& analog_value_pup,
		  	  	  	  std::span<os::internal::stack_t>              stack_span,
                      wlib::StringSink_Interface&  sink = wlib::StringSink_Interface::get_null_sink());

  auto get_analog_values() const -> analog_values_t;

  auto print(wlib::StringSink_Interface& sink) const -> void;

private:
  void new_analog_value(BSP::analog_values_adc3_t const& values);

  void process();

  using this_t                                                                               = analog_value_logger_adc3;
  mutable os::mutex                                                           m_mtex         = {};
  wlib::publisher::Memberfunction_CallbackSubscriber<this_t, analog_values_t> m_sub          = { *this, &this_t::new_analog_value };
  os::Static_MemberfunctionCallbackTask<this_t, 0>                            m_worker;
  bslib::container::SPSC<analog_values_t, 2>                                  m_input_buffer = {};
  wlib::container::circular_buffer_t<analog_values_t, 10>                     m_circ_buffer  = {};
  wlib::StringSink_Interface&                                                 m_sink;
};


} // namespace app
