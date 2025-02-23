
#ifndef SIMULATOR
#  include <stm32h7xx_hal_conf.h>
#  include <stm32h753xx.h>
#  include <stm32h7xx_hal_flash.h>
#  include <stm32h7xx_hal_flash_ex.h>
#  include <stm32h7xx_hal_cortex.h>
#endif

#include "CppUtilsUartDebug.hpp"
#include "H7TwoFace.h"
#include "SimpleFlashFsFileBuffer.h"
#include "bsp_internal_fs.hpp"
#include "task_status_led.h"

#include <bslib.hpp>
#include <bsp.hpp>
#include <bsp_uart_usb.hpp>
#include <cstring>
#include <functional>
#include <os.hpp>
#include <static_format.h>
#include <wlib.hpp>
#include <serial_command_parser.hpp>
#include <string_utils.h>
#include <SimpleIni.h>
#include <SimpleFlashFsFileBuffer.h>
#include <unistd.h>
#include <can_config.hpp>
#include <canopen.hpp>
#include <canopen_legacy_updater.hpp>
#include <bsp_canopen.hpp>
#include "serial_nr_canopen_adapter.hpp"
#include "AnalogValueLogger.hpp"
#include <canopen_instance1_names.h>
#include "UtilityValues_CANOpen_Adapter.hpp"
#include "FunnelModule_CANOpen_Adapter.hpp"
#include "SimpleFill_CANOpen_Adapter.hpp"
#include "AirPump_CANOpen_Adapter.hpp"
#include "FunnelModule.hpp"
#include "SimpleFillModule.hpp"

using namespace Tools;
using namespace app;

namespace
{

  //std::span<const std::byte> to_span(const char* data) { return std::span<const std::byte>(reinterpret_cast<const std::byte*>(data), strlen(data) + 1); }
  /*
  std::string to_string( const std::span<std::byte> & data ) {
    return reinterpret_cast<const char*>(data.data());
  }*/

}    // namespace

namespace
{

  template <std::size_t N> class SimpleTask: public os::Static_Task<N>
  {
  protected:
    using func_t = std::function<void()>;
    const func_t func;

  public:
    SimpleTask(const func_t& func_, const char* name)
        : os::Static_Task<N>(name)
        , func(func_)
    {
      os::Static_Task<N>::start();
    }

    void process() override { func(); }
  };

  class SimpleSpanTask: public os::Static_Task<0>
  {
  protected:
    using func_t = std::function<void()>;
    const func_t func;

  public:
    SimpleSpanTask(const func_t& func_, const char* name, const std::span<uint64_t>& stack)
        : Static_Task<0>(stack, name)
        , func(func_)
    {
      Static_Task<0>::start();
    }

    void process() override { func(); }
  };

}    // namespace


class LogTemperature
{
public:
  using analog_values_t = BSP::analog_values_t;

private:

  using this_t                                                                               = LogTemperature;
  mutable os::mutex                                                           m_mtex         = {};
  wlib::publisher::Memberfunction_CallbackSubscriber<this_t, analog_values_t> m_sub          = { *this, &this_t::new_analog_value };
  os::Static_MemberfunctionCallbackTask<this_t, 5120>                         m_worker       = { *this, &this_t::process,    "temperature" };
  os::Static_MemberfunctionCallbackTask<this_t,20120>                         m_worker_timer = { *this, &this_t::on_timeout, "temperature_timer" };
  bslib::container::SPSC<analog_values_t, 2>                                  m_input_buffer = {};
  wlib::container::circular_buffer_t<analog_values_t, 10>                     m_circ_buffer  = {};

  const char *       														  filename;
  std::atomic<bool>                                                           m_enable = false;

public:

  LogTemperature(wlib::publisher::Publisher_Interface<analog_values_t>& analog_value_pup,
                 const char *filename_)
  : filename( filename_ )
  {
    this->m_sub.subscribe(analog_value_pup);
    this->m_worker.start();
    this->m_worker_timer.start();
  };

