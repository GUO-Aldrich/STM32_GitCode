/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "can.h"
#include "dac.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ring.h"
#include "shell.h"
#include "stdio.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC1_DMA_BUF_SIZE 1024

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
ring uart_rx_ring;
char ring_buf[512];
SHELL_TypeDef shell;
uint16_t adc1_dma_buf[ADC1_DMA_BUF_SIZE];
uint16_t dac1_dma_buf[ADC1_DMA_BUF_SIZE];
volatile uint8_t adc1_dma_half_ready = 0;
volatile uint8_t adc1_dma_full_ready = 0;
volatile uint8_t dac1_dma_half_ready = 1;
volatile uint8_t dac1_dma_full_ready = 1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
signed char myShellRead(char *c)
{
	if (ring_poll(&uart_rx_ring, c) == 0)
		return 0;
	else
		return -1;
}

void myShellWrite(const char c)
{
	HAL_UART_Transmit(&huart1, (uint8_t *)&c, 1, 1000);
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC1)
  {
    adc1_dma_half_ready = 1;
  }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC1)
  {
    adc1_dma_full_ready = 1;
  }
}

void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
  if (hdac->Instance == DAC)
  {
    dac1_dma_half_ready = 1;
  }
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
  if (hdac->Instance == DAC)
  {
    dac1_dma_full_ready = 1;
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	ring_init(&uart_rx_ring,ring_buf,512);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_CAN1_Init();
  MX_ADC1_Init();
  MX_DAC_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  
	shell.read = myShellRead;
	shell.write = myShellWrite;
	shellInit(&shell);

  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc1_dma_buf, ADC1_DMA_BUF_SIZE) != HAL_OK)
  {
    Error_Handler();
  }

  memset(dac1_dma_buf, 0, sizeof(dac1_dma_buf));

  if (HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t *)dac1_dma_buf, ADC1_DMA_BUF_SIZE, DAC_ALIGN_12B_R) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_Base_Start(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t shellTick = HAL_GetTick();

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (HAL_GetTick() - shellTick >= 10)
    {
      shellTick = HAL_GetTick();
      shellTask(&shell);
    }

    if (adc1_dma_half_ready && dac1_dma_half_ready)
    {
      adc1_dma_half_ready = 0;
      dac1_dma_half_ready = 0;
      //可以做一些数据处理，这里直接复制buffer
      memcpy((void *)&dac1_dma_buf[0], (void *)&adc1_dma_buf[0], (ADC1_DMA_BUF_SIZE / 2) * sizeof(uint16_t));
    }

    if (adc1_dma_full_ready && dac1_dma_full_ready)
    {
      adc1_dma_full_ready = 0;
      dac1_dma_full_ready = 0;
      //可以做一些数据处理，这里直接复制buffer
      memcpy((void *)&dac1_dma_buf[ADC1_DMA_BUF_SIZE / 2],
            (void *)&adc1_dma_buf[ADC1_DMA_BUF_SIZE / 2],
            (ADC1_DMA_BUF_SIZE / 2) * sizeof(uint16_t));
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
