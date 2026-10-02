#include "stm32f4xx_hal.h"
#include "usart.h"
#include "serial.h"
#include "shell.h"
#include "lcd_shell.h"
#include "stdint.h"

Shell shell;
char shellBuffer[512];
uint8_t buffer[32];

static signed short shell_write_tee(char *bytes, unsigned short len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)bytes, len, 50);
    if (LcdShell_IsEnabled()) {
        LcdShell_PutString(bytes, len);
    }
    return len;
}

void shell_init(void)
{
    shell.write = shell_write_tee;
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
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
    if (LcdShell_IsEnabled()) {
        LcdShell_PutChar(ch);
    }
    return ch;
}