  auto get_analog_values() const -> analog_values_t
  {
    os::lock_guard  l{ this->m_mtex };
    analog_values_t ret;
    for (analog_values_t const& el : this->m_circ_buffer)
    {
      ret += el;
    }
    return ret;
  }

  auto print(wlib::StringSink_Interface& sink) const -> void
  {
    auto tmp = this->get_analog_values();
    char buf[2048]{};
    snprintf(buf, 2048,
             "      NTC_max: %10.5f,       NTC_min: %10.5f,       NTC_mean: %10.5f,       NTC_var: %10.9E\n"
             "NTC-BOARD_max: %10.5f, NTC-BOARD_min: %10.5f, NTC-BOARD_mean: %10.5f, NTC-BOARD_var: %10.9E\n"
             "      CPU_max: %10.5f,       CPU_min: %10.5f,       CPU_mean: %10.5f,       CPU_var: %10.9E\n"
             "\n"
             "        i_max: %10.5f,         i_min: %10.5f,         i_mean: %10.5f,         i_var: %10.9E\n"
             "       u1_max: %10.5f,        u1_min: %10.5f,        u1_mean: %10.5f,        u1_var: %10.9E\n"
             "       u2_max: %10.5f,        u2_min: %10.5f,        u2_mean: %10.5f,        u2_var: %10.9E\n"
             "\n"
             "      AIR_max: %10.5f,       AIR_min: %10.5f,       AIR_mean: %10.5f,       AIR_var: %10.9E\n"
             "     VREF_max: %10.5f,      VREF_min: %10.5f,      VREF_mean: %10.5f,      VREF_var: %10.9E\n",
             tmp.ambient_temperature.get_max(), tmp.ambient_temperature.get_min(), tmp.ambient_temperature.get_mean(),
             tmp.ambient_temperature.get_variance(),                                                                                                      //
             tmp.board_temperature.get_max(), tmp.board_temperature.get_min(), tmp.board_temperature.get_mean(), tmp.board_temperature.get_variance(),    //
             tmp.cpu_temperature.get_max(), tmp.cpu_temperature.get_min(), tmp.cpu_temperature.get_mean(), tmp.cpu_temperature.get_variance(),            //
             tmp.current.get_max(), tmp.current.get_min(), tmp.current.get_mean(), tmp.current.get_variance(),                                            //
             tmp.voltage_stage_1.get_max(), tmp.voltage_stage_1.get_min(), tmp.voltage_stage_1.get_mean(), tmp.voltage_stage_1.get_variance(),            //
             tmp.voltage_stage_2.get_max(), tmp.voltage_stage_2.get_min(), tmp.voltage_stage_2.get_mean(), tmp.voltage_stage_2.get_variance(),            //
             tmp.air_pressure.get_max(), tmp.air_pressure.get_min(), tmp.air_pressure.get_mean(), tmp.air_pressure.get_variance(),                        //
             tmp.ref_voltage.get_max(), tmp.ref_voltage.get_min(), tmp.ref_voltage.get_mean(), tmp.ref_voltage.get_variance()                             //
    );
    sink(buf);
  }

