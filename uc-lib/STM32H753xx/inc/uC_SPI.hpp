#pragma once
#ifndef UC_SPI_HPP
#define UC_SPI_HPP

#include <wlib.hpp>
#include <bslib.hpp>
#include <cstdint>
#include <cstring>
#include <os.hpp>
#include <uC_DMA.hpp>
#include <uC_Errors.hpp>
#include <uC_GPIO.hpp>
#include <uC_HW_Handles.hpp>

namespace uC
{
  class GPIO_CS final: public wlib::SPI::Chipselect_Interface
  {
  public:
    GPIO_CS(uC::GPIOs::HW_Unit gpio)
        : m_pin(gpio, uC::Output_Pin::Speed::Very_High, uC::Output_Pin::Output_Mode::Push_Pull, uC::Output_Pin::Pull_Mode::No_Pull, true)
    {
    }

  private:
    virtual void   select() override { this->m_pin.set(false); }
    virtual void   deselect() override { this->m_pin.set(true); }
    uC::Output_Pin m_pin;
  };

  class SPI__TX_RX_DMA final: public wlib::SPI::Hardware_Interface
  {
    using this_t       = SPI__TX_RX_DMA;
    using irq_reason_t = uC::HANDLEs::DMA_Stream_Handle_t::irq_reason_t;
    struct af_pin_t
    {
      uC::GPIOs::HW_Unit const pin;
      uint32_t const           af_val;
    };

    struct hw_cfg_t
    {
      uC::SPIs::HW_Unit const spi_name;
      af_pin_t const          sck;
      af_pin_t const          miso;
      af_pin_t const          mosi;
      uint8_t const           mux_val_rx;
      uint8_t const           mux_val_tx;
    };

  public:
    static constexpr hw_cfg_t SPI_1__SCK_A_05__MISO_A_06__MOSI_A_07{
      uC::SPIs::SPI_1, { uC::GPIOs::A_05, 5 }, { uC::GPIOs::A_06, 5 }, { uC::GPIOs::A_07, 5 }, 37, 38,
    };
    static constexpr hw_cfg_t SPI_2__SCK_B_10__MISO_B_14__MOSI_B_15{
      uC::SPIs::SPI_2, { uC::GPIOs::B_10, 5 }, { uC::GPIOs::B_14, 5 }, { uC::GPIOs::B_15, 5 }, 39, 40,
    };

