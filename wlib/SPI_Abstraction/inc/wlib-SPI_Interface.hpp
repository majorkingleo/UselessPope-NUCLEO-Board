#pragma once
#ifndef WLIB_SPI_INTERFACE_HPP_INCLUDED
#define WLIB_SPI_INTERFACE_HPP_INCLUDED

#include <cstddef>
#include <cstdint>

namespace wlib::SPI
{
  class SPI_configuration_t;
  class Hardware_handle_t;
  class Channel_handle_t;
  class Connection_handle_t;

  class SPI_configuration_t
  {
  public:
    enum class Mode
    {
      Mode_0,
      Mode_1,
      Mode_2,
      Mode_3,
      CPOL_0__CPHA_0 = Mode_0,
      CPOL_0__CPHA_1 = Mode_1,
      CPOL_1__CPHA_0 = Mode_2,
      CPOL_1__CPHA_1 = Mode_3,
    };

    enum class Bitorder
    {
      MSB_first,
      LSB_first,
    };

    constexpr SPI_configuration_t(uint32_t const& max_baudrate, Mode const& mode = Mode::Mode_0, Bitorder const& bitorder = Bitorder::MSB_first)
        : m_baudrate(max_baudrate)
        , m_mode(mode)
        , m_bitorder(bitorder)
    {
    }

    [[nodiscard]] constexpr uint32_t get_baudrate() const { return this->m_baudrate; }
    [[nodiscard]] constexpr Mode     get_mode() const { return this->m_mode; }
    [[nodiscard]] constexpr Bitorder get_bitorder() const { return this->m_bitorder; }

  private:
    uint32_t m_baudrate;
    Mode     m_mode;
    Bitorder m_bitorder;
  };

  class Connection_Interface
  {
  public:
    virtual ~Connection_Interface() = default;

    virtual void transcieve(std::byte const* tx, std::byte* rx, std::size_t const& len) = 0;
  };

  class Hardware_Interface: protected Connection_Interface
  {
  public:
    using SPI_configuration_t = wlib::SPI::SPI_configuration_t;

    Hardware_Interface()                                     = default;
    Hardware_Interface(Hardware_Interface const&)            = delete;
    Hardware_Interface(Hardware_Interface&&)                 = delete;
    Hardware_Interface& operator=(Hardware_Interface const&) = delete;
    Hardware_Interface& operator=(Hardware_Interface&&)      = delete;
    virtual ~Hardware_Interface()                            = default;

  protected:
    static Hardware_Interface& get_dummy();
    virtual void               enable(SPI_configuration_t const& cfg) = 0;
    virtual void               disable()                              = 0;

    friend Hardware_handle_t;
    friend Channel_handle_t;
    friend Connection_handle_t;
  };

  class Chipselect_Interface
  {
  public:
    Chipselect_Interface()                                       = default;
    Chipselect_Interface(Chipselect_Interface const&)            = delete;
    Chipselect_Interface(Chipselect_Interface&&)                 = delete;
    Chipselect_Interface& operator=(Chipselect_Interface const&) = delete;
    Chipselect_Interface& operator=(Chipselect_Interface&&)      = delete;
    virtual ~Chipselect_Interface()                              = default;

  protected:
    virtual void select()   = 0;
    virtual void deselect() = 0;

    friend Hardware_handle_t;
    friend Channel_handle_t;
    friend Connection_handle_t;
  };

  class Connection_handle_t: public Connection_Interface
  {
  public:
    Connection_handle_t(Hardware_Interface& hw, Chipselect_Interface& cs, Connection_Interface& con);
    Connection_handle_t(Connection_handle_t const&)            = delete;
    Connection_handle_t(Connection_handle_t&&)                 = delete;
    Connection_handle_t& operator=(Connection_handle_t const&) = delete;
    Connection_handle_t& operator=(Connection_handle_t&&)      = delete;
    ~Connection_handle_t();

    void transcieve(std::byte const* tx, std::byte* rx, std::size_t const& len) override;

  private:
    Hardware_Interface*   m_hw;
    Chipselect_Interface* m_cs;
    Connection_Interface* m_con;
  };

  class Channel_handle_t
  {
  public:
    Channel_handle_t(Hardware_Interface& hw, Chipselect_Interface& cs, SPI_configuration_t const& cfg);
    Channel_handle_t(Channel_handle_t const&)            = delete;
    Channel_handle_t(Channel_handle_t&&)                 = delete;
    Channel_handle_t& operator=(Channel_handle_t const&) = delete;
    Channel_handle_t& operator=(Channel_handle_t&&)      = delete;
    ~Channel_handle_t();

    [[nodiscard]] Connection_handle_t select() &;
    [[nodiscard]] Connection_handle_t select() &&;

  private:
    Hardware_Interface*   m_hw;
    Chipselect_Interface* m_cs;
    Connection_Interface* m_con;
  };

  class Hardware_handle_t
  {
  public:
    Hardware_handle_t(Hardware_Interface& hw, SPI_configuration_t const& cfg);
    Hardware_handle_t(Hardware_handle_t const&)            = delete;
    Hardware_handle_t(Hardware_handle_t&&)                 = delete;
    Hardware_handle_t& operator=(Hardware_handle_t const&) = delete;
    Hardware_handle_t& operator=(Hardware_handle_t&&)      = delete;
    ~Hardware_handle_t();

    [[nodiscard]] Connection_handle_t select(Chipselect_Interface& cs) &;
    [[nodiscard]] Connection_handle_t select(Chipselect_Interface& cs) &&;

  private:
    class single_use_hardware_lock_t final: public Hardware_Interface
    {
    public:
      void enable(SPI_configuration_t const&) override;
      void disable() override;
      void transcieve(std::byte const*, std::byte*, std::size_t const&) override;

      void lock_hw();

    private:
      bool m_is_locked = false;
    };

    Hardware_Interface*        m_hw;
    Connection_Interface*      m_con;
    single_use_hardware_lock_t m_single_use_hw;
  };

  class Channel_Provider
  {
  public:
    constexpr Channel_Provider(Hardware_Interface& hw, Chipselect_Interface& cs)
        : m_hw(hw)
        , m_cs(cs)
    {
    }

    [[nodiscard]] Channel_handle_t request(SPI_configuration_t const& cfg) { return Channel_handle_t{ this->m_hw, this->m_cs, cfg }; }

  private:
    Hardware_Interface&   m_hw;
    Chipselect_Interface& m_cs;
  };

  class Connection_Provider
  {
  public:
    constexpr Connection_Provider(Hardware_Interface& hw, Chipselect_Interface& cs)
        : m_channel_pro(hw, cs)
    {
    }

    [[nodiscard]] Connection_handle_t request(SPI_configuration_t const& cfg) { return this->m_channel_pro.request(cfg).select(); }

  private:
    Channel_Provider m_channel_pro;
  };
}    // namespace wlib::SPI

#endif
