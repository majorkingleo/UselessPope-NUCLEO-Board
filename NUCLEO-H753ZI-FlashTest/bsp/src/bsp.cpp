#include <atomic>
#include <bslib-h753_internal_flash_update_memory.hpp>
#include <bsp.hpp>
#include <bsp_uart_usb.hpp>
#include <canopen_legacy_updater.hpp>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exmath.hpp>
#include <sensors.hpp>
#include <stm32h753xx.h>
#include <system_stm32h7xx.h>
#include <uC.hpp>

namespace BSP
{
  uC::DMA_Buffer_Allocator_Interface& get_dma_buffer_allocator()
  {
    using T = uC::DMA_Buffer_Allocator;
    static std::byte __attribute__((section(".reserved_for_DMA_BUFFER"))) buffer[1024 * 150]{};
    static T                                                              obj(buffer);
    return obj;
  }
}    // namespace BSP

namespace
{
  auto& get_uart_debug()
  {
    using T = uC::UART__TX_DMA__RX_IRQ;
    static T obj(T::UART_1__TX_A_09__RX__A_10, uC::DMA_Streams::DMA_1_Stream_0, 1024 * 5);
    return obj;
  }
}    // namespace

namespace BSP
{
  void init() {
	  init_uart_usb();
  }
  void enter_save_state() {}

  wlib::io::DigitalOutput_Interface& get_output_LED_green()
  {
    using T = uC::Output_Pin;
    static T pin{ uC::GPIOs::B_00, T::Speed::High, T::Output_Mode::Push_Pull, T::Pull_Mode::No_Pull, false };
    return pin;
  }

  wlib::CharPuplisher&        get_uart_input_debug() { return get_uart_debug().get_input_publisher(); }
  wlib::StringSink_Interface& get_uart_output_debug()
  {
    return get_usb_uart_output_debug();
	  //return get_uart_debug();
  }

}    // namespace BSP

class bsp_ADC_t
{
  using Notifyable_Interface = wlib::publisher::Publisher_Interface<BSP::analog_values_t>::Notifyable_Interface;
  struct adc_ext_trg_t
  {
    uint32_t const tigger_val;
  };

public:
  static constexpr adc_ext_trg_t TIM_6_TRGO = { 13 };

