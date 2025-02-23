#include <bslib-h753_internal_flash_update_memory.hpp>
#include <cstring>
#include <stdexcept>
#include <wlib.hpp>

namespace BSP {

H753_internal_flash_update_memory::H753_internal_flash_update_memory(std::size_t start_address, std::size_t size_in_byte)
    : m_start_address(start_address)
    , m_size_in_byte(size_in_byte)
{
  if (((this->m_start_address - bank_start_address) % sector_size != 0) || (size_in_byte % sector_size != 0))
  {
    throw std::runtime_error("Please check your config");
  }
}

bool H753_internal_flash_update_memory::erase()
{
  this->m_write_idx = 0;
  for (std::size_t add = this->m_start_address; add < (this->m_start_address + this->m_size_in_byte); add += sector_size)
  {
    uint8_t sec = (add - bank_start_address) / sector_size;
    if (!this->erase_sector(FLASH->KEYR2, FLASH->CR2, FLASH->SR2, sec))
      return false;
  }
  return true;
}

bool H753_internal_flash_update_memory::write(std::byte const& data)
{ 
  return this->write_data(FLASH->KEYR2, FLASH->CR2, FLASH->SR2, data); 
}

std::optional<uint32_t> H753_internal_flash_update_memory::flush()
{
  this->flush_data(FLASH->KEYR2, FLASH->CR2, FLASH->SR2);
  wlib::crc::CRC_32 crc;

  auto func_call_crc = [&crc](std::byte b){
  	 crc(b);
  	 return true;
  };

  auto read_result = test_read(0,this->m_write_idx, func_call_crc );

  if( !std::holds_alternative<bool>(read_result) ) {
	  return {};
  };

  this->m_write_idx = 0;
  return crc.get();
}

bool H753_internal_flash_update_memory::swap()
{
  uint32_t const current_value = FLASH->OPTCR & FLASH_OPTCR_SWAP_BANK_Msk;
  uint32_t const new_value     = ~current_value & FLASH_OPTCR_SWAP_BANK_Msk;

  if ((FLASH->OPTCR & FLASH_OPTCR_OPTLOCK) != 0)
  {
    FLASH->OPTKEYR = 0x0819'2A3B;
    FLASH->OPTKEYR = 0x4C5D'6E7F;
  }

  FLASH->OPTSR_PRG = (FLASH->OPTSR_PRG & ~FLASH_OPTCR_SWAP_BANK_Msk) | (new_value & FLASH_OPTCR_SWAP_BANK_Msk);
  FLASH->OPTCR |= FLASH_OPTCR_OPTSTART;

  while ((FLASH->OPTSR_CUR & FLASH_OPTCR_SWAP_BANK_Msk) != new_value)
  {
  }

  FLASH->OPTCR |= FLASH_OPTCR_OPTLOCK;
  return true;
}

bool H753_internal_flash_update_memory::erase_sector(uint32_t volatile& key, uint32_t volatile& cr, uint32_t volatile& sr, uint8_t sec)
{
  if ((cr & FLASH_CR_LOCK) != 0)
  {
    key = 0x4567'0123;
    key = 0xCDEF'89AB;
  }

  if ((cr & FLASH_CR_LOCK) != 0)
  {
    return false;
  }

  cr = (cr & ~(FLASH_CR_SNB_Msk | FLASH_CR_SER_Msk | FLASH_CR_BER_Msk)) | (((sec << FLASH_CR_SNB_Pos) & FLASH_CR_SNB_Msk) | FLASH_CR_SER);
  cr |= FLASH_CR_START;

  while (((sr & FLASH_SR_BSY) != 0))
  {
  }
  SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(bank_start_address + sector_size * sec), sector_size);

  cr |= FLASH_CR_LOCK;
  return (cr & FLASH_CR_LOCK) != 0;
}

