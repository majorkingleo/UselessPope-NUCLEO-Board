#include "stm32h7xx_hal.h"
#include "bsp_uart_usb.hpp"
#include <bslib.hpp>

static void MX_USART3_UART_Init(void);
static void UART_MspInit(UART_HandleTypeDef* huart);

static UART_HandleTypeDef huart3;

static void Error_Handler(void)
{
  while (1)
  {
  }
}

/**
* @brief USART3 Initialization Function
* @param None
* @retval None
*/
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  huart3.MspInitCallback = UART_MspInit;

  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

int BSP::usb_uart_put_char(int ch)
{
  HAL_UART_Transmit(&huart3, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

std::optional<char> BSP::usb_uart_get_char()
{
  char ch;
  if( HAL_UART_Receive(&huart3, (uint8_t*)&ch, 1, HAL_MAX_DELAY) == HAL_OK ) {
	  return ch;
  }

  return {};
}

void BSP::init_uart_usb()
{
  MX_USART3_UART_Init();
}


/**
* @brief UART MSP Initialization
* This function configures the hardware resources used in this example
* @param huart: UART handle pointer
* @retval None
*/
static void UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {};
  if(huart->Instance==USART3)
  {
    /* USER CODE BEGIN USART3_MspInit 0 */

    /* USER CODE END USART3_MspInit 0 */

    /** Initializes the peripherals clock
    */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3;
    PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_HSI;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* Peripheral clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**USART3 GPIO Configuration
    PD8     ------> USART3_TX
    PD9     ------> USART3_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* USER CODE BEGIN USART3_MspInit 1 */

    /* USER CODE END USART3_MspInit 1 */
  }
}

namespace {

class USBUartDebug : public wlib::StringSink_Interface
{
public:
  bool operator()(char const* c_str, uint32_t len ) override {
    for (uint32_t i = 0; i < len; i++)
    {
      BSP::usb_uart_put_char(c_str[i]);
    }
    return true;
  }
};

} // namespace


wlib::StringSink_Interface& BSP::get_usb_uart_output_debug()
{
  static USBUartDebug obj;
  return obj;
}
