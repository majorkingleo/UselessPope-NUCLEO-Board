/*
 * stm32_internal_flash_raw_h7.h
 *
 *  Created on: Sep 5, 2024
 *      Author: martin.oberzalek
 */
#pragma once

#include "stm32_internal_flash_raw_h7.h"

namespace stm32_internal_flash {

#if defined(STM32H753xx)
/**
 * Optimized class for writing data.
 * Won't invalidate instruction cache.
 * Don't use this, if you wan't to execute the written data.
 */
class STM32InternalFlashHalRawH74Data : public STM32InternalFlashHalRawH7
{
public:
	STM32InternalFlashHalRawH74Data( Configuration & conf )
	 : STM32InternalFlashHalRawH7( conf )
	{
	}

protected:
	HAL_StatusTypeDef FLASH_Program(uint32_t TypeProgram, uint32_t FlashAddress, uint32_t DataAddress) override;
};
#endif

} // namespace stm32_internal_flash



