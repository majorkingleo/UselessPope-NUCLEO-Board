/*
 * Configuration structures for the raw flash driver.
 *
 * @author Copyright (c) 2024 Martin Oberzalek
 */

#ifndef APP_STM32_INTERNAL_FLASH_STM32FXXX_HAL_STM32_INTERNAL_FLASH_H_
#define APP_STM32_INTERNAL_FLASH_STM32FXXX_HAL_STM32_INTERNAL_FLASH_H_

#include <optional>
#include <span>
#include <stdint.h>

#if defined(STM32F407xx) || defined(STM32F401xE)
#include <stm32f4xx_hal.h>
#endif

#if defined(STM32F207xx)
#include <stm32f2xx_hal.h>
#endif

#if defined(STM32H753xx)
#include <stm32h7xx_hal.h>
#endif

namespace stm32_internal_flash {

struct Configuration
{
	struct Sector
	{
		uint32_t sector{};
		uint32_t size{};
		std::size_t start_address = 0;
	};

	std::span<const Sector> used_sectors{};
	std::byte* data_ptr = nullptr; // if null start_addess of forst sector is used.

	uint32_t voltage_range = VOLTAGE_RANGE_3;
	uint32_t banks = FLASH_BANK_1;

	std::size_t size = 0; // full size, calculated

	void init() {

		// calc size
		for( const Sector & sector : used_sectors ) {
			size += sector.size;
		}

		if( !data_ptr ) {
			data_ptr = reinterpret_cast<std::byte*>(used_sectors[0].start_address);
		}
	}

	bool check() const;

	bool operator!() const {
		return !check();
	}
};

class Error
{
public:
	enum ErrorCode
	{
		ConfigurationError,
		InvalidSectorAddress,
		ErrorUnlockingFlash,
		ErrorErasingFlash,
		ErrorUnalignedData,
		HAL_Error
	};

	ErrorCode driver_error;
	uint32_t hal_error{};
public:

	Error( ErrorCode driver_error_, uint32_t hal_error_ = 0 )
	: driver_error( driver_error_ ),
	  hal_error(hal_error_)
	{}
};

} // namespace namespace smt32_internal_flash

#endif /* APP_STM32_INTERNAL_FLASH_STM32F4XX_HAL_STM32_INTERNAL_FLASH_H_ */
