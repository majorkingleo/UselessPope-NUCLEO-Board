#include <wlib-SPI_Interface.hpp>

void throw_multible_active_con_error();

namespace wlib::SPI
{
  namespace
  {
    class hw_dummy final: public Hardware_Interface
    {
    public:
      virtual void transcieve(std::byte const*, std::byte*, std::size_t const&) override {}
      virtual void enable(const SPI_configuration_t&) override {}
      virtual void disable() override {}
    } hw_dummy_obj;

  }    // namespace

  Hardware_Interface& Hardware_Interface::get_dummy() { return hw_dummy_obj; }

  Connection_handle_t::Connection_handle_t(Hardware_Interface& hw, Chipselect_Interface& cs, Connection_Interface& con)
      : m_hw(&hw)
      , m_cs(&cs)
      , m_con(&con)
  {
    this->m_cs->select();
  }
  Connection_handle_t::~Connection_handle_t()
  {
    this->m_cs->deselect();
    this->m_hw->disable();
  }
  void Connection_handle_t::transcieve(std::byte const* tx, std::byte* rx, std::size_t const& len) { return this->m_con->transcieve(tx, rx, len); }

  Channel_handle_t::Channel_handle_t(Hardware_Interface& hw, Chipselect_Interface& cs, SPI_configuration_t const& cfg)
      : m_hw(&hw)
      , m_cs(&cs)
      , m_con(&hw)
  {
    this->m_hw->enable(cfg);
  }
  Channel_handle_t::~Channel_handle_t() { this->m_hw->disable(); }

  Connection_handle_t Channel_handle_t::select() & { return Connection_handle_t(Hardware_Interface::get_dummy(), *this->m_cs, *this->m_con); }
  Connection_handle_t Channel_handle_t::select() &&
  {
    Hardware_Interface& hw = *this->m_hw;
    this->m_hw             = &Hardware_Interface::get_dummy();
    return Connection_handle_t(hw, *this->m_cs, *this->m_con);
  }

  Hardware_handle_t::Hardware_handle_t(Hardware_Interface& hw, SPI_configuration_t const& cfg)
      : m_hw(&hw)
      , m_con(&hw)
  {
    this->m_hw->enable(cfg);
  }

  Hardware_handle_t::~Hardware_handle_t() { this->m_hw->disable(); }

  Connection_handle_t Hardware_handle_t::select(Chipselect_Interface& cs) &
  {
    this->m_single_use_hw.lock_hw();
    return Connection_handle_t(this->m_single_use_hw, cs, *this->m_con);
  }
  Connection_handle_t Hardware_handle_t::select(Chipselect_Interface& cs) &&
  {
    Hardware_Interface& hw = *this->m_hw;
    this->m_hw             = &Hardware_Interface::get_dummy();
    return Connection_handle_t(hw, cs, *this->m_con);
  }

  void Hardware_handle_t::single_use_hardware_lock_t::enable(SPI_configuration_t const&) {};
  void Hardware_handle_t::single_use_hardware_lock_t::disable() { this->m_is_locked = false; };
  void Hardware_handle_t::single_use_hardware_lock_t::transcieve(std::byte const*, std::byte*, std::size_t const&) {}

  void Hardware_handle_t::single_use_hardware_lock_t::lock_hw()
  {
    if (this->m_is_locked)
      throw_multible_active_con_error();

    this->m_is_locked = true;
  }

}    // namespace wlib::SPI

void throw_multible_active_con_error()
{
  while (true)
    ;
}
