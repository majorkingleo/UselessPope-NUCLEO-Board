#pragma once

#include <CpputilsDebug.h>
#include <chrono>
#include <array>

class CppUtilsUartDebug : public Tools::Debug
{
	struct LogTime
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		std::array<char,50> buffer;

		std::string_view get_time();
	};

  std::optional<LogTime> lt;

public:
  CppUtilsUartDebug( bool enable_time_stamps = false );
  
  void add( const char *file, unsigned line, const char *function, const std::string_view & s ) override;
  void add( const char *file, unsigned line, const char *function, const std::wstring_view & s ) override;
};
