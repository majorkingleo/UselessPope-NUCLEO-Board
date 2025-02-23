/*
 * stm32_internal_flash_raw_h7.h
 *
 *  Created on: Sep 5, 2024
 *      Author: martin.oberzalek
 */
#pragma once

#include "stm32_internal_flash_raw_base.h"

namespace stm32_internal_flash {
#if defined(STM32F407xx) || defined(STM32F401xE) || defined(STM32F207xx)

class STM32InternalFlashHalRawF4 : public STM32InternalFlashHalRawBase
{
public:
	STM32InternalFlashHalRawF4( Configuration & conf )
	 : STM32InternalFlashHalRawBase( conf )
	{
	}

	/*
	 * Writes a page to flash, the memory has to be erased before.
	 * The function is not doing this automatically.
	 * After a write operation HAL driver is flushing the caches, of course there can be program code located.
	 *
	 * Buffer: can be smaller then page size.
	 */
	std::size_t write_page( std::size_t address, const std::span<const std::byte> & buffer ) override;
};

#endif
} // namespace stm32_internal_flash



