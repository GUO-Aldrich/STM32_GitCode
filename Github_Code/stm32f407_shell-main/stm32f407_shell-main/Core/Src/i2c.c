/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  *          of the I2C instances.
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
#include "i2c.h"

/* USER CODE BEGIN 0 */
#include "shell.h"
#include "stdio.h"
/* USER CODE END 0 */

I2C_HandleTypeDef hi2c1;

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

  /* USER CODE END I2C1_MspInit 0 */

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB8     ------> I2C1_SCL
    PB9     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
  /* USER CODE BEGIN I2C1_MspInit 1 */

  /* USER CODE END I2C1_MspInit 1 */
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspDeInit 0 */

  /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB8     ------> I2C1_SCL
    PB9     ------> I2C1_SDA
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_9);

  /* USER CODE BEGIN I2C1_MspDeInit 1 */

  /* USER CODE END I2C1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/**
 * @brief I2C Scan function
 * 
 */
void I2C_Scan(void)
{
  HAL_StatusTypeDef result;
  uint8_t i;

  printf("Scanning I2C bus...\r\n");
  printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");

  for (i = 0; i < 128; i++)
  {
    if (i % 16 == 0)
    {
      printf("%02x: ", i);
    }

    /*
     * The HAL_I2C_IsDeviceReady function checks if the I2C device with the 
     * specified address is ready for communication.
     */
    result = HAL_I2C_IsDeviceReady(&hi2c1, (uint32_t)(i << 1), 3, 20);

    if (result == HAL_OK)
    {
      printf("%02X ", i);
    }
    else
    {
      printf("-- ");
    }

    if ((i + 1) % 16 == 0)
    {
      printf("\r\n");
    }
  }
  printf("Scan complete.\r\n");
}

SHELL_EXPORT_CMD(i2c_scan, I2C_Scan, scan i2c devices);

/**
 * @brief I2C Write function
 * 
 * @param devAddr Device address (7-bit)
 * @param regAddr Register address
 * @param data Data to write
 */
void I2C_Mem_Write(uint8_t devAddr, uint8_t regAddr, uint8_t data)
{
  HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(devAddr << 1), regAddr, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
  if (status == HAL_OK)
  {
    printf("I2C Mem Write: [0x%02X] @ 0x%02X = 0x%02X OK\r\n", regAddr, devAddr, data);
  }
  else
  {
    printf("I2C Mem Write Failed: %d\r\n", status);
  }
}
SHELL_EXPORT_CMD(i2c_mem_write, I2C_Mem_Write, i2c memory write data);

/**
 * @brief I2C Read function
 * 
 * @param devAddr Device address (7-bit)
 * @param regAddr Register address
 */
void I2C_Mem_Read(uint8_t devAddr, uint8_t regAddr)
{
  uint8_t data = 0;
  HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(devAddr << 1), regAddr, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
  if (status == HAL_OK)
  {
    printf("I2C Mem Read: [0x%02X] @ 0x%02X = 0x%02X\r\n", regAddr, devAddr, data);
  }
  else
  {
    printf("I2C Mem Read Failed: %d\r\n", status);
  }
}
SHELL_EXPORT_CMD(i2c_mem_read, I2C_Mem_Read, i2c memory read data);

/**
 * @brief I2C Dump function
 * 
 * @param devAddr Device address (7-bit)
 * @param regAddr Start register address
 * @param len Number of bytes to read
 */
void I2C_Dump(uint8_t devAddr, uint8_t regAddr, uint8_t len)
{
  uint8_t buffer[256];
  if (len == 0) return;
  
  HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(devAddr << 1), regAddr, I2C_MEMADD_SIZE_8BIT, buffer, len, 1000);
  if (status == HAL_OK)
  {
    printf("I2C Dump: %d bytes from [0x%02X] @ 0x%02X\r\n", len, regAddr, devAddr);
    printf("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
    for (uint16_t i = 0; i < len; i++)
    {
      if (i % 16 == 0)
      {
        printf("\r\n%02X: ", regAddr + i);
      }
      printf("%02X ", buffer[i]);
    }
    printf("\r\n");
  }
  else
  {
    printf("I2C Dump Failed: %d\r\n", status);
  }
}
SHELL_EXPORT_CMD(i2c_dump, I2C_Dump, i2c dump data);

/**
 * @brief I2C Master Transmit function (using HAL_I2C_Master_Transmit)
 * 
 * @param devAddr Device address (7-bit)
 * @param regAddr Register address
 * @param data Data to transmit
 */
void I2C_Master_Transmit(uint8_t devAddr, uint8_t regAddr, uint8_t data)
{
  uint8_t buf[2] = {regAddr, data};
  HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(devAddr << 1), buf, 2, 100);
  if (status == HAL_OK)
  {
    printf("I2C Master Transmit: [0x%02X] @ 0x%02X = 0x%02X OK\r\n", regAddr, devAddr, data);
  }
  else
  {
    printf("I2C Master Transmit Failed: %d\r\n", status);
  }
}
SHELL_EXPORT_CMD(i2c_transmit, I2C_Master_Transmit, i2c master transmit data);

/**
 * @brief I2C Master Receive function (using HAL_I2C_Master_Transmit + HAL_I2C_Master_Receive)
 * 
 * @param devAddr Device address (7-bit)
 * @param regAddr Register address
 */
void I2C_Master_Receive(uint8_t devAddr, uint8_t regAddr)
{
  uint8_t data = 0;
  HAL_StatusTypeDef status;

  /* Step 1: Send Register Address */
  status = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(devAddr << 1), &regAddr, 1, 100);
  if (status == HAL_OK)
  {
    /* Step 2: Receive Data */
    status = HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(devAddr << 1), &data, 1, 100);
  }

  if (status == HAL_OK)
  {
    printf("I2C Master Receive: [0x%02X] @ 0x%02X = 0x%02X\r\n", regAddr, devAddr, data);
  }
  else
  {
    printf("I2C Master Receive Failed: %d\r\n", status);
  }
}
SHELL_EXPORT_CMD(i2c_receive, I2C_Master_Receive, i2c master receive data);

/* USER CODE END 1 */
