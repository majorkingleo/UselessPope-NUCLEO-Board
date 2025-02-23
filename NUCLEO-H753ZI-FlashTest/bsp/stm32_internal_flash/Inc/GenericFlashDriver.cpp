/*
 * Generic driver class that can write unaligned to any
 * flash page. Will read necessary data from the page before.
 * Do call automatically erase, before writing.
 *
 * @author Copyright (c) 2024 Martin Oberzalek
 */
#include "GenericFlashDriver.h"
#include <alloca.h>
#include <string.h>

namespace stm32_internal_flash {

GenericFlashDriver::GenericFlashDriver( RawDriverInterface & raw_driver_ )
: raw_driver( raw_driver_ )
{
	properties.set_property_changed_func([this](){
		properties_changed();
	});

	MemoryInterface::properties.CanRestoreDataOnUnaligendWrites = true;
}

std::size_t GenericFlashDriver::get_size() const
{
	return raw_driver.get_size();
}

std::size_t GenericFlashDriver::get_page_size() const
{
	return raw_driver.get_page_size();
}

std::size_t GenericFlashDriver::write( std::size_t address, const std::span<const std::byte> & data )
{
	auto data_int = data;
	const std::size_t page_size = get_page_size();
	std::size_t len_written = 0;

	// address is not page aligned
	if( const std::size_t first_slice_len = address % page_size; first_slice_len != 0 ) {
		const std::size_t data_left_on_first_page = page_size - first_slice_len;

		std::size_t len = 0;

		if( MemoryInterface::properties.RestoreDataOnUnaligendWrites ) {
			len = write_unaligned_first_page( address, data_int.subspan( 0, data_left_on_first_page ) );
		} else {
			len = write_unaligned_first_page_no_buffer( address, data_int.subspan( 0, data_left_on_first_page ) );
		}

		if( len != data_left_on_first_page ) {
			return len;
		}

		len_written += len;

		data_int = data_int.subspan( len_written );
	}

	std::size_t page_aligned_data_len = data_int.size() - len_written;
	page_aligned_data_len -= page_aligned_data_len % page_size;

	if( page_aligned_data_len > 0 ) {
		if(  MemoryInterface::properties.AutoErasePage ) {
			if( !raw_driver.erase_page(address + len_written, page_aligned_data_len ) ) {
				return len_written;
			}
		}

		auto data_to_write = data_int.subspan(0, page_aligned_data_len);
		std::size_t len = raw_driver.write_page(address + len_written, data_to_write);
		len_written += len;

		if( len != data_to_write.size() ) {
			return len_written;
		}

		data_int = data_int.subspan(len);
	}

	// last slice of data is not page aligned
	if( len_written < data.size() ) {
		if(  MemoryInterface::properties.RestoreDataOnUnaligendWrites ) {
			len_written += write_unaligned_last_page( address + len_written, data_int );
		} else {
			len_written += write_unaligned_last_page_no_buffer( address + len_written, data_int );
		}
	}


	return len_written;
}

std::size_t GenericFlashDriver::write_unaligned_first_page( std::size_t address, const std::span<const std::byte> & data )
{
	const std::size_t page_size = get_page_size();
	const std::size_t size_to_read_from_page = address % page_size;
	const std::size_t page_start_address = address - size_to_read_from_page;

	std::byte *buffer = nullptr;
	auto page_buffer =  properties.PageBuffer.get();

	if( page_buffer ) {

		if( page_buffer->size() < page_size ) {
			return 0;
		}

		buffer = page_buffer->data();

	} else {
		// allocate space on stack
		buffer = reinterpret_cast<std::byte*>( alloca( page_size ) );
	}

	std::span<std::byte> span_buffer( buffer, page_size );
	auto span_to_read = span_buffer.subspan(0, size_to_read_from_page);

	// read first part of the page into memory
	std::size_t len = read( page_start_address, span_to_read );

	if( len != span_to_read.size() ) {
		return 0;
	}

	// copy rest of first page into buffer
	auto rest_of_data = span_buffer.subspan( size_to_read_from_page );
	memcpy( rest_of_data.data(), data.data(), data.size() );

	if( MemoryInterface::properties.AutoErasePage ) {
		// now we can erase the page and write it
		if( !raw_driver.erase_page(page_start_address, page_size) ) {
			return 0;
		}
	}

	// now write it
	len = raw_driver.write_page(page_start_address, span_buffer);

	if( len != span_buffer.size() ) {
		return 0;
	}

	len -= size_to_read_from_page;

	// amount of new data written to flash
	// must be data.size()
	if( len != data.size() ) {
		return 0;
	}

	return len;
}

std::size_t GenericFlashDriver::write_unaligned_last_page( std::size_t address, const std::span<const std::byte> & data )
{
	const std::size_t page_size = get_page_size();

	std::byte *buffer = nullptr;
	auto page_buffer = properties.PageBuffer.get();

	if( page_buffer ) {
		buffer = page_buffer->data();

		if( page_buffer->size() < page_size ) {
			return 0;
		}

	} else {
		// allocate space on stack
		buffer = reinterpret_cast<std::byte*>( alloca( page_size ) );
	}

	std::span<std::byte> span_buffer( buffer, page_size );
	auto span_to_read = span_buffer.subspan(data.size());

	// read first part of the page into memory
	std::size_t len = read( address + data.size(), span_to_read );

	if( len != span_to_read.size() ) {
		return 0;
	}

	// copy rest of first page into buffer
	auto rest_of_data = span_buffer.subspan( 0, data.size() );
	memcpy( rest_of_data.data(), data.data(), data.size() );

	if( MemoryInterface::properties.AutoErasePage ) {
		// now we can erase the page and write it
		if( !raw_driver.erase_page(address, page_size) ) {
			return 0;
		}
	}

	// now write it
	len = raw_driver.write_page(address, span_buffer);

	if( len != span_buffer.size() ) {
		return 0;
	}

	len -= span_to_read.size();

	// amount of new data written to flash
	// must be data.size()
	if( len != data.size() ) {
		return 0;
	}

	return len;
}

std::size_t GenericFlashDriver::read( std::size_t address, std::span<std::byte> & data )
{
	/**
	 * since reading from internal flash is just mem access,
	 * there is no page wide reading required here.
	 */
	return raw_driver.read_page( address, data );
}

bool GenericFlashDriver::erase( std::size_t address, std::size_t size )
{
	return raw_driver.erase_page(address, size);
}

std::size_t GenericFlashDriver::write_unaligned_first_page_no_buffer( std::size_t address, const std::span<const std::byte> & data )
{
	const std::size_t page_size = get_page_size();
	const std::size_t size_to_read_from_page = address % page_size;
	const std::size_t page_start_address = address - size_to_read_from_page;

	if( MemoryInterface::properties.AutoErasePage ) {
		// now we can erase the page and write it
		if( !raw_driver.erase_page(page_start_address, page_size) ) {
			return 0;
		}
	}

	// now write it
	std::size_t len = raw_driver.write_page(address, data);

	if( len != data.size() ) {
		return 0;
	}

	return len;
}

std::size_t GenericFlashDriver::write_unaligned_last_page_no_buffer( std::size_t address, const std::span<const std::byte> & data )
{
	const std::size_t page_size = get_page_size();

	if( MemoryInterface::properties.AutoErasePage ) {
		// now we can erase the page and write it
		if( !raw_driver.erase_page(address, page_size) ) {
			return 0;
		}
	}

	// now write it
	std::size_t len = raw_driver.write_page(address, data);

	if( len != data.size() ) {
		return 0;
	}

	return len;
}

} // namespace smt32_internal_flash