  bsp_ADC_t(Notifyable_Interface& publisher, adc_ext_trg_t adc_ext_trg)
      : m_pub{ publisher }
  {
    this->setup_dma();
    this->setup_adc_common();
    ADC_TypeDef& adc1_base = this->m_adc_1_handle.get_base();
    ADC_TypeDef& adc2_base = this->m_adc_2_handle.get_base();
    ADC_TypeDef& adc3_base = this->m_adc_3_handle.get_base();

    // Set Trigger, BOOST and Disable deep-power-down
    // clang-format off
    uint32_t const cfg_cr = ((0b01 << ADC_CFGR_EXTEN_Pos) & ADC_CFGR_EXTEN_Msk)
                          | ((adc_ext_trg.tigger_val << ADC_CFGR_EXTSEL_Pos) & ADC_CFGR_EXTSEL_Msk)
                          | ((0b11 << ADC_CFGR_DMNGT_Pos) & ADC_CFGR_DMNGT_Msk);
    uint32_t const cr     = ((0b11 << ADC_CR_BOOST_Pos) & ADC_CR_BOOST_Msk);
    // clang-format on

    adc1_base.CFGR  = cfg_cr;
    adc1_base.CFGR2 = 0;
    adc1_base.CR    = cr;

    adc2_base.CFGR  = cfg_cr;
    adc2_base.CFGR2 = 0;
    adc2_base.CR    = cr;

    adc3_base.CFGR  = cfg_cr;
    adc3_base.CFGR2 = 0;
    adc3_base.CR    = cr;

    //       || Sample 1 | Sample 2 | Sample 3 ||
    // ------||----------|----------|----------||
    // ADC1: ||   PC00   |   PC03   |   PC04   ||
    // ADC2: ||   PC01   |   PC05   |   PC05   ||
    // ADC3: ||  VSENSE  |   VREF   |   PC02   ||
    // mit :
    //   PC00 -> ADC 1 INP 10
    //   PC01 -> ADC 2 INP 11
    //   PC02 -> ADC 3 INP 12
    //   PC03 -> ADC 1 INP 13
    //   PC04 -> ADC 1 INP 4
    //   PC05 -> ADC 2 INP 8
    // VSENSE -> ADC 3 INP 18
    //   VREF -> ADC 3 INP 19

    constexpr uint32_t smt_msk12 = 0b101;
    constexpr uint32_t smt_msk3  = 0b110;
    static_assert(smt_msk12 < smt_msk3, "sample and hold time for adc 1 and 2 has to be shorter than adc3 then adc3 creates the interrupt");

    this->setup_channels(adc1_base, { { 10, smt_msk12 }, { 13, smt_msk12 }, { 4, smt_msk12 } });
    this->setup_channels(adc2_base, { { 11, smt_msk12 }, { 8, smt_msk12 }, { 8, smt_msk12 } });
    this->setup_channels(adc3_base, { { 18, smt_msk3 }, { 19, smt_msk3 }, { 12, smt_msk3 } });

    this->enable_adc_voltage_regulator(adc1_base);
    this->enable_adc_voltage_regulator(adc2_base);
    this->enable_adc_voltage_regulator(adc3_base);

    // ADC Calib
    this->preform_adc_calibration(adc1_base);
    this->preform_adc_calibration(adc2_base);
    this->preform_adc_calibration(adc3_base);

    // Enable ADC
    this->adc_enable(adc1_base);
    this->adc_enable(adc2_base);
    this->adc_enable(adc3_base);

    this->adc_start(adc1_base);
    this->adc_start(adc2_base);
    this->adc_start(adc3_base);
  }

  void setup_adc_common()
  {
    ADC_Common_TypeDef& adc12_common_base = this->m_adc_1_handle.get_common_base();
    ADC_Common_TypeDef& adc3_common_base  = this->m_adc_3_handle.get_common_base();
    // set clk for adc12 and switch in dual mode
    // clang-format off
    adc12_common_base.CCR = ((  0b0001 << ADC_CCR_PRESC_Pos) & ADC_CCR_PRESC_Msk) // 100MHz -> 25MHz // adc_ker_ck_input /2 (PRESC) --> Fadc_ker_ck_x2 /2 (rev V) --> Fadc_ker_ck
                          | ((    0b00 << ADC_CCR_CKMODE_Pos) & ADC_CCR_CKMODE_Msk)
                          | ((    0b00 << ADC_CCR_DAMDF_Pos) & ADC_CCR_DAMDF_Msk)
                          | (( 0b00110 << ADC_CCR_DUAL_Pos) & ADC_CCR_DUAL_Msk)
                          | ((     0b1 << ADC_CCR_TSEN_Pos) & ADC_CCR_TSEN_Msk)
                          | ((     0b1 << ADC_CCR_VREFEN_Pos) & ADC_CCR_VREFEN_Msk);
    // clang-format on

    // set clk for adc3
    // clang-format off
    adc3_common_base.CCR = ((  0b0001 << ADC_CCR_PRESC_Pos) & ADC_CCR_PRESC_Msk)  // 100MHz -> 25MHz // adc_ker_ck_input /2 (PRESC) --> Fadc_ker_ck_x2 /2 (rev V) --> Fadc_ker_ck
                         | ((    0b00 << ADC_CCR_CKMODE_Pos) & ADC_CCR_CKMODE_Msk)
                         | ((    0b00 << ADC_CCR_DAMDF_Pos) & ADC_CCR_DAMDF_Msk)
                         | (( 0b00000 << ADC_CCR_DUAL_Pos) & ADC_CCR_DUAL_Msk)
                         | ((     0b1 << ADC_CCR_TSEN_Pos) & ADC_CCR_TSEN_Msk)
                         | ((     0b1 << ADC_CCR_VREFEN_Pos) & ADC_CCR_VREFEN_Msk);
    // clang-format on
  }

private:
  using this_t       = bsp_ADC_t;
  using irq_reason_t = uC::HANDLEs::DMA_Stream_Handle_t::irq_reason_t;