  bool log_temp()
  {
	  auto tmp = this->get_analog_values();
	  auto file_handle = H7TwoFace::open( filename,std::ios_base::in | std::ios_base::out );

	  if( !file_handle ) {
		  CPPDEBUG( static_format<100>("cannot open file '%s' for reading/wrinting", filename ) );
		  return false;
	  }

	  SimpleFlashFs::StaticFileBuffer<1024> file_buffer( *file_handle );
	  SimpleFlashFs::SimpleIni<100>         ini(file_buffer);

	  const char *SECTION_GLOBAL    = "global";
	  const char *SECTION_CPU_TEMP  = "CPU Temperature";

	  const char *KEY_CURRENT_IDX   = "current_idx";
	  const char *KEY_GLOBAL_WRITES = "global_writes";
	  const char *KEY_CPU_MAX       = "CPU_max";

	  int32_t current_idx = -1;
	  ini.read( SECTION_GLOBAL, KEY_CURRENT_IDX,   current_idx );

	  int32_t global_writes = -1;
	  ini.read( SECTION_GLOBAL, KEY_GLOBAL_WRITES, global_writes );

	  static_assert( sizeof(int) == sizeof(int32_t) );

	  if( current_idx < 0 ) {
		  current_idx = 0;
	  } else if( current_idx > 100 ) {
		  current_idx = 0;
	  }

	  current_idx++;
	  global_writes++;

	  CPPDEBUG( static_format<100>("current_idx: %d global_writes: %d", current_idx, global_writes ) );

	  if( !ini.write( SECTION_CPU_TEMP, static_format<100>("%s%d", KEY_CPU_MAX, current_idx),  tmp.cpu_temperature.get_max() ) ) {
		  CPPDEBUG( "cannot write CPU_max" );
		  return false;
	  }

	  if( !ini.write( SECTION_GLOBAL, KEY_CURRENT_IDX, current_idx ) ) {
		  CPPDEBUG( "cannot write current_idx" );
		  return false;
	  }

	  if( !ini.write( SECTION_GLOBAL, KEY_GLOBAL_WRITES, global_writes ) ) {
		  CPPDEBUG( "cannot write global_writes" );
		  return false;
	  }

	  return true;
  }

  void set_enable( bool enable_logging = true ) {
	  m_enable = enable_logging;
  }

private:
  void new_analog_value(BSP::analog_values_t const& values)
  {
    this->m_input_buffer.push_back(values);
    this->m_worker.notify();
  }

  void process()
  {
    while (os::this_thread::keep_running())
    {
      bool const     timeout = os::this_thread::try_wait_for_notify_for(std::chrono::milliseconds(300)) == 0;
      os::lock_guard l{ this->m_mtex };
      if (!timeout)
      {
        std::optional<analog_values_t> tmp = this->m_input_buffer.pop_front();
        while (tmp.has_value())
        {
          this->m_circ_buffer.push(tmp.value());
          tmp = this->m_input_buffer.pop_front();
        }
      }
      else
      {
        this->m_circ_buffer.clear();
      }
    }
  }

  void on_timeout()
  {
	  while (os::this_thread::keep_running())
	  {
		  if( m_enable ) {
			  if( !log_temp() ) {
				  m_enable = false;
				  break;
			  }
		  }

		  os::this_thread::try_wait_for_notify_for(std::chrono::seconds(5));
	  }
  }

};

bool cmd_info(wlib::StringSink_Interface& sink, std::string_view param)
{
  if (param.length() != 0)
    return false;
  char buf[1024] = {};
  snprintf(buf, 1024, "NUCLEO-H753ZI-FlashTest_uC: FW:%s\n", "0.0.0.0");
  sink(buf);
  return true;
}

class StackAnalyzer
{
public:
	void print( wlib::StringSink_Interface& sink) const;
};

analog_value_logger* ANALOG_VALUE_LOGGER = nullptr;
LogTemperature*      TEMPERATURE_LOGGER = nullptr;
StackAnalyzer *STACK_ANALYZER = nullptr;

bool cmd_status(wlib::StringSink_Interface& sink, std::string_view param)
{
  if (param.length() != 0)
    return false;

  while( ANALOG_VALUE_LOGGER == nullptr || STACK_ANALYZER == nullptr ) {
	os::this_thread::sleep_for( std::chrono::milliseconds(100) );
  }

  ANALOG_VALUE_LOGGER->print(sink);
  sink("\n");

  STACK_ANALYZER->print(sink);
  sink("\n");
  return true;
}

