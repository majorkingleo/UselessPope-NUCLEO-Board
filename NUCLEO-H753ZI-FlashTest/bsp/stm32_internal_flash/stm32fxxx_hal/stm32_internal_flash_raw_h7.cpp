/*
 * stm32_internal_flash_raw_h7.cpp
 *
 *  Created on: Sep 5, 2024
 *      Author: martin.oberzalek
 */
#include "stm32_internal_flash_raw_h7.h"
#include <CpputilsDebug.h>
#include <static_format.h>

using namespace Tools;

namespace {
	class AutoLockFlash
	{
	public:
		~AutoLockFlash()
		{
			HAL_FLASH_Lock();
		}
	};

#ifndef NDEBUG
	void debug_hal_error( stm32_internal_flash::Error error)
	{
		auto dbg = []( uint32_t error, uint32_t flag, const char *message, const char *flag_name  ) {
			if( (error & flag) == flag ) {
				CPPDEBUG( static_format<100>( "HalError: 0x%X flag 0x%X (%s)", error, flag, message ) );
			}
		};

#ifdef DBG_FLASH_ERROR
# error "Macro already defined"
#endif

#define DBG_FLASH_ERROR( def, message ) \
	dbg( error.hal_error, def, message, #def )

		DBG_FLASH_ERROR( HAL_FLASH_ERROR_WRP_BANK1, 	"Write Protection Error on Bank 1" );
		DBG_FLASH_ERROR( HAL_FLASH_ERROR_PGS_BANK1, 	"Program Sequence Error on Bank 1" );
		/*
		DBG_FLASH_ERROR(_FLASH_ERROR_STRB_BANK1, 		"Strobe Error on Bank 1" );
		DBG_FLASH_ERROR(_FLASH_ERROR_INC_BANK1, 		"Inconsistency Error on Bank 1" );
		DBG_FLASH_ERROR(_FLASH_ERROR_OPE_BANK1, 		"Operation Error on Bank 1" );
		DBG_FLASH_ERROR(_FLASH_ERROR_RDP_BANK1, 		"Read Protection Error on Bank 1" );
		*/
		DBG_FLASH_ERROR( HAL_FLASH_ERROR_RDS_BANK1, 	"Read Secured Error on Bank 1" );
		DBG_FLASH_ERROR( HAL_FLASH_ERROR_SNECC_BANK1, 	"ECC Single Correction Error on Bank 1" );
		DBG_FLASH_ERROR( HAL_FLASH_ERROR_DBECC_BANK1, 	"ECC Double Detection Error on Bank 1" );
		DBG_FLASH_ERROR( HAL_FLASH_ERROR_CRCRD_BANK1, 	"CRC Read Error on Bank 1" );

		DBG_FLASH_ERROR( HAL_FLASH_ERROR_WRP_BANK2, 	"Write Protection Error on Bank 2" );
		DBG_FLASH_ERROR( HAL_FLASH_ERROR_PGS_BANK2, 	"Program Sequence Error on Bank 2" );
		DBG_FLASH_ERROR( HAL_FLASH_ERROR_STRB_BANK2, 	"Strobe Error on Bank 2" );
		DBG_FLASH_ERROR( HAL_FLASH_ERROR_INC_BANK2, 	"Inconsistency Error on Bank 2" );
		DBG_FLASH_ERROR( HAL_FLASH_ERROR_OPE_BANK2, 	"Operation Error on Bank 2" );
		DBG_FLASH_ERROR( HAL_FLASH_ERROR_RDP_BANK2, 	"Read Protection Error on Bank 2" );
		DBG_FLASH_ERROR( HAL_FLASH_ERROR_RDS_BANK2, 	"Read Secured Error on Bank 2" );
		DBG_FLASH_ERROR( HAL_FLASH_ERROR_SNECC_BANK2, 	"SNECC Error on Bank 2" );
		DBG_FLASH_ERROR(HAL_FLASH_ERROR_DBECC_BANK2, 	"Double Detection ECC on Bank 2" );
		DBG_FLASH_ERROR( HAL_FLASH_ERROR_CRCRD_BANK2, 	"CRC Read Error on Bank 2" );

#undef DBG_FLASH_ERROR
	}
#endif
}

namespace stm32_internal_flash {

#if defined(STM32H753xx)

std::size_t STM32InternalFlashHalRawH7::write_page( std::size_t address, const std::span<const std::byte> & buffer )
{
	if( HAL_FLASH_Unlock() != HAL_OK) {
		error = Error(Error::ErrorUnlockingFlash);
		return 0;
	}

	AutoLockFlash auto_lock_flash;

	clear_flags();

	auto do_write = [this]( uint32_t TypeProgram, std::size_t address, const std::span<const std::byte> & buffer ) {
		constexpr uint32_t data_step_size = 8 * sizeof(uint32_t);
		std::size_t size_written = 0;
		const uint32_t start_offset = reinterpret_cast<uint32_t>(conf.data_ptr);

		for( uint32_t offset = 0; offset <= buffer.size() - data_step_size; offset += data_step_size ) {

			std::size_t target_address = start_offset + address + offset;
			const uint64_t *source = reinterpret_cast<const uint64_t*>(buffer.data() + offset);
			//const char *sc = (char*)source;
			//uint64_t aligned_source_data = *source;


			HAL_StatusTypeDef ret = FLASH_Program(TypeProgram,
					target_address,
					(uint32_t)source );

			if( ret != HAL_OK ) {
				error = Error(Error::HAL_Error,HAL_FLASH_GetError());

#ifndef NDEBUG
				debug_hal_error(*error);
				CPPDEBUG( static_format<100>( "cannot write %d bytes at address: 0x%X", data_step_size, target_address ) );
				CPPDEBUG( static_format<100>( "data there1: 0x%X", *(((uint32_t*)target_address+0) )) );
				CPPDEBUG( static_format<100>( "data there2: 0x%X", *(((uint32_t*)target_address+1) )) );
				CPPDEBUG( static_format<100>( "data there3: 0x%X", *(((uint32_t*)target_address+2) )) );
				CPPDEBUG( static_format<100>( "data there4: 0x%X", *(((uint32_t*)target_address+3) )) );
				CPPDEBUG( static_format<100>( "data there5: 0x%X", *(((uint32_t*)target_address+4) )) );
				CPPDEBUG( static_format<100>( "data there6: 0x%X", *(((uint32_t*)target_address+5) )) );
				CPPDEBUG( static_format<100>( "data there7: 0x%X", *(((uint32_t*)target_address+6) )) );
				CPPDEBUG( static_format<100>( "data there8: 0x%X", *(((uint32_t*)target_address+7) )) );
#endif

				return size_written;
			}

			size_written += data_step_size;
		}

		return size_written;
	};

	std::size_t size_written = 0;

	if( buffer.size() % 256 == 0 ) {
		size_written = do_write( FLASH_TYPEPROGRAM_FLASHWORD, address, buffer );
	} else {
		error = Error(Error::ErrorUnalignedData);
		return 0;
	}

	return size_written;
}

HAL_StatusTypeDef STM32InternalFlashHalRawH7::FLASH_Program(uint32_t TypeProgram, uint32_t FlashAddress, uint32_t DataAddress)
{
	return HAL_FLASH_Program( TypeProgram, FlashAddress, DataAddress );
}

std::size_t STM32InternalFlashHalRawH7::read_page( std::size_t address, std::span<std::byte> & buffer )
{
	if((IS_FLASH_PROGRAM_ADDRESS_BANK1(address))) {
		HAL_StatusTypeDef hal_ret = FLASH_WaitForLastOperation(1000, FLASH_BANK_1);
		if( hal_ret != HAL_OK ) {
			error = Error(Error::HAL_Error,HAL_FLASH_GetError());
#ifndef NDEBUG
			debug_hal_error(*error);
#endif
			return 0;
		}
	}

	else if((IS_FLASH_PROGRAM_ADDRESS_BANK2(address))) {
		HAL_StatusTypeDef hal_ret = FLASH_WaitForLastOperation(1000, FLASH_BANK_2);
		if( hal_ret != HAL_OK ) {
			error = Error(Error::HAL_Error,HAL_FLASH_GetError());
#ifndef NDEBUG
			debug_hal_error(*error);
#endif
			return 0;
		}
	}

	std::size_t data_size = std::min( buffer.size(), conf.size );

	std::byte* source_address = conf.data_ptr + address;

	if((IS_FLASH_PROGRAM_ADDRESS_BANK1((uint32_t)source_address))) {
		if( FLASH->SR1 ) {
			CPPDEBUG( static_format<100>("Error at FLASH->SR1: 0x%X", FLASH->SR1 ) );
			return 0;
		}
	}

	if((IS_FLASH_PROGRAM_ADDRESS_BANK2((uint32_t)source_address))) {
		if( FLASH->SR2 ) {
			CPPDEBUG( static_format<100>("Error at FLASH->SR2: 0x%X", FLASH->SR2 ) );
			return 0;
		}
	}

	memcpy( buffer.data(), source_address, data_size );

	if((IS_FLASH_PROGRAM_ADDRESS_BANK1((uint32_t)source_address))) {
		if( FLASH->SR1 ) {
			CPPDEBUG( static_format<100>("Error at FLASH->SR1: 0x%X", FLASH->SR1 ) );
			return 0;
		}
	}

	if((IS_FLASH_PROGRAM_ADDRESS_BANK2((uint32_t)source_address))) {
		if( FLASH->SR2 ) {
			CPPDEBUG( static_format<100>("Error at FLASH->SR2: 0x%X", FLASH->SR2 ) );
			return 0;
		}
	}

	return data_size;
}

#endif

} // namespace stm32_internal_flash {