  void adc_start(ADC_TypeDef& adc_base) { adc_base.CR |= ADC_CR_ADSTART; }

  void start_adc_voltage_regulator(ADC_TypeDef& adc_base) { adc_base.CR |= ADC_CR_ADVREGEN; }

  void wait_for_adc_voltage_regulator(ADC_TypeDef& adc_base)
  {
    while ((adc_base.ISR & ADC_ISR_LDORDY) == 0)
    {
    }
  }

  void start_adc_calibration(ADC_TypeDef& adc_base)
  {
    adc_base.CR &= ~ADC_CR_ADCALDIF;
    adc_base.CR |= ADC_CR_ADCALLIN;
    adc_base.CR |= ADC_CR_ADCAL;
  }

  void wait_for_adc_calibration(ADC_TypeDef& adc_base)
  {
    while ((adc_base.CR & ADC_CR_ADCAL) != 0)
    {
    }
  }

  void start_adc_enable(ADC_TypeDef& adc_base)
  {
    adc_base.ISR |= ADC_ISR_ADRDY;
    adc_base.CR |= ADC_CR_ADEN;
  }
  void wait_for_adc_enable(ADC_TypeDef& adc_base)
  {
    while ((adc_base.ISR & ADC_ISR_ADRDY) == 0)
    {
    }
  }

  void adc_enable(ADC_TypeDef& adc_base)
  {
    this->start_adc_enable(adc_base);
    this->wait_for_adc_enable(adc_base);
  }

  void enable_adc_voltage_regulator(ADC_TypeDef& adc_base)
  {
    this->start_adc_voltage_regulator(adc_base);
    this->wait_for_adc_voltage_regulator(adc_base);
  }

  void preform_adc_calibration(ADC_TypeDef& adc_base)
  {
    this->start_adc_calibration(adc_base);
    this->wait_for_adc_calibration(adc_base);
  }

