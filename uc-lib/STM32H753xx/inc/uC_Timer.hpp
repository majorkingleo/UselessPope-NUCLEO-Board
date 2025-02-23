#pragma once
#ifndef UC_TIMER_HPP
#define UC_TIMER_HPP

#include <cmath>
#include <uC_GPIO.hpp>
#include <uC_HW_Handles.hpp>
#include <wlib.hpp>
#include <bslib.hpp>

namespace uC
{
  namespace Internal
  {
    static constexpr std::tuple<uint32_t, uint32_t, uint32_t> calculate_psc_arr(uint64_t clk, uint64_t trg, uint64_t tim_max)
    {
      uint64_t const cnt       = clk / trg;
      uint64_t const psc_val   = (cnt / (tim_max + 1)) + 1;
      uint64_t const arr_val_1 = (cnt / psc_val);
      uint64_t const arr_val_2 = arr_val_1 + 1;

      uint64_t const real_1 = clk / (psc_val * arr_val_1);
      uint64_t const real_2 = clk / (psc_val * arr_val_2);

      uint64_t const arr_val = real_1 - trg < trg - real_2 ? arr_val_1 : arr_val_2;

      uint64_t const real = clk / (psc_val * arr_val);

      return { psc_val, arr_val, real };
    }
  }    // namespace Internal

  class Single_PWM: public bslib::PWM_Interface
  {
  public:
    struct af_pin
    {
      GPIOs::HW_Unit const pin;
      uint32_t const       af;
    };
    struct config_t
    {
      TIMERs::HW_Unit const timer_unit;
      uint32_t const        ch;
      af_pin const          pin;
    };

    using PIN = Alternative_Funktion_Pin;

  public:
    static constexpr config_t TIMER_2__CH_3__PB10 = { uC::TIMERs::TIMER_2, 2, { uC::GPIOs::B_10, 1 } };
    static constexpr config_t TIMER_3__CH_1__PC06 = { uC::TIMERs::TIMER_3, 0, { uC::GPIOs::C_06, 2 } };

    static constexpr config_t TIMER_4__CH_1__PD12 = { uC::TIMERs::TIMER_4, 0, { uC::GPIOs::D_12, 2 } };
    static constexpr config_t TIMER_4__CH_2__PB07 = { uC::TIMERs::TIMER_4, 1, { uC::GPIOs::B_07, 2 } };
    static constexpr config_t TIMER_4__CH_3__PB08 = { uC::TIMERs::TIMER_4, 2, { uC::GPIOs::B_08, 2 } };

    static constexpr config_t TIMER_5__CH_2__PH11 = { uC::TIMERs::TIMER_5, 1, { uC::GPIOs::H_11, 2 } };

    static constexpr config_t TIMER_14__CH_1__PF09 = { uC::TIMERs::TIMER_14, 0, { uC::GPIOs::F_09, 9 } };

    Single_PWM(config_t const cfg, uint32_t pwm_frq)
        : m_tim{ cfg.timer_unit }
        , m_pin{ cfg.pin.pin, PIN::Speed::High, PIN::Output_Mode::Push_Pull, PIN::Pull_Mode::No_Pull, cfg.pin.af }
    {
      auto [psc, arr, pwm_f_real] = Internal::calculate_psc_arr(this->m_tim.get_clk(), pwm_frq, 0xFFFF);

      if (pwm_f_real != pwm_frq)
        uC::Errors::uC_config_error("PWM frequency not possible");

      TIM_TypeDef& tim_base = this->m_tim.get_base();

      uint32_t ccmr1 = 0;
      uint32_t ccmr2 = 0;
      uint32_t ccer  = 0;

      switch (cfg.ch)
      {
      case 0:
        ccmr1 |= ((0b110 << TIM_CCMR1_OC1M_Pos) & TIM_CCMR1_OC1M_Msk) | ((1 << TIM_CCMR1_OC1PE_Pos) & TIM_CCMR1_OC1PE_Msk);
        ccer |= ((1 << TIM_CCER_CC1E_Pos) & TIM_CCER_CC1E_Msk);
        this->m_cc_reg = &tim_base.CCR1;
        break;

      case 1:
        ccmr1 |= ((0b110 << TIM_CCMR1_OC2M_Pos) & TIM_CCMR1_OC2M_Msk) | ((1 << TIM_CCMR1_OC2PE_Pos) & TIM_CCMR1_OC2PE_Msk);
        ccer |= ((1 << TIM_CCER_CC2E_Pos) & TIM_CCER_CC2E_Msk);
        this->m_cc_reg = &tim_base.CCR2;
        break;

      case 2:
        ccmr2 |= ((0b110 << TIM_CCMR2_OC3M_Pos) & TIM_CCMR2_OC3M_Msk) | ((1 << TIM_CCMR2_OC3PE_Pos) & TIM_CCMR2_OC3PE_Msk);
        ccer |= ((1 << TIM_CCER_CC3E_Pos) & TIM_CCER_CC3E_Msk);
        this->m_cc_reg = &tim_base.CCR3;
        break;

      case 3:
        ccmr2 |= ((0b110 << TIM_CCMR2_OC4M_Pos) & TIM_CCMR2_OC4M_Msk) | ((1 << TIM_CCMR2_OC4PE_Pos) & TIM_CCMR2_OC4PE_Msk);
        ccer |= ((1 << TIM_CCER_CC4E_Pos) & TIM_CCER_CC4E_Msk);
        this->m_cc_reg = &tim_base.CCR4;
        break;

      default:
        uC::Errors::not_implemented();
        break;
      }

      // clang-format off
    tim_base.CNT  = 0;
    tim_base.PSC  = psc - 1;
    tim_base.ARR  = arr - 1;
    tim_base.CCR1 = 0;
    tim_base.CCR2 = 0;
    tim_base.CCR3 = 0;
    tim_base.CCR4 = 0;

    tim_base.CR2   = ((0b010 << TIM_CR2_MMS_Pos) & TIM_CR2_MMS_Msk);
    tim_base.CCMR1 = ccmr1;
    tim_base.CCMR2 = ccmr2;
    tim_base.CCER  = ccer;

    this->m_max_val = arr - 1;

    tim_base.CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
      // clang-format on
    }

