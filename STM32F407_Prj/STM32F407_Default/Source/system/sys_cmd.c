#include "stm32f4xx_hal.h"
#include "shell.h"
#include "stdio.h"

#define FLASH_SIZE_REG   (*(volatile uint16_t*)0x1FFF7A22)
#define DBGMCU_IDCODE    (*(volatile uint32_t*)0xE0042000)

static void print_uptime(void)
{
    uint32_t total_sec = HAL_GetTick() / 1000;
    uint32_t days  = total_sec / 86400;
    uint32_t hours = (total_sec % 86400) / 3600;
    uint32_t mins  = (total_sec % 3600) / 60;
    uint32_t secs  = total_sec % 60;
    if (days > 0) printf("%lud ", days);
    printf("%02lu:%02lu:%02lu", hours, mins, secs);
}

void reboot(void)
{
    HAL_NVIC_SystemReset();
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), reboot, reboot, the mcu);

void show_chip_info(int argc, char *argv[])
{
    uint32_t dev_id   = DBGMCU_IDCODE & 0x0FFF;
    uint32_t rev_id   = (DBGMCU_IDCODE >> 16) & 0xFFFF;

    printf("\r\n========== STM32F407VET6 Info ==========\r\n");
    printf("  Chip ID    : 0x%03lX  (Rev: 0x%04lX)\r\n", dev_id, rev_id);
    printf("  Flash      : %u KB\r\n", FLASH_SIZE_REG);
    printf("  RAM        : 128 KB (SRAM1) + 64 KB (SRAM2)\r\n");
    printf("  Core       : ARM Cortex-M4 @ %.1f MHz\r\n",
           (float)HAL_RCC_GetSysClockFreq() / 1000000.0f);
    printf("  HCLK       : %lu Hz\r\n", HAL_RCC_GetHCLKFreq());
    printf("  PCLK1      : %lu Hz\r\n", HAL_RCC_GetPCLK1Freq());
    printf("  PCLK2      : %lu Hz\r\n", HAL_RCC_GetPCLK2Freq());
    printf("  Uptime     : ");
    print_uptime();
    printf("\r\n==========================================\r\n\r\n");
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), cfg, show_chip_info, the mcu);

void sys_time(void)
{
    printf("系统运行时间: ");
    print_uptime();
    printf("\r\n");
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), time, sys_time, the mcu);