  void setup_channels(ADC_TypeDef& adc_base, std::initializer_list<std::tuple<uint32_t, uint32_t>> list)
  {
    uint32_t len     = list.size();
    uint32_t ch[16]  = {};
    uint32_t smt[20] = {};    // sampling_time
    uint32_t p_sel   = 0;
    uint32_t idx     = 0;
    for (auto [cur_ch, cur_smt] : list)
    {
      if (idx >= 16)
        break;
      if (cur_ch >= 20)
        continue;

      p_sel |= 1 << cur_ch;
      ch[idx++] = cur_ch;

      if (smt[cur_ch] < cur_smt)
        smt[cur_ch] = cur_smt;
    }
    // clang-format off
        adc_base.PCSEL = p_sel;
        adc_base.SQR1  = (( (len - 1) << ADC_SQR1_L_Pos) & ADC_SQR1_L_Msk)
                       | ((    ch[ 0] << ADC_SQR1_SQ1_Pos) & ADC_SQR1_SQ1_Msk)
                       | ((    ch[ 1] << ADC_SQR1_SQ2_Pos) & ADC_SQR1_SQ2_Msk)
                       | ((    ch[ 2] << ADC_SQR1_SQ3_Pos) & ADC_SQR1_SQ3_Msk)
                       | ((    ch[ 3] << ADC_SQR1_SQ4_Pos) & ADC_SQR1_SQ4_Msk);
  
        adc_base.SQR2  = ((    ch[ 4] << ADC_SQR2_SQ5_Pos) & ADC_SQR2_SQ5_Msk)
                       | ((    ch[ 5] << ADC_SQR2_SQ6_Pos) & ADC_SQR2_SQ6_Msk)
                       | ((    ch[ 6] << ADC_SQR2_SQ7_Pos) & ADC_SQR2_SQ7_Msk)
                       | ((    ch[ 7] << ADC_SQR2_SQ8_Pos) & ADC_SQR2_SQ8_Msk)
                       | ((    ch[ 8] << ADC_SQR2_SQ9_Pos) & ADC_SQR2_SQ9_Msk);
  
        adc_base.SQR3  = ((    ch[ 9] << ADC_SQR3_SQ10_Pos) & ADC_SQR3_SQ10_Msk)
                       | ((    ch[10] << ADC_SQR3_SQ11_Pos) & ADC_SQR3_SQ11_Msk)
                       | ((    ch[11] << ADC_SQR3_SQ12_Pos) & ADC_SQR3_SQ12_Msk)
                       | ((    ch[12] << ADC_SQR3_SQ13_Pos) & ADC_SQR3_SQ13_Msk)
                       | ((    ch[13] << ADC_SQR3_SQ14_Pos) & ADC_SQR3_SQ14_Msk);
  
        adc_base.SQR4  = ((    ch[14] << ADC_SQR4_SQ15_Pos) & ADC_SQR4_SQ15_Msk)
                       | ((    ch[15] << ADC_SQR4_SQ16_Pos) & ADC_SQR4_SQ16_Msk);
  
        adc_base.SMPR1 = (( smt[ 0] << ADC_SMPR1_SMP0_Pos) & ADC_SMPR1_SMP0_Msk)
                       | (( smt[ 1] << ADC_SMPR1_SMP1_Pos) & ADC_SMPR1_SMP1_Msk)
                       | (( smt[ 2] << ADC_SMPR1_SMP2_Pos) & ADC_SMPR1_SMP2_Msk)
                       | (( smt[ 3] << ADC_SMPR1_SMP3_Pos) & ADC_SMPR1_SMP3_Msk)
                       | (( smt[ 4] << ADC_SMPR1_SMP4_Pos) & ADC_SMPR1_SMP4_Msk)
                       | (( smt[ 5] << ADC_SMPR1_SMP5_Pos) & ADC_SMPR1_SMP5_Msk)
                       | (( smt[ 6] << ADC_SMPR1_SMP6_Pos) & ADC_SMPR1_SMP6_Msk)
                       | (( smt[ 7] << ADC_SMPR1_SMP7_Pos) & ADC_SMPR1_SMP7_Msk)
                       | (( smt[ 8] << ADC_SMPR1_SMP8_Pos) & ADC_SMPR1_SMP8_Msk)
                       | (( smt[ 9] << ADC_SMPR1_SMP9_Pos) & ADC_SMPR1_SMP9_Msk);
  
        adc_base.SMPR2 = (( smt[10] << ADC_SMPR2_SMP10_Pos) & ADC_SMPR2_SMP10_Msk)
                       | (( smt[11] << ADC_SMPR2_SMP11_Pos) & ADC_SMPR2_SMP11_Msk)
                       | (( smt[12] << ADC_SMPR2_SMP12_Pos) & ADC_SMPR2_SMP12_Msk)
                       | (( smt[13] << ADC_SMPR2_SMP13_Pos) & ADC_SMPR2_SMP13_Msk)
                       | (( smt[14] << ADC_SMPR2_SMP14_Pos) & ADC_SMPR2_SMP14_Msk)
                       | (( smt[15] << ADC_SMPR2_SMP15_Pos) & ADC_SMPR2_SMP15_Msk)
                       | (( smt[16] << ADC_SMPR2_SMP16_Pos) & ADC_SMPR2_SMP16_Msk)
                       | (( smt[17] << ADC_SMPR2_SMP17_Pos) & ADC_SMPR2_SMP17_Msk)
                       | (( smt[18] << ADC_SMPR2_SMP18_Pos) & ADC_SMPR2_SMP18_Msk)
                       | (( smt[19] << ADC_SMPR2_SMP19_Pos) & ADC_SMPR2_SMP19_Msk);
    // clang-format on
  }

  static constexpr uint32_t number_of_burst_in_block   = 2000;
  static constexpr uint32_t number_of_samples_in_burst = 3;
  static constexpr uint32_t number_of_samples_in_block = number_of_samples_in_burst * number_of_burst_in_block;

