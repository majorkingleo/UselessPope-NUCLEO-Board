/*
 * Raw driver. Expects that all operations
 * are page aligned and only full pages can be written.
 *
 * This driver is using the HAL library.
 *
 * @author Copyright (c) 2024 Martin Oberzalek
 */
#include "stm32_internal_flash_raw_base.h"
#include <string.h>

namespace stm32_internal_flash {

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

STM32InternalFlashHalRawBase::STM32InternalFlashHalRawBase( Configuration & conf_ )
: conf( conf_ )
{
	conf.init();

	if( !conf ) {
		error = Error( Error::ConfigurationError );
	}
}

bool STM32InternalFlashHalRawBase::erase_page_by_page_startaddress( std::size_t address, std::size_t size )
{
	auto sector = get_sector_from_address( address );

	if( !sector ) {
		error = Error::InvalidSectorAddress;
		return false;
	}

	FLASH_EraseInitTypeDef EraseInitStruct {};
	uint32_t PAGEError = 0;

	EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.Banks     = conf.banks;
	EraseInitStruct.Sector	  = sector->sector;
	EraseInitStruct.NbSectors = size / sector->size;


#if defined(STM32H753xx)
	EraseInitStruct.VoltageRange = VOLTAGE_RANGE_4;
#endif


	if( EraseInitStruct.NbSectors == 0 ) {
		 EraseInitStruct.NbSectors = 1;
	}

	EraseInitStruct.VoltageRange = conf.voltage_range;


	if( HAL_FLASH_Unlock() != HAL_OK) {
		error = Error(Error::ErrorUnlockingFlash);
		return false;
	}

	AutoLockFlash auto_lock_flash;

	clear_flags();

	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK) {
		error = Error(Error::ErrorErasingFlash);
		return false;
	}

	return true;
}

void STM32InternalFlashHalRawBase::clear_flags()
{
	uint32_t flags = 		   FLASH_FLAG_EOP |
				               FLASH_FLAG_OPERR |
							   FLASH_FLAG_WRPERR |
	#ifdef FLASH_FLAG_PGAERR
							   FLASH_FLAG_PGAERR |
	#endif
							   FLASH_FLAG_PGSERR |

	#ifdef FLASH_FLAG_PGPERR
							   FLASH_FLAG_PGPERR |
	#endif
	#ifdef FLASH_SR_RDERR
							   FLASH_SR_RDERR |
	#endif
							   FLASH_SR_BSY;


	__HAL_FLASH_CLEAR_FLAG( flags );
}

std::optional<Configuration::Sector> STM32InternalFlashHalRawBase::get_sector_from_address( std::size_t address ) const
{
	for( const Configuration::Sector & sec : conf.used_sectors ) {
		if( address == sec.start_address ) {
			return sec;
		}
	}

	return {};
}


std::size_t STM32InternalFlashHalRawBase::read_page( std::size_t address, std::span<std::byte> & buffer )
{
	std::size_t data_size = std::min( buffer.size(), conf.size );

	std::byte* source_address = conf.data_ptr + address;

	memcpy( buffer.data(), source_address, data_size );

	return data_size;
}

std::size_t STM32InternalFlashHalRawBase::get_page_size()
{
	return conf.used_sectors.begin()->size;
}

bool STM32InternalFlashHalRawBase::erase_page( std::size_t address, std::size_t size )
{
	const uint32_t start_offset = reinterpret_cast<uint32_t>(conf.data_ptr);
	std::size_t target_address = start_offset + address;
	return erase_page_by_page_startaddress(target_address, size );
}

} // namespace smt32_internal_flash