    float set_ratio(float value) noexcept override
    {
      if (!std::isfinite(value))
        value = 0.0f;
      else if (value > 1.0f)
        value = 1.0f;
      else if (value < 0.0f)
        value = 0.0f;

      float          wish_value = this->m_max_val * value;
      uint32_t const cc_value   = std::round(wish_value);

      *this->m_cc_reg = cc_value;

      return static_cast<float>(cc_value) / static_cast<float>(this->m_max_val);
    }

  private:
    uC::HANDLEs::BASIC_TIMER_Handle_t const m_tim;
    PIN const                               m_pin;
    uint32_t                                m_max_val = 0;
    uint32_t volatile*                      m_cc_reg  = nullptr;
  };

  template<std::size_t MAX_CHANNELS=6>
  class MultiChannel_PWM
  {
  public:
    using af_pin = Single_PWM::af_pin;

    struct config_t
    {
      uint32_t const        ch;
      af_pin const          pin;
    };

    using PIN = Alternative_Funktion_Pin;

  public:
    MultiChannel_PWM( TIMERs::HW_Unit const timer_unit, const std::initializer_list<config_t> & cfg_list, uint32_t pwm_frq)
        : m_tim{ timer_unit }
    {
    	if( cfg_list.size() > MAX_CHANNELS ) {
    		throw std::out_of_range("too many channels");
    	}

		auto [psc, arr, pwm_f_real] = Internal::calculate_psc_arr(this->m_tim.get_clk(), pwm_frq, 0xFFFF);

		if (pwm_f_real != pwm_frq) {
		  uC::Errors::uC_config_error("PWM frequency not possible");
		}

		TIM_TypeDef& tim_base = this->m_tim.get_base();

		uint32_t ccmr1 = 0;
		uint32_t ccmr2 = 0;
		uint32_t ccer  = 0;

		for( const config_t & cfg : cfg_list ) {
			PIN pin{ cfg.pin.pin, PIN::Speed::High, PIN::Output_Mode::Push_Pull, PIN::Pull_Mode::No_Pull, cfg.pin.af };

			switch (cfg.ch)
			{
			case 0:
				ccmr1 |= ((0b110 << TIM_CCMR1_OC1M_Pos) & TIM_CCMR1_OC1M_Msk) | ((1 << TIM_CCMR1_OC1PE_Pos) & TIM_CCMR1_OC1PE_Msk);
				ccer |= ((1 << TIM_CCER_CC1E_Pos) & TIM_CCER_CC1E_Msk);
				this->m_cc_reg[0] = &tim_base.CCR1;
				break;

			case 1:
				ccmr1 |= ((0b110 << TIM_CCMR1_OC2M_Pos) & TIM_CCMR1_OC2M_Msk) | ((1 << TIM_CCMR1_OC2PE_Pos) & TIM_CCMR1_OC2PE_Msk);
				ccer |= ((1 << TIM_CCER_CC2E_Pos) & TIM_CCER_CC2E_Msk);
				this->m_cc_reg[1] = &tim_base.CCR2;
				break;

			case 2:
				ccmr2 |= ((0b110 << TIM_CCMR2_OC3M_Pos) & TIM_CCMR2_OC3M_Msk) | ((1 << TIM_CCMR2_OC3PE_Pos) & TIM_CCMR2_OC3PE_Msk);
				ccer |= ((1 << TIM_CCER_CC3E_Pos) & TIM_CCER_CC3E_Msk);
				this->m_cc_reg[2] = &tim_base.CCR3;
				break;

			case 3:
				ccmr2 |= ((0b110 << TIM_CCMR2_OC4M_Pos) & TIM_CCMR2_OC4M_Msk) | ((1 << TIM_CCMR2_OC4PE_Pos) & TIM_CCMR2_OC4PE_Msk);
				ccer |= ((1 << TIM_CCER_CC4E_Pos) & TIM_CCER_CC4E_Msk);
				this->m_cc_reg[3] = &tim_base.CCR4;
				break;

				// todo channel 5 and 6

			default:
				uC::Errors::not_implemented();
				break;
			}
		}

		// clang-format off
		tim_base.CNT  = 0;
		tim_base.PSC  = psc - 1;
		tim_base.ARR  = arr - 1;
		tim_base.CCR1 = 0;
		tim_base.CCR2 = 0;
		tim_base.CCR3 = 0;
		tim_base.CCR4 = 0;

		tim_base.CR2   = ((0b010 << TIM_CR2_MMS_Pos) & TIM_CR2_MMS_Msk);
		tim_base.CCMR1 = ccmr1;
		tim_base.CCMR2 = ccmr2;
		tim_base.CCER  = ccer;

		this->m_max_val = arr - 1;

		tim_base.CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
		// clang-format on
    }

