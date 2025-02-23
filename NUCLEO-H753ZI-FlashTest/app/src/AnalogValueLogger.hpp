#pragma once

#include <bsp.hpp>

namespace app
{

  class analog_value_logger
  {
  public:
    using analog_values_t = BSP::analog_values_t;

    analog_value_logger(wlib::publisher::Publisher_Interface<analog_values_t>& analog_value_pup,
                        wlib::StringSink_Interface&                            sink = wlib::StringSink_Interface::get_null_sink());

    auto get_analog_values() const -> analog_values_t;

    auto print(wlib::StringSink_Interface& sink) const -> void;

  private:
    void new_analog_value(BSP::analog_values_t const& values);

    void process();

    using this_t                                                                               = analog_value_logger;
    mutable os::mutex                                                           m_mtex         = {};
    wlib::publisher::Memberfunction_CallbackSubscriber<this_t, analog_values_t> m_sub          = { *this, &this_t::new_analog_value };
    os::Static_MemberfunctionCallbackTask<this_t, 5120>                         m_worker       = { *this, &this_t::process, "anal" };
    bslib::container::SPSC<analog_values_t, 2>                                  m_input_buffer = {};
    wlib::container::circular_buffer_t<analog_values_t, 10>                     m_circ_buffer  = {};
    wlib::StringSink_Interface&                                                 m_sink;
  };

}    // namespace app
