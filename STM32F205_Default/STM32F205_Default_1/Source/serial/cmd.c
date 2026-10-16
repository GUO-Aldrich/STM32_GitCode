#include "stm32f2xx_hal.h"
#include "shell.h"
#include "stdio.h"
#include "serial.h"
#include "gpio.h"

/* 读取 STM32F2 的 Flash Size 寄存器（单位：KB） */
#define FLASH_SIZE_REG   (*(volatile uint16_t*)0x1FFF7A22)

/* 读取 DBGMCU_IDCODE 获取芯片 ID */
#define DBGMCU_IDCODE    (*(volatile uint32_t*)0xE0042000)

// 重启设备
void reboot(void)
{
	HAL_NVIC_SystemReset(); // Reset the microcontroller
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), reboot, reboot, the mcu);

// 加法运算
int plus(int a, int b)
{
	return a + b;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), plus, plus, test);

// 加法运算
void Debug(uint8_t i)
{
    while(i)
    {
        printf("Error: 404 Not Found\r\n");
        HAL_Delay(1000);
    }
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), Debug, Debug, test);

// LED控制
void led(int state)
{
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_10);
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_11);
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), led, led, test);

void ms_to_hms(uint32_t ms, char *buf)
{
    uint32_t total_seconds = ms / 1000;
    uint32_t hours   = total_seconds / 3600;
    uint32_t minutes = (total_seconds % 3600) / 60;
    uint32_t seconds = total_seconds % 60;

    sprintf(buf, "%02lu:%02lu:%02lu", hours, minutes, seconds);
}

int time(void)
{
	char time_str[9];
	ms_to_hms(HAL_GetTick(), time_str);
	printf("系统运行时间: %s\r\n", time_str);
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), time, time, test);

void show_chip_info(int argc, char *argv[])
{
    uint32_t dev_id = DBGMCU_IDCODE & 0x0FFF;
    uint32_t rev_id = (DBGMCU_IDCODE >> 16) & 0xFFFF;
    uint32_t flash_kb = FLASH_SIZE_REG;
    uint32_t sysclk = HAL_RCC_GetSysClockFreq();
    uint32_t ms = HAL_GetTick();
    uint32_t total_sec = ms / 1000;
    uint32_t days  = total_sec / 86400;
    uint32_t hours = (total_sec % 86400) / 3600;
    uint32_t mins  = (total_sec % 3600) / 60;
    uint32_t secs  = total_sec % 60;

    printf("\r\n========== STM32F205VET6 Info ==========\r\n");
    printf("  Chip ID    : 0x%03lX  (Rev: 0x%04lX)\r\n", dev_id, rev_id);
    printf("  Flash      : %lu KB\r\n", flash_kb);
    printf("  RAM        : 128 KB (SRAM1) + 64 KB (SRAM2)\r\n");
    printf("  Core       : ARM Cortex-M3 @ %lu Hz (%.1f MHz)\r\n",
           sysclk, (float)sysclk / 1000000.0f);
    printf("  HCLK       : %lu Hz\r\n", HAL_RCC_GetHCLKFreq());
    printf("  PCLK1      : %lu Hz\r\n", HAL_RCC_GetPCLK1Freq());
    printf("  PCLK2      : %lu Hz\r\n", HAL_RCC_GetPCLK2Freq());
    printf("  Uptime     : ");
    if (days > 0) printf("%lud ", days);
    printf("%02lu:%02lu:%02lu\r\n", hours, mins, secs);
    printf("==========================================\r\n\r\n");
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), cfg, show_chip_info, test);


int varInt = 0;
SHELL_EXPORT_VAR(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_VAR_INT), varInt, &varInt, test);

char str[] = "test string";
SHELL_EXPORT_VAR(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_VAR_STRING), varStr, str, test);