bool H753_internal_flash_update_memory::write_data(uint32_t volatile& key, uint32_t volatile& cr, uint32_t volatile& sr, std::byte const& data)
{
  if ((cr & FLASH_CR_LOCK) != 0)
  {
    key = 0x4567'0123;
    key = 0xCDEF'89AB;
  }

  if ((cr & FLASH_CR_LOCK) != 0)
  {
    return false;
  }

  cr |= FLASH_CR_PG;

  reinterpret_cast<std::byte*>(this->m_start_address)[this->m_write_idx++] = data;
  while (((sr & FLASH_SR_BSY) != 0))
  {
  }
  return true;
}

bool H753_internal_flash_update_memory::flush_data(uint32_t volatile& key, uint32_t volatile& cr, uint32_t volatile& sr)
{
  if ((cr & FLASH_CR_LOCK) != 0)
  {
    key = 0x4567'0123;
    key = 0xCDEF'89AB;
  }

  if ((cr & FLASH_CR_LOCK) != 0)
  {
    return false;
  }

  cr |= FLASH_CR_PG;
  if ((sr & FLASH_SR_WBNE) != 0)
  {
    cr |= FLASH_CR_FW;
    while (((sr & FLASH_SR_BSY) != 0))
    {
    }
  }
  cr &= FLASH_CR_PG;
  cr |= FLASH_CR_LOCK;
  SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(this->m_start_address), this->m_size_in_byte);
  return (cr & FLASH_CR_LOCK) != 0;
}


std::size_t H753_internal_flash_update_memory::write( std::size_t address, const std::byte *data, std::size_t size )
{
  m_write_idx = address;
  std::size_t data_written = 0;

  for ( std::size_t i = 0; i < size; ++i) {
    if (!write(data[i]) ) {
      return data_written;
    }
    ++data_written;
  }

  return data_written;
}

std::size_t H753_internal_flash_update_memory::read( std::size_t address, std::byte *data, std::size_t size )
{
	if( should_test_read( address, size ) ) {
	  auto func_memcpy_while_testing = [&data]( std::byte b ) {
		  *data = b;
		  data++;
		  return true;
	  };

	  auto res = test_read( address, size, func_memcpy_while_testing );

	  if( !std::holds_alternative<bool>(res) ) {
		return 0;
	  }

	  return size;
	}

  std::memcpy(data, reinterpret_cast<std::byte*>(this->m_start_address) + address, size);
  return size;
}

std::variant<bool,std::size_t> H753_internal_flash_update_memory::test_read( std::size_t address, std::size_t size, std::function<bool(std::byte)> crc_func )
{
	const uint32_t BFARVALID_MASK = (0x80 << SCB_CFSR_BUSFAULTSR_Pos);
	std::variant<bool,std::size_t> res = true;

	/* Clear BFARVALID flag by writing 1 to it */
	SCB->CFSR |= BFARVALID_MASK;

	/* Ignore BusFault by enabling BFHFNMIGN; disable faults and interrupts */
	uint32_t mask = __get_FAULTMASK();
	__disable_fault_irq();
	SCB->CCR |= SCB_CCR_BFHFNMIGN_Msk;

	volatile const char *addr = reinterpret_cast<const char*>( m_start_address + address );
	std::byte b;

	/* probe the address by performing 8-bit read */
	for( unsigned i = 0; i < size; ++i, addr++ ) {
		__DSB();
		b = static_cast<std::byte>(*addr);
		__DMB();

		if (SCB->CFSR & BFARVALID_MASK) {
			/* Yes, Bus Fault occurred */
			res = address + i;
			break;
		}

		if( crc_func ) {
			if( !crc_func( b ) ) {
				break;
			}
		}
	}

	/* Re-enable BusFault by clearing  BFHFNMIGN */
	SCB->CCR &= ~SCB_CCR_BFHFNMIGN_Msk;
	__set_FAULTMASK(mask);
	__enable_fault_irq();
	__DSB();
	return res;
}

const std::byte* H753_internal_flash_update_memory::map_read( std::size_t address, std::size_t size ) {

  if( should_test_read( address, size ) ) {
	  auto res = test_read( address, size );
	  if( !std::holds_alternative<bool>(res) ) {
		  return nullptr;
	  }
  }

  return reinterpret_cast<std::byte*>(m_start_address + address);
}

} // namespace BSP
