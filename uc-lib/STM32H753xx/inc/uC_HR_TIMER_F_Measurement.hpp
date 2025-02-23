#pragma once
#ifndef UC_HR_TIMER_HPP_INCLUDED
#define UC_HR_TIMER_HPP_INCLUDED

#include <bslib.hpp>
#include <cmath>
#include <os.hpp>
#include <uC_GPIO.hpp>
#include <uC_HW_Handles.hpp>

namespace uC
{
  class HR_TIMER_DT_Event_Measurement
  {
    using Notifyable_Interface = wlib::publisher::Publisher_Interface<double>::Notifyable_Interface&;
    struct input_pin_t
    {
      uC::GPIOs::HW_Unit const pin;
      uint32_t const           af;
      uint32_t const           ext_event_msk;
    };
    struct config_t
    {
      uC::HRTIMERs::HW_Unit const hr_timer_unit;
      input_pin_t const           input_pin;
    };
    using PIN = uC::Alternative_Funktion_Pin;

  public:
    static constexpr config_t HRTIM_1__PB_05 = { uC::HRTIMERs::HR_TIMER_1, { uC::GPIOs::B_05, 3, (1 << 8) } };
    static constexpr config_t HRTIM_1__PB_06 = { uC::HRTIMERs::HR_TIMER_1, { uC::GPIOs::B_06, 3, (1 << 9) } };
    static constexpr config_t HRTIM_1__PB_07 = { uC::HRTIMERs::HR_TIMER_1, { uC::GPIOs::B_07, 3, (1 << 10) } };

    HR_TIMER_DT_Event_Measurement(config_t const& config, Notifyable_Interface& pub)
        : m_hw{ config.hr_timer_unit }
        , m_input_pin{ config.input_pin.pin, PIN::Speed::High, PIN::Output_Mode::Push_Pull, PIN::Pull_Mode::No_Pull, config.input_pin.af }
        , m_pub{ pub }
    {
      HRTIM_TypeDef& hrtim = this->m_hw.get_base();
      // clang-format off
      hrtim.sTimerxRegs[0].PERxR  = 0xFFDF;
      hrtim.sTimerxRegs[0].CNTxR  = 0;
      hrtim.sTimerxRegs[0].TIMxCR = 0
                                  | ((    1 << HRTIM_TIMCR_PREEN_Pos)  & HRTIM_TIMCR_PREEN_Msk)
                                  | ((    1 << HRTIM_TIMCR_TRSTU_Pos)  & HRTIM_TIMCR_TRSTU_Msk)
                                  | ((    1 << HRTIM_TIMCR_CONT_Pos)   & HRTIM_TIMCR_CONT_Msk)
                                  | ((    1 << HRTIM_TIMCR_RETRIG_Pos) & HRTIM_TIMCR_RETRIG_Msk)
                                  | ((0b101 << HRTIM_TIMCR_CK_PSC_Pos) & HRTIM_TIMCR_CK_PSC_Msk);

      hrtim.sTimerxRegs[0].CPT1xCR = config.input_pin.ext_event_msk;
      hrtim.sTimerxRegs[0].TIMxDIER = HRTIM_TIMDIER_CPT1IE | HRTIM_TIMDIER_RSTIE;  // IRQs capture-1 and roll-over

      hrtim.sCommonRegs.EECR1 = 0
                                | ((    0 << HRTIM_EECR1_EE1FAST_Pos)  & HRTIM_EECR1_EE1FAST_Msk)
                                | (( 0b01 << HRTIM_EECR1_EE1SNS_Pos)   & HRTIM_EECR1_EE1SNS_Msk)
                                | ((    0 << HRTIM_EECR1_EE1POL_Pos)   & HRTIM_EECR1_EE1POL_Msk)
                                | ((    0 << HRTIM_EECR1_EE1SRC_Pos)   & HRTIM_EECR1_EE1SRC_Msk);

      hrtim.sMasterRegs.MCR = ((0b101 << HRTIM_MCR_CK_PSC_Pos) & HRTIM_MCR_CK_PSC_Msk);
      // clang-format on

      this->m_hw.register_irq_A(this->m_irq_callback, 0);
      hrtim.sMasterRegs.MCR |= HRTIM_MCR_TACEN;
    }

  private:
    using this_t = HR_TIMER_DT_Event_Measurement;
    void trigger_event(bool const res, bool const cap, uint32_t const val)
    {
      if (res)
        this->m_base_value += this->rollover_offset;

      if (cap)
      {
        int64_t new_event = this->m_base_value + val;
        if (res && (val > (this->rollover_offset / 2)))
          new_event -= this->rollover_offset;

        if (this->m_last_event < new_event)
        {
          int64_t const diff = new_event - this->m_last_event;
          double const  f    = static_cast<double>(timer_frequency) / static_cast<double>(diff);

          this->m_pub.notify(f);
        }
        this->m_last_event = new_event - this->m_base_value;
        this->m_base_value = 0;
        return;
      }

      if (this->m_event_deadline < this->m_base_value)
      {
        this->m_pub.notify(0.0);

        this->m_base_value = 0;
        this->m_last_event = std::numeric_limits<int64_t>::max();
      }
    }

