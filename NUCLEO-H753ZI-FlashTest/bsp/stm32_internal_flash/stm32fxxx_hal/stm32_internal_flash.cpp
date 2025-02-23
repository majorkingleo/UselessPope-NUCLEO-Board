/*
 * @author Copyright (c) 2024 Martin Oberzalek
 */
#include "stm32_internal_flash.h"

namespace stm32_internal_flash {

bool Configuration::check() const
{
	std::optional<std::size_t> size;

	if( used_sectors.empty() ) {
		return false;
	}

	for( const Sector & sec : used_sectors ) {
		if( sec.size == 0 ) {
			return false;
		}

		if( sec.start_address == 0 ) {
			return false;
		}

		if( !size ) {
			size = sec.size;
		} else {
			if( size.value() != sec.size ) {
				return false;
			}
		}
	}

	if( data_ptr == nullptr ) {
		return false;
	}

	return true;
}

} // namespace smt32_internal_flash




