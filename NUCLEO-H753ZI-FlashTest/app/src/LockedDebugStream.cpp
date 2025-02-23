/*
 * LockedDebugStream.cpp
 *
 *  Created on: Mar 5, 2024
 *      Author: martin.oberzalek
 */
#include "LockedDebugStream.hpp"
#include "bsp_uart_usb.hpp"

LockedDebugStream & LockedDebugStream::instance()
{
	static std::optional<LockedDebugStream> ds;

	if( !ds ) {
		ds.emplace();
	}

	return *ds;
}

LockedDebugStream::WrappedHandle LockedDebugStream::get()
{
  return WrappedHandle( m_lock, BSP::get_usb_uart_output_debug() );
}