    void irq_cb()
    {
      uint32_t const reason          = HRTIM1->sTimerxRegs[0].TIMxISR;
      HRTIM1->sTimerxRegs[0].TIMxICR = reason;

      bool const     res     = (reason & HRTIM_TIMISR_RST) != 0;
      bool const     cap     = (reason & HRTIM_TIMISR_CPT1) != 0;
      uint32_t const cap_val = cap ? HRTIM1->sTimerxRegs[0].CPT1xR : 0;

      this->trigger_event(res, cap, cap_val);
    }

    static constexpr int64_t timer_frequency = 480'000'000;
    static constexpr int64_t rollover_offset = 0xFFDF;

    uC::HANDLEs::HR_TIMER_Handle_t m_hw;
    uC::Alternative_Funktion_Pin   m_input_pin;
    Notifyable_Interface&          m_pub;

    wlib::Memberfunction_Callback<this_t, void()> m_irq_callback = { *this, &this_t::irq_cb };

    int64_t m_event_deadline = timer_frequency;
    int64_t m_base_value     = 0;
    int64_t m_last_event     = 0;
  };

  class HR_TIMER_Double_Event_Measurement
  {
    using Notifyable_Interface = wlib::publisher::Publisher_Interface<double>::Notifyable_Interface&;
    struct input_pin_t
    {
      uC::GPIOs::HW_Unit const pin;
      uint32_t const           af;
      uint32_t const           ext_event_msk;
    };
    struct config_t
    {
      uC::HRTIMERs::HW_Unit const hr_timer_unit;
      input_pin_t const           input_pin_1;
      input_pin_t const           input_pin_2;
    };
    using PIN = uC::Alternative_Funktion_Pin;

  public:
    static constexpr config_t HRTIM_1__PB_05__PB_06 = {
      uC::HRTIMERs::HR_TIMER_1,
      { uC::GPIOs::B_05, 3, (1 << 8) },    // HRTIM_CPT1xCR EEV7 -->  8
      { uC::GPIOs::B_06, 3, (1 << 9) },    // HRTIM_CPT1xCR EEV8 -->  9
    };

    HR_TIMER_Double_Event_Measurement(config_t const& config, Notifyable_Interface& pub_a, Notifyable_Interface& pub_b)
        : m_hw{ config.hr_timer_unit }
        , m_input_pin_1{ config.input_pin_1.pin, PIN::Speed::High, PIN::Output_Mode::Push_Pull, PIN::Pull_Mode::No_Pull, config.input_pin_1.af }
        , m_input_pin_2{ config.input_pin_2.pin, PIN::Speed::High, PIN::Output_Mode::Push_Pull, PIN::Pull_Mode::No_Pull, config.input_pin_2.af }
        , m_evtim1{ pub_a }
        , m_evtim2{ pub_b }
    {
      HRTIM_TypeDef& hrtim = this->m_hw.get_base();
      // clang-format off
      hrtim.sTimerxRegs[0].PERxR  = 0xFFDF;
      hrtim.sTimerxRegs[0].CNTxR  = 0;
      hrtim.sTimerxRegs[0].TIMxCR = 0
                                  | ((    1 << HRTIM_TIMCR_PREEN_Pos)  & HRTIM_TIMCR_PREEN_Msk)
                                  | ((    1 << HRTIM_TIMCR_TRSTU_Pos)  & HRTIM_TIMCR_TRSTU_Msk)
                                  | ((    1 << HRTIM_TIMCR_CONT_Pos)   & HRTIM_TIMCR_CONT_Msk)
                                  | ((    1 << HRTIM_TIMCR_RETRIG_Pos) & HRTIM_TIMCR_RETRIG_Msk)
                                  | ((0b101 << HRTIM_TIMCR_CK_PSC_Pos) & HRTIM_TIMCR_CK_PSC_Msk);

      hrtim.sTimerxRegs[0].CPT1xCR = config.input_pin_1.ext_event_msk;
      hrtim.sTimerxRegs[0].CPT2xCR = config.input_pin_2.ext_event_msk;
      hrtim.sTimerxRegs[0].TIMxDIER = HRTIM_TIMDIER_CPT1IE | HRTIM_TIMDIER_CPT2IE | HRTIM_TIMDIER_RSTIE;  // IRQs capture-1, IRQs capture-2 and roll-over

      hrtim.sCommonRegs.EECR2 = 0
                                //| ((    0 << HRTIM_EECR2_EE7FAST_Pos)  & HRTIM_EECR2_EE7FAST_Msk)
                                | (( 0b01 << HRTIM_EECR2_EE7SNS_Pos)   & HRTIM_EECR2_EE7SNS_Msk)
                                | ((    0 << HRTIM_EECR2_EE7POL_Pos)   & HRTIM_EECR2_EE7POL_Msk)
                                | ((    0 << HRTIM_EECR2_EE7SRC_Pos)   & HRTIM_EECR2_EE7SRC_Msk)
                                //| ((    0 << HRTIM_EECR2_EE8FAST_Pos)  & HRTIM_EECR2_EE8FAST_Msk)
                                | (( 0b01 << HRTIM_EECR2_EE8SNS_Pos)   & HRTIM_EECR2_EE8SNS_Msk)
                                | ((    0 << HRTIM_EECR2_EE8POL_Pos)   & HRTIM_EECR2_EE8POL_Msk)
                                | ((    0 << HRTIM_EECR2_EE8SRC_Pos)   & HRTIM_EECR2_EE8SRC_Msk);

      hrtim.sMasterRegs.MCR = ((0b101 << HRTIM_MCR_CK_PSC_Pos) & HRTIM_MCR_CK_PSC_Msk);
      // clang-format on

      this->m_hw.register_irq_A(this->m_irq_callback, 0);
      hrtim.sMasterRegs.MCR |= HRTIM_MCR_TACEN;
    }

