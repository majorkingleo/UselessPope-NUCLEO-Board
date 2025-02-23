/*
 * stm32_internal_flash_raw_h7.cpp
 *
 *  Created on: Sep 5, 2024
 *      Author: martin.oberzalek
 */
#include "stm32_internal_flash_raw_f4.h"

namespace {
	class AutoLockFlash
	{
	public:
		~AutoLockFlash()
		{
			HAL_FLASH_Lock();
		}
	};
}

namespace stm32_internal_flash {

#if defined(STM32F407xx) || defined(STM32F401xE) || defined(STM32F207xx)
std::size_t STM32InternalFlashHalRawF4::write_page( std::size_t address, const std::span<const std::byte> & buffer )
{
	if( HAL_FLASH_Unlock() != HAL_OK) {
		error = Error(Error::ErrorUnlockingFlash);
		return 0;
	}

	AutoLockFlash auto_lock_flash;

	clear_flags();

	auto do_write = [this]( uint32_t TypeProgram, auto data_type, std::size_t address, const std::span<const std::byte> & buffer ) {
		using data_t = decltype(data_type);
		constexpr uint32_t data_step_size = sizeof(data_t);
		std::size_t size_written = 0;
		const uint32_t start_offset = reinterpret_cast<uint32_t>(conf.data_ptr);

		for( uint32_t offset = 0; offset <= buffer.size() - data_step_size; offset += data_step_size ) {

			std::size_t target_address = start_offset + address + offset;
			const data_t *source = reinterpret_cast<const data_t*>(buffer.data() + offset);
			uint64_t aligned_source_data = *source;

			HAL_StatusTypeDef ret = HAL_FLASH_Program(TypeProgram,
					target_address,
					aligned_source_data );

			if( ret != HAL_OK ) {
				error = Error(Error::HAL_Error,HAL_FLASH_GetError());
				return size_written;
			}

			size_written += data_step_size;
		}

		return size_written;
	};

	std::size_t size_written = 0;

	// Double word programming requires an external power supply of 9V
/*
	if( buffer.size() % sizeof(uint64_t) == 0 ) {
		size_written = do_write( FLASH_TYPEPROGRAM_DOUBLEWORD, (uint64_t)1, address, buffer );
	}
*/

	if( buffer.size() % sizeof(uint32_t) == 0 ) {
		size_written = do_write( FLASH_TYPEPROGRAM_WORD,       (uint32_t)1, address, buffer );
	} else if( buffer.size() % sizeof(uint16_t) == 0 ) {
		size_written = do_write( FLASH_TYPEPROGRAM_HALFWORD,   (uint16_t)1, address, buffer );
	} else {
		size_written = do_write( FLASH_TYPEPROGRAM_BYTE,       (uint16_t)1, address, buffer );
	}
	return size_written;
}
#endif

} // namespace stm32_internal_flash