bool cmd_log_temp(wlib::StringSink_Interface& sink, std::string_view param)
{
	while( TEMPERATURE_LOGGER == nullptr ) {
		os::this_thread::sleep_for( std::chrono::milliseconds(100) );
	}

	if( param.empty() ) {
		TEMPERATURE_LOGGER->log_temp();
		return true;
	}

	if( param == "enable" ) {
		TEMPERATURE_LOGGER->set_enable( true );
		return true;
	}

	if( param == "disable" ) {
		TEMPERATURE_LOGGER->set_enable( false );
		return true;
	}

	return false;
}

// USB UART reader

bslib::publisher::LF_Publisher<char, 5> usb_uart_input;

void task_usb_uart_listener()
{
  while (os::this_thread::keep_running())
  {
    auto o_ch = BSP::usb_uart_get_char();

    if (o_ch)
    {
      if( *o_ch ==  static_cast<char>(EOF) ) {
    	  break;
      }

      usb_uart_input.notify(*o_ch);
    }
  }
}

bool cmd_fs(wlib::StringSink_Interface& sink, std::string_view param)
{
	if( param.empty() ) {
		sink( "sub commands are:\n" );
		sink( "\tstatus\n" );
		sink( "\tinit\n" );
		sink( "\tls\n" );
		sink( "\tcat FILENAME\n" );
		return true;
	}

	if( param == "ls" ) {
		auto files = H7TwoFace::list_files();
		for( auto file : files ) {
			sink( file.data(), file.size() );
			sink("\n");
		}
		return true;
	}

	if( param == "status" ) {
	    H7TwoFace::Stat stats = H7TwoFace::get_stat();
	    sink(static_format<200>("\n"
	                            "\t max_number_of_files: % 6d\n"
	                            "\t max_file_size:       % 6dB (%dKB)\n"
	                            "\t largest_file_size:   % 6dB (%dKB)\n"
	                            "\t free_space:          % 6dB (%dKB)\n"
	                            "\t free_inodes:         % 6d"
	    						"\n",
	                            stats.max_number_of_files, stats.max_file_size, stats.max_file_size / 1024, stats.largest_file_size,
	                            stats.largest_file_size / 1024, stats.free_space, stats.free_space / 1024, stats.free_inodes).c_str());
	    return true;
	}

	if( param == "init" ) {
		return H7TwoFace::recreate();
	}

	auto param_args = Tools::split_and_strip_simple_custom<static_vector<std::string_view,10>,std::string_view>(param, " \t\r\n", 10);
	const std::string_view & first_param = *param_args.begin();

	if( first_param == "cat" && param_args.size() > 1 ) {
		bool ret = true;

		for( unsigned i = 1; i < param_args.size(); ++i ) {
			const std::string_view & file_name = param_args[i];
			auto file = H7TwoFace::open( file_name, std::ios_base::in );

			if( !file ) {
				sink( static_format<100>("File '%s' not found\n", file_name).c_str() );
				ret = false;
				continue;
			}

			std::array<std::byte,100> read_buffer;

			while( true ) {
				std::size_t data_read = file->read(read_buffer.data(), read_buffer.size());

				if( data_read == 0 ) {
					break;
				}

				sink( reinterpret_cast<char*>( read_buffer.data() ), data_read );
			}
		}

		return ret;
	}


	return false;
}

bool application_quit = false;

#ifdef SIMULATOR
bool cmd_quit(bslib::StringSink_Interface& sink, std::string_view param)
{
	CPPDEBUG( "quitting application");
	application_quit = true;
	return true;
}
#endif

#ifdef _MSC_VER
#    define __attribute__(x)
#endif

