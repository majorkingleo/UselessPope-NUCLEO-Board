#include <sensors-CPU_Temperature.hpp>

sensors::cpu_temperature_sensor_t::cpu_temperature_sensor_t()
{
  constexpr uint32_t rev_id_pos = 16;
  constexpr uint32_t rev_id_msk = 0xFFFFUL << rev_id_pos;

  uint32_t const id = *reinterpret_cast<uint32_t volatile*>(0x5C00'1000);
  uint16_t const v_cal_1 = *reinterpret_cast<uint16_t volatile*>(0x1FF1'E820);
  uint16_t const v_cal_2 = *reinterpret_cast<uint16_t volatile*>(0x1FF1'E840);

  float t1 = 30.0f;
  float v1 = static_cast<float>(v_cal_1);
  float t2 = 110.0f;
  float v2 = static_cast<float>(v_cal_2);

  uint32_t const rev = (id & rev_id_msk) >> rev_id_pos;
  switch (rev)
  {
  case 0x1003:
    break;
  case 0x2003:
    t2 = 130.0f;
    break;
  default:
    throw "ahhh";
  }

  float const s  = (t2 - t1) / (v2 - v1);
  this->m_scale  = s;
  this->m_offset = t1 - s * v1;
}
