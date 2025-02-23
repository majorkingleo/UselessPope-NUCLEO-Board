/*
 * stm32_internal_flash_raw_h7.cpp
 *
 *  Created on: Sep 5, 2024
 *      Author: martin.oberzalek
 */
#include "stm32_internal_flash_raw_h7_4data.h"

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

namespace stm32_internal_flash {

#if defined(STM32H753xx)
/**
  * @brief  Program a flash word at a specified address
  * @param  TypeProgram Indicate the way to program at a specified address.
  *         This parameter can be a value of @ref FLASH_Type_Program
  * @param  FlashAddress specifies the address to be programmed.
  *         This parameter shall be aligned to the Flash word:
  *          - 256 bits for STM32H74x/5X devices (8x 32bits words)
  *          - 128 bits for STM32H7Ax/BX devices (4x 32bits words)
  *          - 256 bits for STM32H72x/3X devices (8x 32bits words)
  * @param  DataAddress specifies the address of data to be programmed.
  *         This parameter shall be 32-bit aligned
  *
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef STM32InternalFlashHalRawH74Data::FLASH_Program(uint32_t TypeProgram, uint32_t FlashAddress, uint32_t DataAddress)
{
/* This is exactly the HAL_FLASH_Program() function
 * except the following 3 defines
 * Since, we are using the flash storage for data, we don't have to
 * sync via __ISB and __DSB.
 */
#define FLASH_TIMEOUT_VALUE HAL_MAX_DELAY
#define __ISB()
#define __DSB()

  HAL_StatusTypeDef status;
  __IO uint32_t *dest_addr = (__IO uint32_t *)FlashAddress;
  __IO uint32_t *src_addr = (__IO uint32_t*)DataAddress;
  uint32_t bank;
  uint8_t row_index = FLASH_NB_32BITWORD_IN_FLASHWORD;

  /* Check the parameters */
  assert_param(IS_FLASH_TYPEPROGRAM(TypeProgram));
  assert_param(IS_FLASH_PROGRAM_ADDRESS(FlashAddress));

  /* Process Locked */
  __HAL_LOCK(&pFlash);

#if defined (FLASH_OPTCR_PG_OTP)
  if((IS_FLASH_PROGRAM_ADDRESS_BANK1(FlashAddress)) || (IS_FLASH_PROGRAM_ADDRESS_OTP(FlashAddress)))
#else
  if(IS_FLASH_PROGRAM_ADDRESS_BANK1(FlashAddress))
#endif /* FLASH_OPTCR_PG_OTP */
  {
	bank = FLASH_BANK_1;
	/* Prevent unused argument(s) compilation warning */
	UNUSED(TypeProgram);
  }
#if defined (DUAL_BANK)
  else if(IS_FLASH_PROGRAM_ADDRESS_BANK2(FlashAddress))
  {
	bank = FLASH_BANK_2;
  }
#endif /* DUAL_BANK */
  else
  {
	return HAL_ERROR;
  }

  /* Reset error code */
  pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation((uint32_t)FLASH_TIMEOUT_VALUE, bank);

  if(status == HAL_OK)
  {
#if defined (DUAL_BANK)
	if(bank == FLASH_BANK_1)
	{
#if defined (FLASH_OPTCR_PG_OTP)
	  if (TypeProgram == FLASH_TYPEPROGRAM_OTPWORD)
	  {
		/* Set OTP_PG bit */
		SET_BIT(FLASH->OPTCR, FLASH_OPTCR_PG_OTP);
	  }
	  else
#endif /* FLASH_OPTCR_PG_OTP */
	  {
		/* Set PG bit */
		SET_BIT(FLASH->CR1, FLASH_CR_PG);
	  }
	}
	else
	{
	  /* Set PG bit */
	  SET_BIT(FLASH->CR2, FLASH_CR_PG);
	}
#else /* Single Bank */
#if defined (FLASH_OPTCR_PG_OTP)
	  if (TypeProgram == FLASH_TYPEPROGRAM_OTPWORD)
	  {
		/* Set OTP_PG bit */
		SET_BIT(FLASH->OPTCR, FLASH_OPTCR_PG_OTP);
	  }
	  else
#endif /* FLASH_OPTCR_PG_OTP */
	  {
		/* Set PG bit */
		SET_BIT(FLASH->CR1, FLASH_CR_PG);
	  }
#endif /* DUAL_BANK */

	__ISB();
	__DSB();

#if defined (FLASH_OPTCR_PG_OTP)
	if (TypeProgram == FLASH_TYPEPROGRAM_OTPWORD)
	{
	  /* Program an OTP word (16 bits) */
	  *(__IO uint16_t *)FlashAddress = *(__IO uint16_t*)DataAddress;
	}
	else
#endif /* FLASH_OPTCR_PG_OTP */
	{
	  /* Program the flash word */
	  do
	  {
		*dest_addr = *src_addr;
		dest_addr++;
		src_addr++;
		row_index--;
	 } while (row_index != 0U);
	}

	__ISB();
	__DSB();

	/* Wait for last operation to be completed */
	status = FLASH_WaitForLastOperation((uint32_t)FLASH_TIMEOUT_VALUE, bank);

#if defined (DUAL_BANK)
#if defined (FLASH_OPTCR_PG_OTP)
	if (TypeProgram == FLASH_TYPEPROGRAM_OTPWORD)
	{
	  /* If the program operation is completed, disable the OTP_PG */
	  CLEAR_BIT(FLASH->OPTCR, FLASH_OPTCR_PG_OTP);
	}
	else
#endif /* FLASH_OPTCR_PG_OTP */
	{
	  if(bank == FLASH_BANK_1)
	  {
		/* If the program operation is completed, disable the PG */
		CLEAR_BIT(FLASH->CR1, FLASH_CR_PG);
	  }
	  else
	  {
		/* If the program operation is completed, disable the PG */
		CLEAR_BIT(FLASH->CR2, FLASH_CR_PG);
	  }
	}
#else /* Single Bank */
#if defined (FLASH_OPTCR_PG_OTP)
	if (TypeProgram == FLASH_TYPEPROGRAM_OTPWORD)
	{
	  /* If the program operation is completed, disable the OTP_PG */
	  CLEAR_BIT(FLASH->OPTCR, FLASH_OPTCR_PG_OTP);
	}
	else
#endif /* FLASH_OPTCR_PG_OTP */
	{
	  /* If the program operation is completed, disable the PG */
	  CLEAR_BIT(FLASH->CR1, FLASH_CR_PG);
	}
#endif /* DUAL_BANK */
  }

  /* Process Unlocked */
  __HAL_UNLOCK(&pFlash);

  return status;
}


#endif

} // namespace stm32_internal_flash {
