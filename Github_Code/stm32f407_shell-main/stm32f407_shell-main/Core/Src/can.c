/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.c
  * @brief   This file provides code for the configuration
  *          of the CAN instances.
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
#include "can.h"

/* USER CODE BEGIN 0 */
#include "shell.h"
#include "stdio.h"

/* USER CODE END 0 */

CAN_HandleTypeDef hcan1;

/* CAN1 init function */
void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 6;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_10TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_3TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */
  CAN_FilterTypeDef  sFilterConfig;

  /* Configure the CAN Filter */
  sFilterConfig.FilterBank = 0;
  sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh = 0x0000;
	sFilterConfig.FilterIdLow = 0x0000;
	sFilterConfig.FilterMaskIdHigh = 0x0000;
	sFilterConfig.FilterMaskIdLow = 0x0000;
  sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  sFilterConfig.FilterActivation = ENABLE;
  sFilterConfig.SlaveStartFilterBank = 14;

  if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
  {
    printf("CAN filter configuration failed\r\n");
  }

  /* Start the CAN peripheral */
  if (HAL_CAN_Start(&hcan1) != HAL_OK)
  {
    printf("CAN start failed\r\n");
  }

  /*  Activate CAN RX notification */
	/*
  if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
  {
    printf("CAN notification activation failed\r\n");
  }
	*/
  /* USER CODE END CAN1_Init 2 */

}

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspInit 0 */

  /* USER CODE END CAN1_MspInit 0 */
    /* CAN1 clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**CAN1 GPIO Configuration
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN CAN1_MspInit 1 */

  /* USER CODE END CAN1_MspInit 1 */
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspDeInit 0 */

  /* USER CODE END CAN1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CAN1_CLK_DISABLE();

    /**CAN1 GPIO Configuration
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11|GPIO_PIN_12);

  /* USER CODE BEGIN CAN1_MspDeInit 1 */

  /* USER CODE END CAN1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/**
  * @brief  Send a standard (11-bit) CAN data frame.
  * @param  stdId: 11-bit standard identifier (0..0x7FF)
  * @param  pData: pointer to data bytes (up to 8 bytes)
  * @param  len: number of data bytes (0..8)
  * @retval HAL status: HAL_OK on success, HAL_ERROR otherwise
  */
HAL_StatusTypeDef CAN_SendStd(uint16_t stdId, uint8_t *pData, uint8_t len)
{
  CAN_TxHeaderTypeDef txHeader;
  uint32_t txMailbox = 0;
  if (len > 8U) {
    printf("CAN_SendStd: invalid len %u\r\n", len);
    return HAL_ERROR;
  }

  txHeader.StdId = stdId & 0x7FFU;
  txHeader.ExtId = 0U;
  txHeader.RTR = CAN_RTR_DATA;
  txHeader.IDE = CAN_ID_STD;
  txHeader.DLC = len;
  txHeader.TransmitGlobalTime = DISABLE;

  HAL_StatusTypeDef ret = HAL_CAN_AddTxMessage(&hcan1, &txHeader, pData, &txMailbox);
  if (ret != HAL_OK) {
    printf("CAN_SendStd: HAL_CAN_AddTxMessage failed=%d mailbox=%u free=%d\r\n",
           (int)ret, txMailbox, HAL_CAN_GetTxMailboxesFreeLevel(&hcan1));
    return ret;
  }

  /* Wait for transmission to complete */
  uint32_t timeout = 100000;
  while (timeout--) {
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 3) {
      return HAL_OK;
    }
  }
  printf("CAN_SendStd: transmit timeout, free=%d\r\n", HAL_CAN_GetTxMailboxesFreeLevel(&hcan1));
  return HAL_ERROR;
}


/**
 * @brief Shell wrapper to send a standard CAN frame.
 * Usage: can_send <stdId> <len>
 */
void CAN_Send_Cmd(uint16_t stdId, uint8_t len)
{
  uint8_t buf[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  if (len > 8) len = 8;
  if (CAN_SendStd(stdId, buf, len) == HAL_OK) {
    printf("CAN send OK: ID=0x%03X len=%d\r\n", stdId & 0x7FF, len);
  } else {
    printf("CAN send Failed: ID=0x%03X len=%d\r\n", stdId & 0x7FF, len);
  }
}

SHELL_EXPORT_CMD(can_send, CAN_Send_Cmd, send can data);

/**
 * @brief Read one CAN message from RX FIFO0 and print it to console.
 */
void CAN_Recv(void)
{
  CAN_RxHeaderTypeDef rxHeader;
  uint8_t data[8] = {0};

  uint32_t fill = HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0);
  if (fill == 0U) {
    printf("CAN_Recv: no message in FIFO0\r\n");
    return;
  }

  if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rxHeader, data) != HAL_OK) {
    printf("CAN_Recv: HAL_CAN_GetRxMessage failed\r\n");
    return;
  }

  if (rxHeader.IDE == CAN_ID_STD) {
    printf("CAN Recv: StdID=0x%03lX DLC=%d %s\r\n",
           (unsigned long)rxHeader.StdId, rxHeader.DLC,
           (rxHeader.RTR == CAN_RTR_REMOTE) ? "RTR" : "DATA");
  } else {
    printf("CAN Recv: ExtID=0x%08lX DLC=%d %s\r\n",
           (unsigned long)rxHeader.ExtId, rxHeader.DLC,
           (rxHeader.RTR == CAN_RTR_REMOTE) ? "RTR" : "DATA");
  }

  printf("Data:");
  for (uint8_t i = 0; i < rxHeader.DLC; i++) {
    printf(" %02X", data[i]);
  }
  printf("\r\n");
}

SHELL_EXPORT_CMD(can_recv, CAN_Recv, receive can data);



/* USER CODE END 1 */