    SPI__TX_RX_DMA(hw_cfg_t const&                 cfg,
                   uC::DMA_Streams::HW_Unit const& dma_stream_rx,
                   uC::DMA_Streams::HW_Unit const& dma_stream_tx,
                   std::size_t const&              buffer_size)
        : m_spi_handle(cfg.spi_name)
        , m_sck_pin(cfg.sck.pin,
                    uC::HANDLEs::GPIO_Handle_t::Speed::Very_High,
                    uC::HANDLEs::GPIO_Handle_t::Output_Mode::Push_Pull,
                    uC::HANDLEs::GPIO_Handle_t::Pull_Mode::No_Pull,
                    cfg.sck.af_val)
        , m_miso_pin(cfg.miso.pin,
                     uC::HANDLEs::GPIO_Handle_t::Speed::Very_High,
                     uC::HANDLEs::GPIO_Handle_t::Output_Mode::Push_Pull,
                     uC::HANDLEs::GPIO_Handle_t::Pull_Mode::No_Pull,
                     cfg.miso.af_val)
        , m_mosi_pin(cfg.mosi.pin,
                     uC::HANDLEs::GPIO_Handle_t::Speed::Very_High,
                     uC::HANDLEs::GPIO_Handle_t::Output_Mode::Push_Pull,
                     uC::HANDLEs::GPIO_Handle_t::Pull_Mode::No_Pull,
                     cfg.mosi.af_val)
        , m_dma_handle_rx(dma_stream_rx)
        , m_dma_handle_tx(dma_stream_tx)
        , m_sem{ 0 }
    {
      auto tmp           = BSP::get_dma_buffer_allocator().allocate<std::byte>(buffer_size);
      this->m_dma_buffer = { reinterpret_cast<std::byte*>(tmp.data()), tmp.size() };

      for (uint32_t i = 0; i < buffer_size; i++)
        this->m_dma_buffer[i] = std::byte(i & 0xFF);

      this->m_dma_handle_rx.get_mux_base().CCR = ((cfg.mux_val_rx << DMAMUX_CxCR_DMAREQ_ID_Pos) & DMAMUX_CxCR_DMAREQ_ID_Msk);
      this->m_dma_handle_tx.get_mux_base().CCR = ((cfg.mux_val_tx << DMAMUX_CxCR_DMAREQ_ID_Pos) & DMAMUX_CxCR_DMAREQ_ID_Msk);

      // clang-format off
      DMA_Stream_TypeDef& dma_base_rx = this->m_dma_handle_rx.get_base();
      dma_base_rx.PAR                 = reinterpret_cast<uint32_t>(&this->m_spi_handle.get_base().RXDR);
      dma_base_rx.CR                  = (DMA_SxCR_MBURST & (0b00  << 23))  //
                                      | (DMA_SxCR_PBURST & (0b00  << 21))  //
                                      | (DMA_SxCR_CT     & (0b0   << 19))  //
                                      | (DMA_SxCR_DBM    & (0b0   << 18))  //
                                      | (DMA_SxCR_PL     & (0b01  << 16))  //
                                      | (DMA_SxCR_PINCOS & (0b0   << 15))  //
                                      | (DMA_SxCR_MSIZE  & (0b00  << 13))  //
                                      | (DMA_SxCR_PSIZE  & (0b00  << 11))  //
                                      | (DMA_SxCR_MINC   & (0b1   << 10))  //
                                      | (DMA_SxCR_PINC   & (0b0   << 9))   //
                                      | (DMA_SxCR_CIRC   & (0b0   << 8))   //
                                      | (DMA_SxCR_DIR    & (0b00  << 6))   //
                                      | (DMA_SxCR_PFCTRL & (0b0   << 5))   //
                                      | (DMA_SxCR_TCIE   & (0b1   << 4))   //
                                      | (DMA_SxCR_HTIE   & (0b0   << 3))   //
                                      | (DMA_SxCR_TEIE   & (0b1   << 2))   //
                                      | (DMA_SxCR_DMEIE  & (0b1   << 1))   //
                                      | (DMA_SxCR_EN     & (0b0   << 0));  //

      DMA_Stream_TypeDef& dma_base_tx = this->m_dma_handle_tx.get_base();
      dma_base_tx.PAR                 = reinterpret_cast<uint32_t>(&this->m_spi_handle.get_base().TXDR);
      dma_base_tx.CR                  = (DMA_SxCR_MBURST & (0b00  << 23))  //
                                      | (DMA_SxCR_PBURST & (0b00  << 21))  //
                                      | (DMA_SxCR_CT     & (0b0   << 19))  //
                                      | (DMA_SxCR_DBM    & (0b0   << 18))  //
                                      | (DMA_SxCR_PL     & (0b00  << 16))  //
                                      | (DMA_SxCR_PINCOS & (0b0   << 15))  //
                                      | (DMA_SxCR_MSIZE  & (0b00  << 13))  //
                                      | (DMA_SxCR_PSIZE  & (0b00  << 11))  //
                                      | (DMA_SxCR_MINC   & (0b1   << 10))  //
                                      | (DMA_SxCR_PINC   & (0b0   << 9))   //
                                      | (DMA_SxCR_CIRC   & (0b0   << 8))   //
                                      | (DMA_SxCR_DIR    & (0b01  << 6))   //
                                      | (DMA_SxCR_PFCTRL & (0b0   << 5))   //
                                      | (DMA_SxCR_TCIE   & (0b0   << 4))   //
                                      | (DMA_SxCR_HTIE   & (0b0   << 3))   //
                                      | (DMA_SxCR_TEIE   & (0b1   << 2))   //
                                      | (DMA_SxCR_DMEIE  & (0b1   << 1))   //
                                      | (DMA_SxCR_EN     & (0b0   << 0));  //
      // clang-format on
      this->m_dma_handle_rx.register_irq(this->m_transfer_complete_cb, 10);
    }

