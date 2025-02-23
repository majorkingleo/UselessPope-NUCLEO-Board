/*
 * Exposed MemoryInterface of this driver
 * @author Copyright (c) 2024 Martin Oberzalek
 */

#ifndef APP_STM32_INTERNAL_FLASH_INC_MEMORYINTERFACE_H_
#define APP_STM32_INTERNAL_FLASH_INC_MEMORYINTERFACE_H_

#include <cstddef>
#include <span>
#include <type_traits>
#include <tuple>
#include <variant>
#include <functional>
#include <string.h>
#include <concepts>

namespace stm32_internal_flash {

namespace PropertyTypes {

template<class Value> struct PropertyValue
{
protected:
	using type_t = Value;
	type_t value{};
	std::function<void()> property_changed_func = [](){};

public:
	operator Value() const {
		return value;
	}

	Value get() const {
		return value;
	}

	PropertyValue & operator=( const Value & value_ ) {
		value = value_;
		property_changed_func();
		return *this;
	}

	PropertyValue & operator=( const PropertyValue & other ) {
		value = other.value;
		// don't copy change function
		property_changed_func();
		return *this;
	}

	void set( const Value & value_ ) {
		value = value_;
		property_changed_func();
	}

	void set_property_changed_func( std::function<void()> property_changed_func_ ) {
		property_changed_func = property_changed_func_;
	}
};

struct PropertyValueBooleanDefaultTrue : public PropertyValue<bool> {
	PropertyValueBooleanDefaultTrue( bool value_ = true ) {
		value = value_;
	}
};


} // namespace PropertyTypes

class MemoryInterface
{
public:
	struct properties_storage_t
	{

		/**
		 * Since the flash drive can erase only whole pages,
		 * It's not possible writing only half of a page, without destroying all.
		 * To do this the driver will read the whole page before, manipulates the data
		 * in RAM, erases the page and writes back all.
		 *
		 * This requires at least PAGE_SIZE bytes of RAM.
		 *
		 * If you don't have so much RAM, or just don't need it you can disable
		 * this feature.
		 */
		PropertyTypes::PropertyValueBooleanDefaultTrue RestoreDataOnUnaligendWrites{};

		/**
		 * Set to true if the driver supports this feature
		 */
		PropertyTypes::PropertyValueBooleanDefaultTrue CanRestoreDataOnUnaligendWrites{};

		/**
		 * Disable automatically page erase
		 */
		PropertyTypes::PropertyValueBooleanDefaultTrue AutoErasePage{};


		void set_property_changed_func( std::function<void()> property_changed_func_ ) {
			RestoreDataOnUnaligendWrites.set_property_changed_func(property_changed_func_);
			CanRestoreDataOnUnaligendWrites.set_property_changed_func(property_changed_func_);
			AutoErasePage.set_property_changed_func(property_changed_func_);
		}
	};

	properties_storage_t properties;

public:

	MemoryInterface()
	: properties()
	{
		properties.set_property_changed_func([this](){
			properties_changed();
		});
	}

	virtual ~MemoryInterface() {}

	virtual std::size_t get_size() const = 0;

	virtual std::size_t get_page_size() const = 0;

	virtual std::size_t write( std::size_t address, const std::span<const std::byte> & data ) = 0;

	virtual std::size_t read( std::size_t address, std::span<std::byte> & data ) = 0;

	virtual bool erase( std::size_t address, std::size_t size ) = 0;


	virtual void properties_changed() {}
};

} // namespace smt32_internal_flash

#endif /* APP_STM32_INTERNAL_FLASH_INC_MEMORYINTERFACE_H_ */
