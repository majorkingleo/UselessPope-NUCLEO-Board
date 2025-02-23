#include <serial_command_parser.hpp>

namespace app::Serial_Commando_Parser
{
  Parser<0>::Parser(wlib::publisher::Publisher_Interface<char>& pub,
                    std::span<char>                             line_buffer,
                    std::span<CMD>                              cmds,
                    wlib::StringSink_Interface&                sink,
                    std::span<stack_t>                          stack)
      : m_stack(stack)
      , m_line_buffer(line_buffer)
      , m_cmds{ cmds }
      , m_sink(sink)
  {
    this->m_sub.subscribe(pub);
    this->m_worker.start();

    this->print_help(sink);
  }

  void Parser<0>::notify_new_char(char const& value)
  {
    this->m_input_buffer.push_back(value);
    this->m_worker.notify();
  }

  bool Parser<0>::execute(std::string_view cmd_str)
  {
    constexpr char const* const withespace_chars = " \t";

    std::size_t      pos_name_begin = cmd_str.find_first_not_of(withespace_chars);
    std::size_t      pos_name_end   = std::string_view::npos;
    std::string_view name{};
    if (pos_name_begin != std::string_view::npos)
    {
      pos_name_end = cmd_str.find_first_of(withespace_chars, pos_name_begin);
      name         = cmd_str.substr(pos_name_begin, pos_name_end - pos_name_begin);
    }

    std::size_t      pos_param_begin = cmd_str.find_first_not_of(withespace_chars, pos_name_end);
    std::size_t      pos_param_end   = std::string_view::npos;
    std::string_view param{};
    if (pos_param_begin != std::string_view::npos)
    {
      pos_param_end = cmd_str.find_last_not_of(withespace_chars) + 1;
      param         = cmd_str.substr(pos_param_begin, pos_param_end - pos_param_begin);
    }

    if (this->m_help_cmd.is(name))
      return this->m_help_cmd.execute(this->m_sink, param);

    for (auto& cmd : this->m_cmds)
    {
      if (!cmd.is(name))
        continue;

      try
      {
        return cmd.execute(this->m_sink, param);
      }
      catch (...)
      {
        return false;
      }
    }
    return false;
  }

  void Parser<0>::process()
  {
    bool              cmd_began = false;
    char*             pos       = this->m_line_buffer.data();
    char const* const end       = this->m_line_buffer.data() + this->m_line_buffer.size() - 1;

    while (os::this_thread::keep_running())
    {
      os::this_thread::wait_for_notify();

      for (auto tmp = this->m_input_buffer.pop_front(); tmp.has_value(); tmp = this->m_input_buffer.pop_front())
      {
        char cur = tmp.value();

        if (cur == '~')
        {
          cmd_began = true;
          pos       = this->m_line_buffer.data();
          *pos      = '\0';
          continue;
        }

        if (!cmd_began)
          continue;

        // input from a console on windows will only give us \r, so we
        // also have to react on this
        if (cur == '\n' || cur == '\r')
        {
          cmd_began = false;
          *pos      = '\0';

          if (this->execute(std::string_view{ this->m_line_buffer.data(), pos }))
            this->m_sink("CMD SUCCESS\n");
          else
            this->m_sink("CMD FAIL\n");

          continue;
        }

        if (pos < end)
        {
          *pos++ = cur;
          continue;
        }
      }
    }
  }

  bool Parser<0>::show_help(wlib::StringSink_Interface& sink, std::string_view param)
  {
    if (param.length() != 0)
      return false;

    this->print_help(sink);
    return true;
  }

  void Parser<0>::print_help(wlib::StringSink_Interface& sink)
  {
    char              cmd_str_buf[256] = {};
    char              buf[2048]        = {};
    char*             pos              = buf;
    char const* const end              = buf + sizeof(buf);

    pos += snprintf(pos, end - pos, "#################################\n");
    pos += snprintf(pos, end - pos, "#     Serial-Command-Parser     #\n");
    pos += snprintf(pos, end - pos, "#################################\n");

    snprintf(cmd_str_buf, sizeof(cmd_str_buf), "~%s", this->m_help_cmd.get_name().data());
    pos += snprintf(pos, end - pos, "# %30s: %s\n", cmd_str_buf, this->m_help_cmd.get_help().data());
    for (auto& cmd : this->m_cmds)
    {
      snprintf(cmd_str_buf, sizeof(cmd_str_buf), "~%s", cmd.get_name().data());
      pos += snprintf(pos, end - pos, "# %30s: %s\n", cmd_str_buf, cmd.get_help().data());
    }
    pos += snprintf(pos, end - pos, "\n\n");
    sink(buf);
  }

}    // namespace app::Serial_Commando_Parser
