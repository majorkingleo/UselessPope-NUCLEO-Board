#pragma once
#ifndef SERIAL_COMMAND_PARSER_HPP_INCLUDED
#define SERIAL_COMMAND_PARSER_HPP_INCLUDED

#include <bslib.hpp>
#include <os.hpp>
#include <span>
#include <string_view>
#include <wlib.hpp>

namespace app
{
  namespace Serial_Commando_Parser
  {

    using stack_t              = os::internal::stack_t;
    using StringSink_Interface = wlib::StringSink_Interface;

    class CMD
    {
    public:
      using callback_t = wlib::Callback<bool(StringSink_Interface&, std::string_view)>;

      CMD(std::string_view name, std::string_view help, callback_t& cb)
          : m_name(name)
          , m_help(help)
          , m_cb(cb)
      {
      }

      ~CMD() = default;

      bool             is(std::string_view name) const noexcept { return this->m_name == name; }
      std::string_view get_name() const noexcept { return this->m_name; }
      std::string_view get_help() const noexcept { return this->m_help; }
      bool             execute(StringSink_Interface& sink, std::string_view param) noexcept { return this->m_cb(sink, param); }

    private:
      std::string_view m_name;
      std::string_view m_help;
      callback_t&      m_cb;
    };

    template <std::size_t N> class Parser;

    template <> class Parser<0>
    {
      using this_t = Parser<0>;

    public:
      Parser(wlib::publisher::Publisher_Interface<char>& pub,
             std::span<char>                             line_buffer,
             std::span<CMD>                              cmds,
             wlib::StringSink_Interface&                sink,
             std::span<stack_t>                          stack);

    private:
      void notify_new_char(char const& value);
      bool execute(std::string_view cmd_str);
      void process();
      bool show_help(StringSink_Interface& sink, std::string_view param);
      void print_help(StringSink_Interface& sink);

      bslib::container::SPSC<char, 1024>                                  m_input_buffer = {};
      std::span<stack_t>                                                  m_stack        = {};
      os::Static_MemberfunctionCallbackTask<this_t, 0>                    m_worker       = { *this, &this_t::process, m_stack, "cmd_parser" };
      wlib::Memberfunction_Callback<this_t, CMD::callback_t::signature_t> m_help_cb      = { *this, &this_t::show_help };
      wlib::publisher::Memberfunction_CallbackSubscriber<this_t, char>    m_sub          = { *this, &this_t::notify_new_char };
      CMD                                                                 m_help_cmd     = { "?", "shows help", this->m_help_cb };
      std::span<char>                                                     m_line_buffer  = {};
      std::span<CMD>                                                      m_cmds         = {};
      StringSink_Interface&                                               m_sink;
    };

    template <std::size_t N> class Parser: public Parser<0>
    {
    protected:
      std::array<stack_t, (N + sizeof(stack_t) - 1) / sizeof(stack_t)> m_stack;

    public:
      Parser(wlib::publisher::Publisher_Interface<char>& pub, std::span<char> line_buffer, std::span<CMD> cmds, wlib::StringSink_Interface& sink)
          : Parser<0>(pub, line_buffer, cmds, sink, m_stack)
      {
      }
    };

  }    // namespace Serial_Commando_Parser

}    // namespace app

#endif
