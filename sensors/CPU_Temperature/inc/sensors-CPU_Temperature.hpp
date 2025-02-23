#pragma once
#ifndef SENSORS_CPU_TEMPERATURE_HPP_INCLUDED
#define SENSORS_CPU_TEMPERATURE_HPP_INCLUDED

#include <cmath>
#include <cstdint>

namespace sensors
{
  class cpu_temperature_sensor_t
  {
  public:
    cpu_temperature_sensor_t();

    auto operator()(uint16_t adc_value) const noexcept -> float { return this->m_scale * adc_value + this->m_offset; }

  private:
    float m_scale;
    float m_offset;
  };

}

#endif
