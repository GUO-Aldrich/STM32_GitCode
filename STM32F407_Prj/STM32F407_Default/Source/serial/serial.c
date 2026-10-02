#include "stm32f4xx_hal.h"
#include "usart.h"
#include "serial.h"

#include "shell.h"
#include "stdint.h"

Shell shell;
char shellBuffer[512];
uint8_t buffer[32];

signed short shell_write(char *bytes, unsigned short len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)bytes, len, 50);
    return len;
}

void shell_init(void)
{
    shell.write = shell_write;
    shellInit(&shell, shellBuffer, sizeof(shellBuffer));
    HAL_UARTEx_ReceiveToIdle_IT(&huart1, buffer, sizeof(buffer));
}

void shell_parse_input(uint16_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        shellHandler(&shell, buffer[i]);
    }
    HAL_UARTEx_ReceiveToIdle_IT(&huart1, buffer, sizeof(buffer));
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == &huart1)
    {
        shell_parse_input(Size);
    }
}

PUTCHAR_PROTOTYPE
{
    /* Place your implementation of fputc here */
    /* e.g. write a character to the EVAL_COM1 and Loop until the end of transmission */
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);

    return ch;
}