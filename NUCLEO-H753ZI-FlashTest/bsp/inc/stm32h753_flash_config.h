/*
 * Symbols and definitions for STM32F401
 * @author Copyright (c) 2024 Martin Oberzalek
 */


#ifndef APP_STM32F401_FLASH_CONFIG_H_
#define APP_STM32F401_FLASH_CONFIG_H_

#include <stm32_internal_flash.h>

/**
 * BANK1
 */
/*
 * Sector 0 0x0800 0000 - 0x0801 FFFF 128 Kbyte
 * Sector 1 0x0802 0000 - 0x0803 FFFF 128 Kbyte
 * Sector 2 0x0804 0000 - 0x0805 FFFF 128 Kbyte
 * Sector 3 0x0806 0000 - 0x0807 FFFF 128 Kbyte
 * Sector 4 0x0808 0000 - 0x0809 FFFF 128 Kbyte
 * Sector 5 0x080A 0000 - 0x080B FFFF 128 Kbyte
 * Sector 6 0x080C 0000 - 0x080D FFFF 128 Kbyte
 * Sector 7 0x080E 0000 - 0x080F FFFF 128 Kbyte
 */
#define ADDRESS_FLASH_SECTOR_0   0x08000000
#define ADDRESS_FLASH_SECTOR_1   (ADDRESS_FLASH_SECTOR_0  + 128*1024)
#define ADDRESS_FLASH_SECTOR_2   (ADDRESS_FLASH_SECTOR_1  + 128*1024)
#define ADDRESS_FLASH_SECTOR_3   (ADDRESS_FLASH_SECTOR_2  + 128*1024)
#define ADDRESS_FLASH_SECTOR_4   (ADDRESS_FLASH_SECTOR_3  + 128*1024)
#define ADDRESS_FLASH_SECTOR_5   (ADDRESS_FLASH_SECTOR_4  + 128*1024)
#define ADDRESS_FLASH_SECTOR_6   (ADDRESS_FLASH_SECTOR_5  + 128*1024)
#define ADDRESS_FLASH_SECTOR_7   (ADDRESS_FLASH_SECTOR_6  + 128*1024)

/**
 * BANK2
 */
/*
 * Sector 8  0x0810 0000 - 0x0811 FFFF 128 Kbyte
 * Sector 9  0x0812 0000 - 0x0813 FFFF 128 Kbyte
 * Sector 10 0x0814 0000 - 0x0815 FFFF 128 Kbyte
 * Sector 11 0x0815 0000 - 0x0817 FFFF 128 Kbyte
 * Sector 12 0x0818 0000 - 0x0819 FFFF 128 Kbyte
 * Sector 13 0x081A 0000 - 0x081B FFFF 128 Kbyte
 * Sector 14 0x081C 0000 - 0x081D FFFF 128 Kbyte
 * Sector 15 0x081E 0000 - 0x081F FFFF 128 Kbyte
 */
#define ADDRESS_FLASH_SECTOR_8    (ADDRESS_FLASH_SECTOR_7   + 128*1024)
#define ADDRESS_FLASH_SECTOR_9    (ADDRESS_FLASH_SECTOR_8   + 128*1024)
#define ADDRESS_FLASH_SECTOR_10   (ADDRESS_FLASH_SECTOR_9   + 128*1024)
#define ADDRESS_FLASH_SECTOR_11   (ADDRESS_FLASH_SECTOR_10  + 128*1024)
#define ADDRESS_FLASH_SECTOR_12   (ADDRESS_FLASH_SECTOR_11  + 128*1024)
#define ADDRESS_FLASH_SECTOR_13   (ADDRESS_FLASH_SECTOR_12  + 128*1024)
#define ADDRESS_FLASH_SECTOR_14   (ADDRESS_FLASH_SECTOR_13  + 128*1024)
#define ADDRESS_FLASH_SECTOR_15   (ADDRESS_FLASH_SECTOR_14  + 128*1024)
/**
 * The raw driver cannot handle different sections sizes at once
 */

// the raw driver can only sections with the same page size
static constexpr const stm32_internal_flash::Configuration::Sector flash_fs1_128k_sector[] = {
  {
    FLASH_SECTOR_7,
	128*1024,
	ADDRESS_FLASH_SECTOR_7
  }
};


// the raw driver can only sections with the same page size
static constexpr const stm32_internal_flash::Configuration::Sector flash_fs2_128k_sector[] = {
  {
    FLASH_SECTOR_7,
	128*1024,
	ADDRESS_FLASH_SECTOR_15
  }
};

#endif /* APP_STM32F401_FLASH_CONFIG_H_ */
