#include "stm32f0xx_hal.h"
#include <stm32f0xx_ll_dma.h>
#include <spilcd16/st7789.hpp>
#include "st7789.h"

SPI_HandleTypeDef hspi1;
TFT_ST7789<3, Pin::NO_PIN, ST7789_DC_Pin, Pin::PB07, ST7789_RST_Pin, 32, 480> lcd(hspi1.Instance, DisplayRotation::ROT0, (uint16_t)240, (uint16_t)240);

void SystemClock_Config(void);
void Error_Handler(void);

extern "C" void SysTick_Handler(void){HAL_IncTick();}

extern "C" void DMA1_Channel3_IRQHandler(void) //SPI1_TX
{
	if (LL_DMA_IsActiveFlag_TC3(DMA1)) {
		LL_DMA_ClearFlag_GI3(DMA1);
		//TODOlcd.spiTxCompleteCallback();
	} else if (LL_DMA_IsActiveFlag_TE3(DMA1)) {
		while (1) {
			__asm__ __volatile__ ("bkpt #0");
		}
	}
}

void HardFault_Handler(void){while(1){}}
void MemManage_Handler(void){while(1){}}
void BusFault_Handler(void){while(1){}}
void UsageFault_Handler(void){while(1){}}
void SVC_Handler(void){}
void DebugMon_Handler(void){}
void PendSV_Handler(void){}
void NMI_Handler(void){}



void MX_SPI1_Init(void)
{
  __HAL_RCC_SPI1_CLK_ENABLE();
  Gpio::ConfigureAlternateFunction(Pin::PB03, GPIO_AF0_SPI1, OutputType::PUSH_PULL, OutputSpeed::HIGH);
  Gpio::ConfigureAlternateFunction(Pin::PB05, GPIO_AF0_SPI1, OutputType::PUSH_PULL, OutputSpeed::HIGH);
  
  NVIC_SetPriority(SPI1_IRQn, 0);
  /* Enable SPI1_IRQn           */
  NVIC_EnableIRQ(SPI1_IRQn);

  
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
}

extern "C" int main(void)
{

  HAL_Init();
  SystemClock_Config();
  Gpio::ConfigureGPIOOutput(ST7789_DC_Pin, false, OutputType::PUSH_PULL, OutputSpeed::HIGH);
  Gpio::ConfigureGPIOOutput(ST7789_RST_Pin, false, OutputType::PUSH_PULL, OutputSpeed::HIGH);
  MX_SPI1_Init();
  
  //lcd.begin();
  ST7789_Init();
  ST7789_Fill_Color(BLUE);
  while(1){
    lcd.setColors(GREEN, RED);
    lcd.fillScreen();
    lcd.setColors(RED, GREEN);
    lcd.fillScreen();
  }
  



  //ST7789_Init();
  //while (1){ST7789_Test();}
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