  private:
    using this_t                             = HR_TIMER_Double_Event_Measurement;
    static constexpr int64_t timer_frequency = 480'000'000;
    static constexpr int64_t rollover_offset = 0xFFDF;

    void trigger_event(bool const res, bool const cap1, uint32_t const val1, bool const cap2, uint32_t const val2)
    {
      if (res)
      {
        this->m_evtim1.rollover();
        this->m_evtim2.rollover();
      }

      if (cap1)
        this->m_evtim1.capture(res, val1);
      if (cap2)
        this->m_evtim2.capture(res, val2);

      this->m_evtim1.check_timeout();
      this->m_evtim2.check_timeout();
    }

    void irq_cb()
    {
      uint32_t const reason          = HRTIM1->sTimerxRegs[0].TIMxISR;
      HRTIM1->sTimerxRegs[0].TIMxICR = reason;

      bool const     res      = (reason & HRTIM_TIMISR_RST) != 0;
      bool const     cap1     = (reason & HRTIM_TIMISR_CPT1) != 0;
      bool const     cap2     = (reason & HRTIM_TIMISR_CPT2) != 0;
      uint32_t const cap_val1 = cap1 ? HRTIM1->sTimerxRegs[0].CPT1xR : 0;
      uint32_t const cap_val2 = cap2 ? HRTIM1->sTimerxRegs[0].CPT2xR : 0;

      this->trigger_event(res, cap1, cap_val1, cap2, cap_val2);
    }

    uC::HANDLEs::HR_TIMER_Handle_t                m_hw;
    uC::Alternative_Funktion_Pin                  m_input_pin_1;
    uC::Alternative_Funktion_Pin                  m_input_pin_2;
    wlib::Memberfunction_Callback<this_t, void()> m_irq_callback = { *this, &this_t::irq_cb };

    class event_timer_t
    {
    public:
      event_timer_t(Notifyable_Interface& pub)
          : m_pub(pub)
      {
      }

      void rollover() { this->m_base_value += this_t::rollover_offset; }

      void capture(bool const& reset, uint32_t const& cap_val)
      {
        int64_t new_event = this->m_base_value + cap_val;
        if (reset && (cap_val > (this_t::rollover_offset / 2)))
          new_event -= this_t::rollover_offset;

        if (this->m_last_event < new_event)
        {
          int64_t const diff = new_event - this->m_last_event;
          double const  f    = static_cast<double>(this_t::timer_frequency) / static_cast<double>(diff);
          this->m_pub.notify(f);
        }
        this->m_last_event = new_event - this->m_base_value;
        this->m_base_value = 0;
      }

      void check_timeout()
      {
        if (this->m_event_deadline < this->m_base_value)
        {
          this->m_pub.notify(0.0);

          this->m_base_value = 0;
          this->m_last_event = std::numeric_limits<int64_t>::max();
        }
      }

    private:
      Notifyable_Interface& m_pub;
      int64_t               m_event_deadline = timer_frequency;
      int64_t               m_base_value     = 0;
      int64_t               m_last_event     = 0;
    };

    event_timer_t m_evtim1;
    event_timer_t m_evtim2;
  };
}    // namespace uC

#endif    // UC_HR_TIMER_HPP_INCLUDED
