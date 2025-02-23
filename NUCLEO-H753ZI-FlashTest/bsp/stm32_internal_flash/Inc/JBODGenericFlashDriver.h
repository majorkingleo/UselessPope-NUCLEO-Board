/*
 * This flash driver can handle multiple underlying
 * GenericFlashDrivers
 *
 * This way you can manage flash storages with different page sizes
 *
 * @author Copyright (c) 2024 Martin Oberzalek
 */

#ifndef DRIVERS_STM32_INTERNAL_FLASH_INC_JBODGENERICFLASHDRIVER_H_
#define DRIVERS_STM32_INTERNAL_FLASH_INC_JBODGENERICFLASHDRIVER_H_

#include "MemoryInterface.h"

namespace stm32_internal_flash {

class JBODGenericFlashDriver : public MemoryInterface
{
	struct DriverInfo {
		MemoryInterface *driver         = nullptr;
		unsigned         driver_idx     = 0;
		std::size_t      address_offset = 0;

		bool operator!() const {
			return driver == nullptr;
		}
	};

protected:
	const std::span<MemoryInterface*> & drivers;

public:

	JBODGenericFlashDriver( const std::span<MemoryInterface*> & drivers_ )
	: drivers( drivers_ )
	{}

	std::size_t get_size() const override;

	/**
	 * returns the largest page size of the bunch of discs
	 */
	std::size_t get_page_size() const override;

	/**
	 * writes data, aligned or unaligned. Erase is called automatically before
	 */
	std::size_t write( std::size_t address, const std::span<const std::byte> & data ) override;
	std::size_t read( std::size_t address, std::span<std::byte> & data ) override;

	bool erase( std::size_t address, std::size_t size ) override;

private:
	MemoryInterface* get_driver_by_address( std::size_t address ) const;

	/**
	 * returns -1 if out of range
	 */
	DriverInfo get_driver_idx_by_address( std::size_t address ) const;


	template<class SPAN, class FUNC>
	std::size_t read_write( std::size_t address, SPAN & data, FUNC func );

	void properties_changed() override;
};


} // namespace stm32_internal_flash

#endif /* DRIVERS_STM32_INTERNAL_FLASH_INC_JBODGENERICFLASHDRIVER_H_ */