  void setup_dma()
  {
    auto request_dma_buffer = [](std::size_t entries) -> uint32_t*
    {
      uint32_t* buffer = reinterpret_cast<uint32_t*>(BSP::get_dma_buffer_allocator().allocate<uint32_t>(entries).data());
      for (std::size_t i = 0; i < entries; i++)
      {
        buffer[i] = 0xFFFF'FFFF;
      }
      return buffer;
    };

    this->m_buffer_adc1_a = request_dma_buffer(number_of_samples_in_block);
    this->m_buffer_adc1_b = request_dma_buffer(number_of_samples_in_block);
    this->m_buffer_adc2_a = request_dma_buffer(number_of_samples_in_block);
    this->m_buffer_adc2_b = request_dma_buffer(number_of_samples_in_block);
    this->m_buffer_adc3_a = request_dma_buffer(number_of_samples_in_block);
    this->m_buffer_adc3_b = request_dma_buffer(number_of_samples_in_block);

    DMA_Stream_TypeDef& dma_base_1 = this->m_dma1_handle.get_base();
    DMA_Stream_TypeDef& dma_base_2 = this->m_dma2_handle.get_base();
    DMA_Stream_TypeDef& dma_base_3 = this->m_dma3_handle.get_base();

    this->m_dma1_handle.get_mux_base().CCR = ((9 << DMAMUX_CxCR_DMAREQ_ID_Pos) & DMAMUX_CxCR_DMAREQ_ID_Msk);
    this->m_dma2_handle.get_mux_base().CCR = ((10 << DMAMUX_CxCR_DMAREQ_ID_Pos) & DMAMUX_CxCR_DMAREQ_ID_Msk);
    this->m_dma3_handle.get_mux_base().CCR = ((115 << DMAMUX_CxCR_DMAREQ_ID_Pos) & DMAMUX_CxCR_DMAREQ_ID_Msk);

    // clang-format off
      dma_base_1.PAR  = reinterpret_cast<uint32_t>(&m_adc_1_handle.get_base().DR);
      dma_base_1.M0AR = reinterpret_cast<uint32_t>(&this->m_buffer_adc1_a[0]);
      dma_base_1.M1AR = reinterpret_cast<uint32_t>(&this->m_buffer_adc1_b[0]);
      dma_base_1.NDTR = number_of_samples_in_block;
      dma_base_1.CR   = (DMA_SxCR_MBURST & (0b00  << 23))
                      | (DMA_SxCR_PBURST & (0b00  << 21))
                      | (DMA_SxCR_CT     & (0b0   << 19))
                      | (DMA_SxCR_DBM    & (0b1   << 18))
                      | (DMA_SxCR_PL     & (0b01  << 16))
                      | (DMA_SxCR_PINCOS & (0b0   << 15))
                      | (DMA_SxCR_MSIZE  & (0b10  << 13))
                      | (DMA_SxCR_PSIZE  & (0b10  << 11))
                      | (DMA_SxCR_MINC   & (0b1   << 10))
                      | (DMA_SxCR_PINC   & (0b0   << 9))
                      | (DMA_SxCR_CIRC   & (0b1   << 8))
                      | (DMA_SxCR_DIR    & (0b00  << 6))
                      | (DMA_SxCR_PFCTRL & (0b0   << 5))
                      | (DMA_SxCR_TCIE   & (0b0   << 4))
                      | (DMA_SxCR_HTIE   & (0b0   << 3))
                      | (DMA_SxCR_TEIE   & (0b1   << 2))
                      | (DMA_SxCR_DMEIE  & (0b1   << 1))
                      | (DMA_SxCR_EN     & (0b1   << 0));

      dma_base_2.PAR  = reinterpret_cast<uint32_t>(&m_adc_2_handle.get_base().DR);
      dma_base_2.M0AR = reinterpret_cast<uint32_t>(&this->m_buffer_adc2_a[0]);
      dma_base_2.M1AR = reinterpret_cast<uint32_t>(&this->m_buffer_adc2_b[0]);
      dma_base_2.NDTR = number_of_samples_in_block;
      dma_base_2.CR   = (DMA_SxCR_MBURST & (0b00  << 23))
                      | (DMA_SxCR_PBURST & (0b00  << 21))
                      | (DMA_SxCR_CT     & (0b0   << 19))
                      | (DMA_SxCR_DBM    & (0b1   << 18))
                      | (DMA_SxCR_PL     & (0b01  << 16))
                      | (DMA_SxCR_PINCOS & (0b0   << 15))
                      | (DMA_SxCR_MSIZE  & (0b10  << 13))
                      | (DMA_SxCR_PSIZE  & (0b10  << 11))
                      | (DMA_SxCR_MINC   & (0b1   << 10))
                      | (DMA_SxCR_PINC   & (0b0   << 9))
                      | (DMA_SxCR_CIRC   & (0b1   << 8))
                      | (DMA_SxCR_DIR    & (0b00  << 6))
                      | (DMA_SxCR_PFCTRL & (0b0   << 5))
                      | (DMA_SxCR_TCIE   & (0b0   << 4))
                      | (DMA_SxCR_HTIE   & (0b0   << 3))
                      | (DMA_SxCR_TEIE   & (0b1   << 2))
                      | (DMA_SxCR_DMEIE  & (0b1   << 1))
                      | (DMA_SxCR_EN     & (0b1   << 0));

      dma_base_3.PAR  = reinterpret_cast<uint32_t>(&m_adc_3_handle.get_base().DR);
      dma_base_3.M0AR = reinterpret_cast<uint32_t>(&this->m_buffer_adc3_a[0]);
      dma_base_3.M1AR = reinterpret_cast<uint32_t>(&this->m_buffer_adc3_b[0]);
      dma_base_3.NDTR = number_of_samples_in_block;
      dma_base_3.CR   = (DMA_SxCR_MBURST & (0b00  << 23))
                      | (DMA_SxCR_PBURST & (0b00  << 21))
                      | (DMA_SxCR_CT     & (0b0   << 19))
                      | (DMA_SxCR_DBM    & (0b1   << 18))
                      | (DMA_SxCR_PL     & (0b00  << 16))
                      | (DMA_SxCR_PINCOS & (0b0   << 15))
                      | (DMA_SxCR_MSIZE  & (0b10  << 13))
                      | (DMA_SxCR_PSIZE  & (0b10  << 11))
                      | (DMA_SxCR_MINC   & (0b1   << 10))
                      | (DMA_SxCR_PINC   & (0b0   << 9))
                      | (DMA_SxCR_CIRC   & (0b1   << 8))
                      | (DMA_SxCR_DIR    & (0b00  << 6))
                      | (DMA_SxCR_PFCTRL & (0b0   << 5))
                      | (DMA_SxCR_TCIE   & (0b1   << 4))
                      | (DMA_SxCR_HTIE   & (0b0   << 3))
                      | (DMA_SxCR_TEIE   & (0b1   << 2))
                      | (DMA_SxCR_DMEIE  & (0b1   << 1))
                      | (DMA_SxCR_EN     & (0b1   << 0));
    // clang-format on
    this->m_dma3_handle.register_irq(this->m_rx_irq_cb, 6);
  }