std::array<uint64_t,  1 * 1024 / sizeof(uint64_t)> __attribute__((section(".reserved_for_stack")))       stack_status_led;
std::array<uint64_t,  4 * 1024 / sizeof(uint64_t)> __attribute__((section(".reserved_for_stack")))       stack_usb_uart_reader;
std::array<uint64_t, 40 * 1024 / sizeof(uint64_t)> __attribute__((section(".reserved_for_stack")))       stack_cmd_parser;
std::array<uint64_t, 40 * 1024 / sizeof(uint64_t)> /*__attribute__((section(".reserved_for_stack")))*/ stack_main_array;
std::span<uint64_t>                                                                                     stack_main(stack_main_array);


#ifdef _MSC_VER
#    undef __attribute__
#endif

void StackAnalyzer::print( wlib::StringSink_Interface& sink) const
{
  auto analyze = [&sink](const std::span<uint64_t>& span, const char* name)
  {
    unsigned long  unused_bytes = 0;
    const uint64_t fill_byte    = 0xa5a5a5a5a5a5a5a5;    
#ifdef _WIN32
    // libco stores the stack pointer at the first index
    const unsigned START_INDEX = 4;
#else
    const unsigned START_INDEX = 0;
#endif

    for (unsigned i = START_INDEX; i < span.size() && span[i] == fill_byte; i++)
    {
      unused_bytes += sizeof(uint64_t);
    }

    const unsigned used_bytes = (span.size_bytes() - unused_bytes);
    unsigned       perc       = 100.0 / span.size_bytes() * used_bytes;

    const char* unit = "kB";
    unsigned    div  = 1024;

    if (span.size_bytes() / 1024 <= 0)
    {
      unit = "B ";
      div  = 1;
    }

    sink(static_format<100>("%s size: % 5d%s in use: % 5d%s usage: % 3d%%\n",
    		name, span.size_bytes() / div, unit, used_bytes / div, unit, perc).c_str());
  };


    analyze(stack_main, 			"main           ");
    analyze(stack_status_led, 		"Status LED     ");
    analyze(stack_usb_uart_reader, 	"USB UART Reader");
    analyze(stack_cmd_parser, 		"CMD Parser     ");
    sink("\n");
}

static void test_y2038()
{
	static_assert( sizeof(time_t) == sizeof(int64_t) );

	struct tm t_decomp;

	t_decomp.tm_year = 139; //2039
	t_decomp.tm_mon = 0;
	t_decomp.tm_mday = 1;
	t_decomp.tm_hour = 8;
	t_decomp.tm_min = 37;
	t_decomp.tm_sec = 53;
	t_decomp.tm_isdst = -1;

	time_t tv_sec = mktime(&t_decomp);

	if( tv_sec < 0 ) {
		throw std::runtime_error( "Year 2038 problem detected" );
	}
}

serial_nr_RPC_adapter pcb_sn;

