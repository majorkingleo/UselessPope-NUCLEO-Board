#pragma once

#include <exmath.hpp>
#include <bslib.hpp>

namespace BSP
{
  class Container : public Tools::static_vector<float,500>
  {
  public:
	  template <class functor>
	  void operator()( const functor & func, uint32_t number_of_burst_in_block ) {
		  for( unsigned i = 0; i < number_of_burst_in_block; ++i ) {
			  push_back( func(i) );
		  }
	  }

	  void operator()( float value ) {
		  push_back( value );
	  }
  };

  struct analog_values_adc1_t
  {

  };

  struct analog_values_adc2_t
  {

  };

  struct analog_values_adc3_t
  {
	exmath::statistics::BatchStatistics ext_temperature;
    exmath::statistics::BatchStatistics cpu_temperature;
    exmath::statistics::BatchStatistics ref_voltage;

    analog_values_adc3_t& operator+=(analog_values_adc3_t const& rhs) noexcept
    {
      this->ext_temperature += rhs.ext_temperature;
      this->cpu_temperature += rhs.cpu_temperature;
      this->ref_voltage += rhs.ref_voltage;
      return *this;
    }
  };

  bslib::publisher::Publisher_Interface<BSP::analog_values_adc1_t>& get_analog_value_adc1_publisher();
  bslib::publisher::Publisher_Interface<BSP::analog_values_adc2_t>& get_analog_value_adc2_publisher();
  bslib::publisher::Publisher_Interface<BSP::analog_values_adc3_t>& get_analog_value_adc3_publisher();
}    // namespace BSP
