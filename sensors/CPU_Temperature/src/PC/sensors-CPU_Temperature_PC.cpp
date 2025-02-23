#include <sensors-CPU_Temperature.hpp>

sensors::cpu_temperature_sensor_t::cpu_temperature_sensor_t()
{
  this->m_scale  = 0;
  this->m_offset = 30.0f;
}