    ~SPI__TX_RX_DMA() = default;

  private:
    void p_start_dma(uint32_t len)
    {
      DMA_Stream_TypeDef& dma_base_rx = this->m_dma_handle_rx.get_base();
      DMA_Stream_TypeDef& dma_base_tx = this->m_dma_handle_tx.get_base();

      this->m_dma_handle_rx.clear_irq_flags();
      this->m_dma_handle_tx.clear_irq_flags();

      // clang-format off
      dma_base_rx.M0AR = reinterpret_cast<uint32_t>(&this->m_dma_buffer[0]);
      dma_base_rx.NDTR = len;
      dma_base_tx.M0AR = reinterpret_cast<uint32_t>(&this->m_dma_buffer[0]);
      dma_base_tx.NDTR = len;

      dma_base_rx.CR   |=DMA_SxCR_EN;
      dma_base_tx.CR   |=DMA_SxCR_EN;
      // clang-format on
    }

    virtual void enable(SPI_configuration_t const& cfg) override
    {
      this->m_tex.lock();

      uint32_t const mbr = [](uint32_t clk, uint32_t br) -> uint32_t
      {
        if (clk / 2 <= br)
          return 0b000;

        if (clk / 4 <= br)
          return 0b001;

        if (clk / 8 <= br)
          return 0b010;

        if (clk / 16 <= br)
          return 0b011;

        if (clk / 32 <= br)
          return 0b100;

        if (clk / 64 <= br)
          return 0b101;

        if (clk / 128 <= br)
          return 0b110;

        return 0b111;
      }(this->m_spi_handle.get_clk(), cfg.get_baudrate());

      uint32_t const lsb  = cfg.get_bitorder() == wlib::SPI::SPI_configuration_t::Bitorder::LSB_first ? 0b1 : 0b0;
      uint32_t const cpol = [](wlib::SPI::SPI_configuration_t::Mode mode) -> uint32_t
      {
        if (mode == wlib::SPI::SPI_configuration_t::Mode::CPOL_1__CPHA_0)
          return 0b1;
        if (mode == wlib::SPI::SPI_configuration_t::Mode::CPOL_1__CPHA_1)
          return 0b1;
        return 0b0;
      }(cfg.get_mode());

      uint32_t const cpha = [](wlib::SPI::SPI_configuration_t::Mode mode) -> uint32_t
      {
        if (mode == wlib::SPI::SPI_configuration_t::Mode::CPOL_0__CPHA_1)
          return 0b1;
        if (mode == wlib::SPI::SPI_configuration_t::Mode::CPOL_1__CPHA_1)
          return 0b1;
        return 0b0;
      }(cfg.get_mode());

      SPI_TypeDef& spi_base = this->m_spi_handle.get_base();
      spi_base.CR1          = SPI_CR1_SSI;
      spi_base.CFG1         = ((mbr << SPI_CFG1_MBR_Pos) & SPI_CFG1_MBR_Msk) | ((0b0 << SPI_CFG1_CRCEN_Pos) & SPI_CFG1_CRCEN_Msk) |
                      ((0b1 << SPI_CFG1_TXDMAEN_Pos) & SPI_CFG1_TXDMAEN_Msk) | ((0b1 << SPI_CFG1_RXDMAEN_Pos) & SPI_CFG1_RXDMAEN_Msk) |
                      ((0b00 << SPI_CFG1_UDRDET_Pos) & SPI_CFG1_UDRDET_Msk) | ((0b00 << SPI_CFG1_UDRCFG_Pos) & SPI_CFG1_UDRCFG_Msk) |
                      ((0b0000 << SPI_CFG1_FTHLV_Pos) & SPI_CFG1_FTHLV_Msk) | ((0b00111 << SPI_CFG1_DSIZE_Pos) & SPI_CFG1_DSIZE_Msk);

      spi_base.CFG2 = ((0b1 << SPI_CFG2_AFCNTR_Pos) & SPI_CFG2_AFCNTR_Msk) | ((0b0 << SPI_CFG2_SSOM_Pos) & SPI_CFG2_SSOM_Msk) |
                      ((0b0 << SPI_CFG2_SSOE_Pos) & SPI_CFG2_SSOE_Msk) | ((0b0 << SPI_CFG2_SSIOP_Pos) & SPI_CFG2_SSIOP_Msk) |
                      ((0b1 << SPI_CFG2_SSM_Pos) & SPI_CFG2_SSM_Msk) | ((cpol << SPI_CFG2_CPOL_Pos) & SPI_CFG2_CPOL_Msk) |
                      ((cpha << SPI_CFG2_CPHA_Pos) & SPI_CFG2_CPHA_Msk) | ((lsb << SPI_CFG2_LSBFRST_Pos) & SPI_CFG2_LSBFRST_Msk) |
                      ((0b1 << SPI_CFG2_MASTER_Pos) & SPI_CFG2_MASTER_Msk) | ((0b000 << SPI_CFG2_SP_Pos) & SPI_CFG2_SP_Msk) |
                      ((0b00 << SPI_CFG2_COMM_Pos) & SPI_CFG2_COMM_Msk) | ((0b0 << SPI_CFG2_IOSWP_Pos) & SPI_CFG2_IOSWP_Msk) |
                      ((0b0000 << SPI_CFG2_MIDI_Pos) & SPI_CFG2_MIDI_Msk) | ((0b0000 << SPI_CFG2_MSSI_Pos) & SPI_CFG2_MSSI_Msk);

      spi_base.CR1 =
          ((0b1 << SPI_CR1_SSI_Pos) & SPI_CR1_SSI_Msk) | ((0b1 << SPI_CR1_MASRX_Pos) & SPI_CR1_MASRX_Msk) | ((0b0 << SPI_CR1_SPE_Pos) & SPI_CR1_SPE_Msk);
      spi_base.CR1 |= SPI_CR1_SPE;
      spi_base.CR1 |= SPI_CR1_CSTART;

    };
    virtual void disable() override
    {
      this->m_spi_handle.get_base().CR1 &= ~SPI_CR1_SPE;
      this->m_tex.unlock();
    };

