/*
 * Raw driver. Expects that all operations
 * are page aligned and only full pages can be written.
 *
 * This driver is using the HAL library.
 *
 * FOR STM32F4xxx processors:
 * No HAL_Init() is required, no SystemClock_Config() or SystemInit()
 * The driver and all function are working out of the box.
 * SystemClock_Config() would be required if calls should be able to run into a timeout
 *
 * @author Copyright (c) 2024 Martin Oberzalek
 */

#pragma once

#include "stm32_internal_flash.h"
#include "RawDriverInterface.h"

namespace stm32_internal_flash {

class STM32InternalFlashHalRawBase : public RawDriverInterface
{
protected:
	Configuration & conf;
	std::optional<Error> error;

public:
	STM32InternalFlashHalRawBase( Configuration & conf );

	bool operator!() const {
		return error.has_value();
	}

	std::size_t get_size() override {
		return conf.size;
	}

	/**
	 * erases on or more pages
	 * size has to be page size
	 */
	bool erase_page_by_page_startaddress( std::size_t page_start_address, std::size_t size );

	const std::optional<Error> & get_error() const {
		return error;
	}

	void clear_error() {
		error = {};
	}

	/**
	 * reads from flash, it's just a memcpy, since reading goes via internal addresses, so
	 * start address has not to be page aligned and
	 * buffer size can be smaller or larger than a page.
	 */
	std::size_t read_page( std::size_t address, std::span<std::byte> & buffer )  override;

	std::size_t get_page_size() override;

	/**
	 * Erases at least one page. size has to be a multiple of PAGE_SIZE
	 */
	bool erase_page( std::size_t address, std::size_t size ) override;

	const std::byte* map_read( std::size_t address, std::size_t size ) {
		return conf.data_ptr + address;
	}

protected:
	std::optional<Configuration::Sector> get_sector_from_address( std::size_t address ) const;
	void clear_flags();
};

} // namespace smt32_internal_flash

