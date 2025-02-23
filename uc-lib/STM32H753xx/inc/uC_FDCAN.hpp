#pragma once
#ifndef UC_FDCAN_HPP
#define UC_FDCAN_HPP

#include <uC_GPIO.hpp>
#include <uC_HW_Handles.hpp>

namespace uC
{
  class FDCAN_HW
  {
    struct af_pin_t
    {
      uC::GPIOs::HW_Unit const pin;
      uint32_t const           af_val;
    };
    struct hw_cfg
    {
      uC::FDCANs::HW_Unit const hw_unit;
      af_pin_t const            rx_pin;
      af_pin_t const            tx_pin;
    };

  public:
    static constexpr hw_cfg FDCAN_1__RX_A11__TX_A12 = { uC::FDCANs::FDCAN_1, { uC::GPIOs::A_11, 9 }, { uC::GPIOs::A_12, 9 } };
    static constexpr hw_cfg FDCAN_1__RX_D00__TX_D01 = { uC::FDCANs::FDCAN_1, { uC::GPIOs::D_00, 9 }, { uC::GPIOs::D_01, 9 } };
    static constexpr hw_cfg FDCAN_1__RX_A11__TX_B09 = { uC::FDCANs::FDCAN_1, { uC::GPIOs::A_11, 9 }, { uC::GPIOs::B_09, 9 } };

    static constexpr hw_cfg FDCAN_2__RX_B05__TX_B06 = { uC::FDCANs::FDCAN_2, { uC::GPIOs::B_05, 9 }, { uC::GPIOs::B_06, 9 } };
    static constexpr hw_cfg FDCAN_2__RX_B12__TX_B13 = { uC::FDCANs::FDCAN_2, { uC::GPIOs::B_12, 9 }, { uC::GPIOs::B_13, 9 } };

    FDCAN_HW(hw_cfg const& hw_cfg)
      : m_rx(hw_cfg.rx_pin.pin,
        uC::Alternative_Funktion_Pin::Speed::High,
        uC::Alternative_Funktion_Pin::Output_Mode::Push_Pull,
        uC::Alternative_Funktion_Pin::Pull_Mode::No_Pull,
        hw_cfg.rx_pin.af_val)
      , m_tx(hw_cfg.tx_pin.pin,
        uC::Alternative_Funktion_Pin::Speed::High,
        uC::Alternative_Funktion_Pin::Output_Mode::Push_Pull,
        uC::Alternative_Funktion_Pin::Pull_Mode::No_Pull,
        hw_cfg.tx_pin.af_val)
      , m_fdcan(hw_cfg.hw_unit)
    {
      if (!uC::HW_Manager::lock(this->m_fdcan))
        uC::Errors::multiple_use();

      this->m_fdcan.get_clk_bit().set();
    }

    ~FDCAN_HW()
    {
      if (uC::HW_Manager::unlock(this->m_fdcan) == 0)
        this->m_fdcan.get_clk_bit().reset();
    }

    std::size_t get_HW_idx() const
    {
      if (this->m_fdcan == uC::FDCANs::FDCAN_1)
        return 0;
      if (this->m_fdcan == uC::FDCANs::FDCAN_2)
        return 1;
      return std::numeric_limits<std::size_t>::max();
    }

  private:
    uC::Alternative_Funktion_Pin m_rx;
    uC::Alternative_Funktion_Pin m_tx;
    uC::FDCANs::HW_Unit          m_fdcan;
  };

}    // namespace uC

#endif
