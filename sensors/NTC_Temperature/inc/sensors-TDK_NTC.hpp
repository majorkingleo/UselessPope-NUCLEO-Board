#pragma once
#ifndef SENSORS_TDK_NTC_HPP_INCLUDED
#define SENSORS_TDK_NTC_HPP_INCLUDED

#include <cmath>

namespace sensors
{
  class TDK_NTC
  {
    static constexpr std::size_t order = 5;
    static constexpr std::size_t number_of_parameters = order + 1;

  public:
    static constexpr float TDK_1038[6] = {
      -9.222592802E-04f, +1.382518376E-02f, -1.546954941E-01f, +1.916382790E+00f, -2.555781901E+01f, +2.500154305E+01f,
    };
    static constexpr float TDK_2014[6] = {
      -2.556432289E-04f, +4.489008171E-03f, -6.354673041E-02f, +1.094767221E+00f, -2.040224554E+01f, +2.500498732E+01f,
    };
    static constexpr float TDK_2901[6] = {
      +4.435922049E-04f, +7.819148210E-03f, -1.413246088E-01f, +1.651983452E+00f, -2.436879535E+01f, +2.502572628E+01f,
    };
    static constexpr float TDK_8016[6] = {
      -5.818855425E-04f, +8.898011976E-03f, -1.097202100E-01f, +1.526588433E+00f, -2.279584681E+01f, +2.500329858E+01f,
    };
    static constexpr float TDK_8018[6] = {
      -5.294985722E-04f, +7.940450304E-03f, -1.015753028E-01f, +1.484490565E+00f, -2.319027998E+01f, +2.500556707E+01f,
    };
    static constexpr float TDK_8500[6] = {
      -8.534152484E-04f, +1.198626043E-02f, -1.360667142E-01f, +1.805070060E+00f, -2.500468741E+01f, +2.500576419E+01f,
    };
    static constexpr float TDK_8501[6] = {
      -8.952351355E-04f, +1.357741942E-02f, -1.541901023E-01f, +1.910485810E+00f, -2.567535833E+01f, +2.500258148E+01f,
    };
    static constexpr float TDK_8502[6] = {
      -5.233580101E-04f, +8.129537586E-03f, -1.053382955E-01f, +1.513670523E+00f, -2.278560326E+01f, +2.500548310E+01f,
    };
    static constexpr float TDK_8507[6] = {
      -2.783328124E-04f, +4.828940299E-03f, -7.004689867E-02f, +1.154083701E+00f, -2.055123124E+01f, +2.500480825E+01f,
    };
    static constexpr float TDK_8509[6] = {
      -1.025139982E-03f, +1.335529931E-02f, -1.551711304E-01f, +1.927570314E+00f, -2.666849367E+01f, +2.500455686E+01f,
    };

    TDK_NTC(float const (&parameter)[number_of_parameters], float r_base)
      : m_para(parameter)
      , m_r_base(r_base)
    {
    }

    auto operator()(float resistance) const noexcept -> float
    {
      float const v = std::log(resistance / this->m_r_base);

      float tmp = this->m_para[0];
      for (std::size_t i = 1; i < number_of_parameters; i++)
      {
        tmp *= v;
        tmp += this->m_para[i];
      }
      return tmp;
    }

  private:
    float const (&m_para)[number_of_parameters];
    float const m_r_base;
  };
}

#endif
