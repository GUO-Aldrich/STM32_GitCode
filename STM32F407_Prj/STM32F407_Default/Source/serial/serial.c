#include "stm32f4xx_hal.h"
#include "usart.h"
#include "serial.h"
#include "shell.h"
#include "lcd_shell.h"
#include "stdint.h"
#include "stdio.h"

extern void shellEnter(Shell *shell);

Shell shell;
char shellBuffer[512];

volatile uint8_t g_ctrl_c_pressed = 0;
static volatile uint8_t s_ctrl_c_prompt_pending = 0;

#define SHELL_RX_BUF_SIZE    64
#define SHELL_TX_TIMEOUT_MS  500

static volatile uint8_t  rxCplt = 0;
static volatile uint16_t rxSize = 0;
static uint8_t buffer[SHELL_RX_BUF_SIZE];

static signed short shell_write_tee(char *bytes, unsigned short len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)bytes, len, SHELL_TX_TIMEOUT_MS);
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

void shell_task(void)
{
    if (rxCplt)
    {
        uint16_t len;

        __disable_irq();
        len = rxSize;
        rxCplt = 0;
        HAL_UARTEx_ReceiveToIdle_IT(&huart1, buffer, sizeof(buffer));
        __enable_irq();

        for (uint16_t i = 0; i < len; i++)
        {
            shellHandler(&shell, buffer[i]);
        }
    }

    if (s_ctrl_c_prompt_pending && !g_ctrl_c_pressed) {
        s_ctrl_c_prompt_pending = 0;
        shellEnter(&shell);
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == &huart1)
    {
        if (huart->ErrorCode != HAL_UART_ERROR_NONE)
        {
            HAL_UART_AbortReceive(huart);
            HAL_UARTEx_ReceiveToIdle_IT(huart, buffer, sizeof(buffer));
            return;
        }
        rxSize = Size;
        rxCplt = 1;
    }
}

PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, SHELL_TX_TIMEOUT_MS);
    if (LcdShell_IsEnabled()) {
        LcdShell_PutChar(ch);
    }
    return ch;
}

static void shell_ctrl_c(Shell *shell)
{
    g_ctrl_c_pressed = 1;
    shellWriteString(shell, "^C\r\n");
    shell->parser.buffer[0] = '\0';
    shell->parser.length = 0;
    s_ctrl_c_prompt_pending = 1;
}
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0), 0x03000000, shell_ctrl_c, ctrl+c interrupt);

uint8_t Shell_ConsumeCtrlC(void)
{
    if (g_ctrl_c_pressed) {
        g_ctrl_c_pressed = 0;
        return 1;
    }
    return 0;
}