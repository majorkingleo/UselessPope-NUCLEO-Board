#include "CppUtilsUartDebug.hpp"
#include <utf8_util.h>
#include <bsp.hpp>
#include <static_format.h>
#include <static_debug_exception.h>
#include <chrono>
#include "LockedDebugStream.hpp"

using namespace Tools;
using namespace std::chrono_literals;

std::string_view CppUtilsUartDebug::LogTime::get_time()
{
	std::chrono::steady_clock::duration diff = std::chrono::steady_clock::now() - start;

	const uint32_t d_hours = std::chrono::duration_cast<std::chrono::hours>( diff ).count();
	uint32_t d_minutes = std::chrono::duration_cast<std::chrono::minutes>( diff - d_hours * 1h ).count();
	uint32_t d_seconds = std::chrono::duration_cast<std::chrono::seconds>( diff - d_hours * 1h - d_minutes * 1min).count();
	uint32_t d_ms = std::chrono::duration_cast<std::chrono::milliseconds>( diff - d_hours * 1h - d_minutes * 1min - d_seconds *1s).count();

	span_vector<char> vbuffer(buffer);
	Tools::basic_string_adapter<char> ret( vbuffer  );

	ret = std::string_view( static_format<50>( "%02ld:%02ld:%02ld.%04ld", d_hours % 24, d_minutes, d_seconds, d_ms ) );
	return buffer.data();
}


CppUtilsUartDebug::CppUtilsUartDebug( bool enable_time_stamps )
{
	if( enable_time_stamps ) {
		lt.emplace();
	}
}

void CppUtilsUartDebug::add( const char *file, unsigned line, const char *function, const std::string_view & s )
{
  auto sink = LockedDebugStream::instance().get();

  if( lt ) {
	  sink.write("[");
	  sink.write(lt->get_time());
	  sink.write("] ");
  }

  // remove leading directory names
  auto file_name = StaticDebugException::get_file_name( file );
  sink.write( file_name );
  sink.write( ":" );

  sink.write( static_format<20>("%d", line) );

  sink.write( " " );
  sink.write( s );
  sink.write( "\n" );
}

void CppUtilsUartDebug::add( const char *file, unsigned line, const char *function, const std::wstring_view & s )
{
	add( file, line, function, Tools::Utf8Util::wStringToUtf8(std::wstring(s)) );
}