int main()
{
#ifndef NDEBUG
  static CppUtilsUartDebug debug_uart(true);
  Tools::x_debug = &debug_uart;
#endif
  auto& sink = BSP::get_usb_uart_output_debug();

  test_y2038();

  BSP::init_internal_fs();
  H7TwoFace::set_lock_unlock_callback( []( bool lock ) {
	  static os::mutex m_lock{};

	  if( lock ) {
		  m_lock.lock();
	  } else {
		  m_lock.unlock();
	  }
  });

  static StackAnalyzer stack_analyzer;
  STACK_ANALYZER = &stack_analyzer;

  static SimpleSpanTask task_led(task_status_led, "Status LED", stack_status_led);
  static SimpleSpanTask task_usb_uart_reader(task_usb_uart_listener, "USB UART Reader", stack_usb_uart_reader);

  static wlib::Function_Callback<app::Serial_Commando_Parser::CMD::callback_t::signature_t> cmd_cb_info   = { cmd_info };
  static wlib::Function_Callback<app::Serial_Commando_Parser::CMD::callback_t::signature_t> cmd_cb_status = { cmd_status };
  static wlib::Function_Callback<app::Serial_Commando_Parser::CMD::callback_t::signature_t> cmd_cb_fs = { cmd_fs };
  static wlib::Function_Callback<app::Serial_Commando_Parser::CMD::callback_t::signature_t> cmd_cb_log_temp = { cmd_log_temp };
#ifdef SIMULATOR
  static wlib::Function_Callback<app::Serial_Commando_Parser::CMD::callback_t::signature_t> cmd_cb_quit = { cmd_quit };
#endif

  static char            line_buffer_parser[1024] = {};
  static app::Serial_Commando_Parser::CMD cmds_parser[]            = {
    { "info",     "shows the device info",   cmd_cb_info },
    { "status",   "shows the device status", cmd_cb_status },
	{ "fs", 	  "filesystem operations",   cmd_cb_fs },
	{ "log_temp", "[enable,disable] log temperature to file", cmd_cb_log_temp },
#ifdef SIMULATOR
	{ "quit", 	  "quit simulator",          cmd_cb_quit },
#endif
  };

  static app::Serial_Commando_Parser::Parser<0> parser(usb_uart_input, line_buffer_parser, cmds_parser, sink, stack_cmd_parser );

  static analog_value_logger anal_logger{ BSP::get_analog_value_publisher(), sink };
  ANALOG_VALUE_LOGGER = &anal_logger;

  static LogTemperature temperature_logger{ BSP::get_analog_value_publisher(), "cpu.ini" };
  TEMPERATURE_LOGGER = &temperature_logger;


  using CANOpen             = canopen::CanOpen_Interface;
  canopen::CanOpenSlave can = { BSP::get_canopen_HWUnit_1(), CANOpen::NodeId{ 0x48 }, CANOpen::Baudrate::_1000kb, canopen::config::get_config_1(),
                                BSP::get_can_identity_object_1() };

  can.config_heartbeat_producer(std::chrono::milliseconds(1000));
  can.register_updater(BSP::get_canopen_updater());

  AirPumpModule 	air_pump_module;
  FunnelModule  	funnel_module;
  SimpleFillModule	simplefill_module;

  app::FunnelModule_CANOpen_Adapter     funnel_module_can_adapter{ funnel_module, can, sink };
  app::SimpleFill_CANOpen_Adapter       simplefill_can_adapter{ simplefill_module, can, sink };
  app::AirPump_CANOpen_Adapter       	air_pump_can_adapter{ air_pump_module, can, sink };

  canopen::RPC_Slave<20>             rpc_slave;
  rpc_slave.add_rpc(canopen::RPC_Id(canopen::instance1::OsCommands::GetPcbSerialNr::Id), pcb_sn.getter());

  rpc_slave.add_rpc(canopen::RPC_Id(canopen::instance1::OsCommands::FunnelSetTargetTemperature::Id), funnel_module_can_adapter.set_target_temperature_rpc());
  rpc_slave.add_rpc(canopen::RPC_Id(canopen::instance1::OsCommands::FunnelChangeState::Id), funnel_module_can_adapter.change_state_rpc());

  rpc_slave.add_rpc(canopen::RPC_Id(canopen::instance1::OsCommands::SimpleFillSet::Id), simplefill_can_adapter.set_rpc());
  rpc_slave.add_rpc(canopen::RPC_Id(canopen::instance1::OsCommands::SimpleFillChangeState::Id), simplefill_can_adapter.change_state_rpc());

  rpc_slave.add_rpc(canopen::RPC_Id(canopen::instance1::OsCommands::AirPumpSet::Id), air_pump_can_adapter.set_rpc());
  rpc_slave.add_rpc(canopen::RPC_Id(canopen::instance1::OsCommands::AirPumpChangeState::Id), air_pump_can_adapter.change_state_rpc());

  app::UtilityValues_CANOpen_Adapter utility_values_adapter{ anal_logger, can, sink };

  can.set_rpc_slave(rpc_slave);
  can.connect();

  do
  {
    os::this_thread::sleep_for(std::chrono::milliseconds(100));
  } while (!application_quit);

  os::quit(0);

  return 0;
}
