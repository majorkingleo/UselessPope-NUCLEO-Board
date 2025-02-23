#include <bsp_internal_fs.hpp>
#include <bslib-h753_internal_flash_update_memory.hpp>
#include "stm32h7xx_hal_rcc.h"
#include "H7TwoFace.h"
#include "H7TwoFaceConfig.h"
#include <stm32_internal_flash_raw_h7.h>
#include "stm32h753_flash_config.h"
#include <CpputilsDebug.h>
#include <static_format.h>

using namespace BSP;
using namespace stm32_internal_flash;
using namespace Tools;

extern "C" uint32_t _FLASH_DATA_1;
extern "C" uint32_t _FLASH_DATA_1_SIZE;

extern "C" uint32_t _FLASH_DATA_2;
extern "C" uint32_t _FLASH_DATA_2_SIZE;

namespace {

static void local_crc32_enable() {
  __HAL_RCC_CRC_CLK_ENABLE();
} 

static uint32_t local_crc32(const std::byte *buffer, size_t size) {
  // If the clock was turned on previously and kept on then it isn't necessary to do that here each time.
  // Did it here to match ST's example in AN4187 (page 8)
  // https://www.st.com/resource/en/application_note/dm00068118-using-the-crc-peripheral-in-the-stm32-family-stmicroelectronics.pdf)
  //__HAL_RCC_CRC_CLK_ENABLE();
  // For standard (Ethernet) CRC-32 bit order is reversed on input and output
  CRC->CR = CRC_CR_REV_IN_0 | CRC_CR_REV_IN_1 | CRC_CR_REV_OUT;
  // The initial value and polynomial are set up during initialization.
  // Conveniently, in my case, the reset values are already correct:
  CRC->INIT = 0xFFFFFFFF;
  CRC->POL = 0x04C11DB7;
  CRC->CR |= CRC_CR_RESET;
  uint32_t i = 0;
  // First work on as many full 32-bit words as we can
  uint32_t full_word_bytes = size & 0b11;
  while (i < full_word_bytes) {
    CRC->DR = *(uint32_t*)&buffer[i];
    i += 4;
  }
  // Now handle any additional bytes one at a time
  while (i < size) {
    // Here we are using 8-bit access to the CRC peripheral's data register
    // so it does not introduce padding into the computation.
    // (e.g. the CRC of 4 zeros is different than for only 1)
    *(uint8_t*)&CRC->DR = static_cast<uint8_t>(buffer[i]);
    i++;
  }
  // For standard (Ethernet) CRC-32 output bits need to all flip
  i = CRC->DR ^ 0xFFFFFFFF;
  //__HAL_RCC_CRC_CLK_DISABLE();
  return i;
}



class FsFlashMemInterfaceProxy : public SimpleFlashFs::FlashMemoryInterface
{
	// stm32_internal_flash::RawDriverInterface & driver;
	stm32_internal_flash::STM32InternalFlashHalRawBase & driver;

public:
	FsFlashMemInterfaceProxy( stm32_internal_flash::STM32InternalFlashHalRawBase & driver_ )
	: driver( driver_ )
	{}

	FsFlashMemInterfaceProxy() = delete;

	std::size_t size() const override {
		return driver.get_size();
	}

	std::size_t write( std::size_t address, const std::byte *data, std::size_t size ) override {
		return driver.write_page( address, std::span( data, size ) );
	}

	std::size_t read( std::size_t address, std::byte *data, std::size_t size ) override {
		std::span<std::byte> buffer( data, size );
		return driver.read_page( address, buffer );
	}

	void erase( std::size_t address, std::size_t size ) override {
		driver.erase_page( address, size );
	}

	bool can_map_read() const override {
		return true;
	}

	/**
	 * converts the address to a in memory mapped address
	 */
	const std::byte* map_read( std::size_t address, std::size_t size ) override {
		return driver.map_read( address, size );
	}
};

class H753updated_with_read_back : public H753_internal_flash_update_memory
{
	std::array<bool,SFF_MAX_PAGES> pages_written{};
	std::array<bool,SFF_MAX_PAGES> pages_tested{};

public:
	// use constructor from base
	using base_t = H753_internal_flash_update_memory;
	using base_t::base_t;

	std::size_t write( std::size_t address, const std::byte *data, std::size_t size ) override {

		set_page_tested( address, size, false );

		bool *val = get_page_written( address, size );

		std::size_t idx = address / SFF_PAGE_SIZE;

		if( val && *val ) {
			CPPDEBUG( static_format<100>("0x%X Page: %d already written", m_start_address, idx ) );
			return 0;
		}

		CPPDEBUG( static_format<100>("0x%X writing Page: %d", m_start_address, idx ) );

		if( val ) {
			*val = true;
		}
		pages_written[idx] = true;

		std::size_t len_written = base_t::write( address, data, size );

		if( len_written == size ) {

			auto res = this->test_read( address, size );

			if( !std::holds_alternative<bool>(res) ) {
				CPPDEBUG( static_format<200>("XXXXXXXXXXX 0x%X invalid write at %d (0x%X) size %d failed address: %d (0x%X)",
						m_start_address,
						address,
						m_start_address + address,
						size,
						std::get<std::size_t>(res),
						m_start_address + std::get<std::size_t>(res) ) );
				return 0;
			}
		}

		return len_written;
	}