  void irq_handler(irq_reason_t const& reason)
  {
    // BSP::get_output_LED_yellow().set(true);
    DMA_Stream_TypeDef& dma_base = this->m_dma3_handle.get_base();
    if (reason.is_transfer_complete())
    {
      if ((dma_base.CR & DMA_SxCR_CT) != 0)
        this->update_buffer_a();
      else
        this->update_buffer_b();
    }
    // BSP::get_output_LED_yellow().set(false);
  }

  void update_buffer_a() { return this->update_buffer(this->m_buffer_adc1_a, this->m_buffer_adc2_a, this->m_buffer_adc3_a); }
  void update_buffer_b() { return this->update_buffer(this->m_buffer_adc1_b, this->m_buffer_adc2_b, this->m_buffer_adc3_b); }

  void update_buffer(uint32_t* buf1, uint32_t* buf2, uint32_t* buf3)
  {
    SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(buf1), number_of_samples_in_block * sizeof(uint32_t));
    SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(buf2), number_of_samples_in_block * sizeof(uint32_t));
    SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(buf3), number_of_samples_in_block * sizeof(uint32_t));

    constexpr float u_ref         = 3.34f;
    constexpr float adc_max_value = static_cast<float>(0xFFFF);
    constexpr float u_scale       = u_ref / adc_max_value;
    constexpr float u24_scale     = u_scale * ((820.0E0f + 7.5E3f) / (820.0E0f));
    constexpr float i_scale       = u_scale / 45.0E-3f;
    constexpr float p_scale       = u_scale * ((18.7E3f + 33.2E3f) / (33.2E3f));

    auto get_adc_value = [](uint32_t* buf, uint32_t sample_idx, uint32_t burst_idx) -> uint32_t
    { return buf[burst_idx * number_of_samples_in_burst + sample_idx]; };
    auto calc_resistance         = [](uint32_t val, float r2) -> float { return (adc_max_value / static_cast<float>(val) - 1.0f) * r2; };
    auto get_ntc_adc_value       = [&](uint32_t burst_idx) -> uint32_t { return get_adc_value(buf1, 0, burst_idx); };
    auto get_ntc_board_adc_value = [&](uint32_t burst_idx) -> uint32_t { return get_adc_value(buf2, 0, burst_idx); };
    auto get_cpu_adc_value       = [&](uint32_t burst_idx) -> uint32_t { return get_adc_value(buf3, 0, burst_idx); };
    auto get_u1_adc_value        = [&](uint32_t burst_idx) -> uint32_t { return get_adc_value(buf1, 1, burst_idx); };
    auto get_i1_adc_value        = [&](uint32_t burst_idx) -> uint32_t { return get_adc_value(buf2, 1, burst_idx); };
    auto get_vref_adc_value      = [&](uint32_t burst_idx) -> uint32_t { return get_adc_value(buf3, 1, burst_idx); };
    auto get_u2_adc_value        = [&](uint32_t burst_idx) -> uint32_t { return get_adc_value(buf1, 2, burst_idx); };
    auto get_i2_adc_value        = [&](uint32_t burst_idx) -> uint32_t { return get_adc_value(buf2, 2, burst_idx); };
    auto get_pressure_adc_value  = [&](uint32_t burst_idx) -> uint32_t { return get_adc_value(buf3, 2, burst_idx); };

    auto get_resistance_ntc       = [&](uint32_t burst_idx) -> float { return calc_resistance(get_ntc_adc_value(burst_idx), 3.32E3f); };
    auto get_resistance_ntc_board = [&](uint32_t burst_idx) -> float { return calc_resistance(get_ntc_board_adc_value(burst_idx), 10.0E3f); };
    auto get_current              = [&](uint32_t burst_idx) -> float
    { return i_scale / 2.0f * (static_cast<float>(get_i1_adc_value(burst_idx)) + static_cast<float>(get_i2_adc_value(burst_idx))); };
    auto get_voltage_1        = [&](uint32_t burst_idx) -> float { return u24_scale * static_cast<float>(get_u1_adc_value(burst_idx)); };
    auto get_voltage_2        = [&](uint32_t burst_idx) -> float { return u24_scale * static_cast<float>(get_u2_adc_value(burst_idx)); };
    auto get_vref             = [&](uint32_t burst_idx) -> float { return 3.3f * this->m_vref_cal / static_cast<float>(get_vref_adc_value(burst_idx)); };
    auto get_voltage_pressure = [&](uint32_t burst_idx) -> float { return p_scale * static_cast<float>(get_pressure_adc_value(burst_idx)); };

    auto get_temperature_ntc       = [&](uint32_t burst_idx) -> float { return this->m_ntc_sen(get_resistance_ntc(burst_idx)); };
    auto get_temperature_ntc_board = [&](uint32_t burst_idx) -> float { return this->m_ntc_board_sen(get_resistance_ntc_board(burst_idx)); };
    auto get_temperature_cpu_board = [&](uint32_t burst_idx) -> float { return this->m_cpu_temp_fnc(get_cpu_adc_value(burst_idx)); };
    auto get_pressure              = [&](uint32_t burst_idx) -> float { return this->m_pressure_sen(get_voltage_pressure(burst_idx)); };

    BSP::analog_values_t analog_values = {
      .ambient_temperature{ get_temperature_ntc, number_of_burst_in_block },
      .board_temperature{ get_temperature_ntc_board, number_of_burst_in_block },
      .cpu_temperature{ get_temperature_cpu_board, number_of_burst_in_block },
      .current{ get_current, number_of_burst_in_block },
      .voltage_stage_1{ get_voltage_1, number_of_burst_in_block },
      .voltage_stage_2{ get_voltage_2, number_of_burst_in_block },
      .air_pressure{ get_pressure, number_of_burst_in_block },
      .ref_voltage{ get_vref, number_of_burst_in_block },
    };
    this->m_pub.notify(analog_values);
  }

  sensors::TDK_NTC                                                 m_ntc_sen{ sensors::TDK_NTC::TDK_8016, 10E3 };
  sensors::TDK_NTC                                                 m_ntc_board_sen{ sensors::TDK_NTC::TDK_8509, 10E3 };
  sensors::cpu_temperature_sensor_t                                m_cpu_temp_fnc{};
  sensors::NXP_MPXxx6115A                                          m_pressure_sen{ sensors::NXP_MPXxx6115A::Unit::mbar };
  uint32_t*                                                        m_buffer_adc1_a = nullptr;
  uint32_t*                                                        m_buffer_adc1_b = nullptr;
  uint32_t*                                                        m_buffer_adc2_a = nullptr;
  uint32_t*                                                        m_buffer_adc2_b = nullptr;
  uint32_t*                                                        m_buffer_adc3_a = nullptr;
  uint32_t*                                                        m_buffer_adc3_b = nullptr;
  wlib::Memberfunction_Callback<this_t, void(irq_reason_t const&)> m_rx_irq_cb{ *this, &this_t::irq_handler };
  uC::Analog_Pin                   m_pin[6]       = { uC::GPIOs::C_00, uC::GPIOs::C_01, uC::GPIOs::C_02, uC::GPIOs::C_03, uC::GPIOs::C_04, uC::GPIOs::C_05 };
  uC::HANDLEs::ADC_Handle_t        m_adc_1_handle = uC::ADCs::ADC_1;
  uC::HANDLEs::ADC_Handle_t        m_adc_2_handle = uC::ADCs::ADC_2;
  uC::HANDLEs::ADC_Handle_t        m_adc_3_handle = uC::ADCs::ADC_3;
  uC::HANDLEs::DMA_Stream_Handle_t m_dma1_handle{ uC::DMA_Streams::DMA_1_Stream_2 };
  uC::HANDLEs::DMA_Stream_Handle_t m_dma2_handle{ uC::DMA_Streams::DMA_1_Stream_3 };
  uC::HANDLEs::DMA_Stream_Handle_t m_dma3_handle{ uC::DMA_Streams::DMA_1_Stream_4 };
  float const                      m_vref_cal = static_cast<float>(*reinterpret_cast<uint16_t volatile*>(0x1FF1'E860));
  Notifyable_Interface&            m_pub;
};

namespace BSP
{
  bslib::publisher::Publisher_Interface<BSP::analog_values_t>& get_analog_value_publisher()
  {
    static bslib::publisher::LF_Publisher<analog_values_t, 5> pub;
    static bsp_ADC_t                                          adc{ pub, bsp_ADC_t::TIM_6_TRGO };
    static uC::Trigger_Timer                                  tim{ uC::TIMERs::TIMER_6, 20'000 };
    return pub;
  }
}    // namespace BSP
