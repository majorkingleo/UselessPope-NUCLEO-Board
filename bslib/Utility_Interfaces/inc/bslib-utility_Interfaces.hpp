#pragma once
#ifndef BSLIB_UTILITY_INTERFACES_HPP_INCLUDED
#define BSLIB_UTILITY_INTERFACES_HPP_INCLUDED

#include <cstdint>
#include <span>

namespace bslib
{
  class PWM_Interface
  {
  public:
    virtual ~PWM_Interface() noexcept       = default;
    virtual float set_ratio(float) noexcept = 0;
  };

  class H_Bridge_Interface
  {
  public:
    virtual ~H_Bridge_Interface() noexcept  = default;
    virtual float set_ratio(float) noexcept = 0;
  };

  class FRQ_Measurement_Interface
  {
  protected:
    class result_t
    {
    public:
      using value_type = double;

      explicit result_t(value_type frequency_value)
          : m_value{ frequency_value }
      {
      }

      constexpr auto get_frequency() const noexcept { return this->m_value; }

    private:
      value_type m_value;
    };

  public:
    virtual ~FRQ_Measurement_Interface() = default;

    virtual result_t get() const noexcept = 0;
  };

  class FRQ_Generator_Interface
  {
  public:
	class result_t
	{
	public:
	  using value_type = double;

	  explicit result_t(value_type frequency_value)
		  : m_value{ frequency_value }
	  {
	  }

	  constexpr auto get_frequency() const noexcept { return this->m_value; }

	private:
	  value_type m_value;
	};

  public:
    virtual ~FRQ_Generator_Interface() = default;

    /**
     * Set the requested frequency.
     *
     * Returns the approximation frequency that was really set.
     */
    virtual result_t set_frequency( result_t::value_type freq ) noexcept = 0;
  };


  class Double_FRQ_Measurement_Interface
  {
  protected:
    class result_t
    {
    public:
      using value_type = double;

      explicit result_t(value_type frequency_value_1, value_type frequency_value_2)
          : m_value{ 0.5 * (frequency_value_1 + frequency_value_2) }
          , m_value_1{ frequency_value_1 }
          , m_value_2{ frequency_value_2 }
      {
      }

      constexpr auto get_frequency() const noexcept { return this->m_value; }
      constexpr auto get_frequency_1() const noexcept { return this->m_value_1; }
      constexpr auto get_frequency_2() const noexcept { return this->m_value_2; }

    private:
      value_type m_value;
      value_type m_value_1;
      value_type m_value_2;
    };

  public:
    virtual ~Double_FRQ_Measurement_Interface() = default;

    virtual result_t get() const noexcept = 0;
  };

}    // namespace bslib

#endif    // BSLIB_UTILITY_INTERFACES_HPP_INCLUDED