	std::variant<bool,std::size_t> test_read( std::size_t address, std::size_t size, std::function<bool(std::byte)> crc_func = {} ) override
	{
		auto ret = base_t::test_read( address, size, crc_func );

		if( std::holds_alternative<bool>( ret ) ) {
			set_page_tested( address, size, true );
		}

		return ret;
	}

	void erase( std::size_t address, std::size_t size ) override
	{
		set_page_tested( address, size, false );
		set_page_written( address, size, false );
		base_t::erase( address, size );
	}

protected:
	bool should_test_read( std::size_t address, std::size_t size ) {

		bool *val = get_page_tested( address, size );

		if( !val ) {
			return true;
		}

		// not tested yet
		if( *val == false ) {
			return true;
		}

		return false;
	}

	bool* get_page_tested( std::size_t address, std::size_t size ) {
		return get_page( pages_tested, address, size );
	}

	bool set_page_tested(  std::size_t address, std::size_t size, bool value ) {

		std::size_t start = address;
		std::size_t junks = size / SFF_PAGE_SIZE;
		std::size_t end = address + SFF_PAGE_SIZE * junks;

		for(  std::size_t addr = start; addr < end; addr += SFF_PAGE_SIZE ) {
			if( !set_page( pages_tested, addr, SFF_PAGE_SIZE, value ) ) {
				return false;
			}
		}

		return true;
	}

	bool* get_page_written( std::size_t address, std::size_t size ) {
		return get_page( pages_written, address, size );
	}

	bool set_page_written(  std::size_t address, std::size_t size, bool value ) {

		std::size_t start = address;
		std::size_t junks = size / SFF_PAGE_SIZE;
		std::size_t end = address + SFF_PAGE_SIZE * junks;

		for(  std::size_t addr = start; addr < end; addr += SFF_PAGE_SIZE ) {
			if( !set_page( pages_written, addr, SFF_PAGE_SIZE, value ) ) {
				return false;
			}
		}

		return true;
	}


	bool* get_page( std::array<bool,SFF_MAX_PAGES> & pages, std::size_t address, std::size_t size ) {
		std::size_t idx = address / SFF_PAGE_SIZE;

		if( idx >= pages.size() ) {
			CPPDEBUG( Tools::static_format<100>("error: requested idx %d > %d", idx, pages.size() ) );
			return nullptr;
		}

		return &pages[idx];
	}

	bool set_page( std::array<bool,SFF_MAX_PAGES> & pages, std::size_t address, std::size_t size, bool value ) {
		bool *val = get_page( pages, address, size );

		if( !val ) {
			return false;
		}

		*val = value;

		return true;
	}

};

#if 0
void init_fs()
{
	using namespace stm32_internal_flash;

	static Configuration conf1;
	conf1.used_sectors = flash_fs1_128k_sector;
	conf1.banks = FLASH_BANK_1;

	static STM32InternalFlashHalRawH7 raw_driver_fs1( conf1 );
	static FsFlashMemInterfaceProxy mem_fs1( raw_driver_fs1 );



	static Configuration conf2;
	conf2.used_sectors = flash_fs2_128k_sector;
	conf2.banks = FLASH_BANK_2;

	static STM32InternalFlashHalRawH7 raw_driver_fs2( conf2 );
	static FsFlashMemInterfaceProxy mem_fs2( raw_driver_fs2 );


	H7TwoFace::set_memory_interface(&mem_fs1, &mem_fs2);

	local_crc32_enable();

	H7TwoFace::set_crc32_func(local_crc32);
}
#endif
} // namespace


void BSP::init_internal_fs()
{
  /* Enable I-Cache */
  SCB_EnableICache();
#if 1
  static H753updated_with_read_back mem_fs1( (std::size_t)(&_FLASH_DATA_1), (std::size_t)(&_FLASH_DATA_1_SIZE));
  static H753updated_with_read_back mem_fs2( (std::size_t)(&_FLASH_DATA_2), (std::size_t)(&_FLASH_DATA_1_SIZE));

  H7TwoFace::set_memory_interface(&mem_fs1, &mem_fs2);

  local_crc32_enable();

  H7TwoFace::set_crc32_func(local_crc32);
#else
  init_fs();
#endif
}