    virtual void transcieve(std::byte const* tx, std::byte* rx, std::size_t const& len) override
    {
      for (uint32_t pos_idx = 0; pos_idx < len;)
      {
        uint32_t blk_len = len - pos_idx;
        if (blk_len > this->m_dma_buffer.size())
          blk_len = this->m_dma_buffer.size();

        if (tx != nullptr)
        {
          for (uint32_t i = 0; i < blk_len; i++)
            this->m_dma_buffer[i] = tx[pos_idx + i];
        }

        SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t*>(this->m_dma_buffer.data()), this->m_dma_buffer.size());
        this->p_start_dma(blk_len);
        this->m_sem.acquire();
        SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(this->m_dma_buffer.data()), this->m_dma_buffer.size());

        if (rx != nullptr)
        {
          for (uint32_t i = 0; i < blk_len; i++)
            rx[pos_idx + i] = this->m_dma_buffer[i];
        }

        pos_idx += blk_len;
      }
    };

    void p_finish_transmission(irq_reason_t const& reason)
    {
      if (reason.is_transfer_complete())
        this->m_sem.release();
    }

    std::atomic<bool>                                                m_trans_ongoing = false;
    std::atomic<uint32_t>                                            m_cur_blk_len   = 0;
    wlib::Memberfunction_Callback<this_t, void(irq_reason_t const&)> m_transfer_complete_cb{ *this, &this_t::p_finish_transmission };

    uC::HANDLEs::SPI_Handle_t        m_spi_handle;
    uC::Alternative_Funktion_Pin     m_sck_pin;
    uC::Alternative_Funktion_Pin     m_miso_pin;
    uC::Alternative_Funktion_Pin     m_mosi_pin;
    uC::HANDLEs::DMA_Stream_Handle_t m_dma_handle_rx;
    uC::HANDLEs::DMA_Stream_Handle_t m_dma_handle_tx;
    std::span<std::byte>             m_dma_buffer;
    std::byte*                       m_tmp_rx = nullptr;
    os::binary_semaphore             m_sem;
    os::mutex                        m_tex;
  };


}    // namespace uC

#endif    // !UC_GPIO_HPP
