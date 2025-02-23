#include <stm32h753xx.h>
#include <SimpleFlashFsFlashMemoryInterface.h>
#include <optional>
#include <variant>
#include <functional>

namespace BSP {

  class H753_internal_flash_update_memory:  public SimpleFlashFs::FlashMemoryInterface
  {
    static constexpr std::size_t sector_size        = 128 * 1024;
    static constexpr std::size_t bank_start_address = 0x0810'0000;

  public:
    H753_internal_flash_update_memory(std::size_t start_address, std::size_t size_in_byte);


    //////////////////////////////////////////////////////////////
    // canopen::legacy_updater::update_mem_Interface functions
    //////////////////////////////////////////////////////////////

    bool erase();

    bool write(std::byte const& data);

    // invalidates instruction cache
    // returns crc32 of all data
    std::optional<uint32_t> flush();

    // swaps the flash banks
    bool swap();



    //////////////////////////////////////////////////////////////
    // SimpleFlashFs::FlashMemoryInterface functions 
    //////////////////////////////////////////////////////////////

    std::size_t size() const override { return m_size_in_byte; }

    std::size_t write( std::size_t address, const std::byte *data, std::size_t size ) override;

    std::size_t read( std::size_t address, std::byte *data, std::size_t size ) override;

    void erase( std::size_t address, std::size_t size ) override {
      erase();
      flush_data(FLASH->KEYR2, FLASH->CR2, FLASH->SR2);
    }

    /**
    * returns true if the flash memory is mapped into the address space,
    * so for reading we can simple get an address pointer
    */
    bool can_map_read() const override {
      return true;
    }

    /**
    * converts the address to a in memory mapped address
    */
    const std::byte* map_read( std::size_t address, std::size_t size ) override;

  public:
    /**
     * Test accessing the specified memory area. Detects if there is CRC error in flash.
     * Continues testing as long the crc_func returns true and the specified size is not reached.
     *
     * return true on success
     *        or the address of failure
     */
    virtual std::variant<bool,std::size_t> test_read( std::size_t address, std::size_t size, std::function<bool(std::byte)> crc_func = {} );

  protected:
    uint32_t const m_start_address;
    uint32_t const m_size_in_byte;
    uint32_t       m_write_idx = 0;

    bool erase_sector(uint32_t volatile& key, uint32_t volatile& cr, uint32_t volatile& sr, uint8_t sec);

    bool write_data(uint32_t volatile& key, uint32_t volatile& cr, uint32_t volatile& sr, std::byte const& data);    

    bool flush_data(uint32_t volatile& key, uint32_t volatile& cr, uint32_t volatile& sr);

    virtual bool should_test_read( std::size_t address, std::size_t size ) {
    	return true;
    }
  };

} // namespace BSP