    float set_ratio( uint32_t channel, float value ) noexcept
    {
      if (!std::isfinite(value))
        value = 0.0f;
      else if (value > 1.0f)
        value = 1.0f;
      else if (value < 0.0f)
        value = 0.0f;

      float          wish_value = this->m_max_val * value;
      uint32_t const cc_value   = std::round(wish_value);

      *m_cc_reg.at(channel) = cc_value;

      return static_cast<float>(cc_value) / static_cast<float>(this->m_max_val);
    }

  private:
    uC::HANDLEs::BASIC_TIMER_Handle_t const 		m_tim;
    uint32_t                                		m_max_val = 0;
    std::array<uint32_t volatile*,MAX_CHANNELS>     m_cc_reg{};
  };

  /**
   * exposes the bslib::PWM_Interface
   *
   * eg:
   * static const MultiChannel_PWM_Wrapper<3>::config_t LIQ1_PWM { 1, { uC::GPIOs::B_07, 2 } };
   * static const MultiChannel_PWM_Wrapper<3>::config_t LIQ2_PWM { 2, { uC::GPIOs::B_08, 2 } };
   * static const MultiChannel_PWM_Wrapper<3>::config_t LIQ3_PWM { 0, { uC::GPIOs::D_12, 2 } };
   *
   * static MultiChannel_PWM_Wrapper<3> wrapper( uC::TIMERs::TIMER_4, { LIQ1_PWM, LIQ2_PWM, LIQ3_PWM }, 20'000 );
   *
   * bslib::PWM_Interface& BSP::get_light_barrier_unit1_led() {
   *	return wrapper.get_pwm_interface(LIQ1_PWM.ch);
   * }
   *
   */
  template<std::size_t MAX_CHANNELS=6>
  class MultiChannel_PWM_Wrapper
  {
  	public:
  		using config_t = MultiChannel_PWM<MAX_CHANNELS>::config_t;

  	private:
  		class Wrapper : public bslib::PWM_Interface
  		{
  		private:
  			MultiChannel_PWM<MAX_CHANNELS> & parent;
  			uint32_t		   channel;

  		public:
  			Wrapper( MultiChannel_PWM<MAX_CHANNELS> & pwm_, uint32_t channel_ )
  			: parent( pwm_ ),
  			  channel( channel_ )
  			{}

  			float set_ratio( float value ) noexcept override
  			{
  				return parent.set_ratio( channel, value );
  			}
  		};

  	private:
  		MultiChannel_PWM<MAX_CHANNELS> pwm;
  		std::array<std::optional<Wrapper>,MAX_CHANNELS> wrappers;

  	public:
  		MultiChannel_PWM_Wrapper( TIMERs::HW_Unit const timer_unit, const std::initializer_list<config_t> & cfg_list, uint32_t pwm_frq )
  		: pwm( timer_unit, cfg_list, pwm_frq )
  		{
  			for( const config_t & cfg : cfg_list ) {

  				if( wrappers.at(cfg.ch).has_value() ) {
  					throw std::out_of_range("channel already in use");
  				}

  				wrappers.at(cfg.ch).emplace( pwm, cfg.ch );
  			}
  		}

  		bslib::PWM_Interface & get_pwm_interface( uint32_t channel ) {
  			return *wrappers.at(channel);
  		}
  	};


  class Trigger_Timer
  {
  public:
    Trigger_Timer(uC::TIMERs::HW_Unit const tim, uint32_t trigger_frq)
        : m_tim(tim)
    {
      auto [psc, arr, pwm_frq_real] = Internal::calculate_psc_arr(this->m_tim.get_clk(), trigger_frq, 0xFFFF);

      if (pwm_frq_real != trigger_frq)
        uC::Errors::uC_config_error("Trigger frequency not possible");
      TIM_TypeDef& tim_base = this->m_tim.get_base();

      tim_base.CNT = 0;
      tim_base.PSC = psc - 1;
      tim_base.ARR = arr - 1;
      tim_base.CR2 = ((0b010 << TIM_CR2_MMS_Pos) & TIM_CR2_MMS_Msk);
      tim_base.CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
    }

  private:
    uC::HANDLEs::BASIC_TIMER_Handle_t const m_tim;
  };
}    // namespace uC

#endif
