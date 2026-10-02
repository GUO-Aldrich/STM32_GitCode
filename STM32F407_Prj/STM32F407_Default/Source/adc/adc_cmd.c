#include "stm32f4xx_hal.h"
#include "shell.h"
#include "stdio.h"
#include "adc.h"

void temp(void)
{
    HAL_ADC_Start(&hadc1);
    if(HAL_ADC_PollForConversion(&hadc1, 100) != HAL_OK)
    {
        printf("ERROR: Timeout waiting for conversion\r\n");
        return;
    }
    uint16_t raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    int32_t vsense_mv = (int32_t)raw * 3300 / 4096;
    int32_t temp_x10  = (vsense_mv - 760) * 2 / 5 + 250;

    int32_t t_int = temp_x10 / 10;
    int32_t t_dec = (temp_x10 < 0 ? -temp_x10 : temp_x10) % 10;

    printf("\r\n========== Internal Temperature ==========\r\n");
    printf("  ADC raw   : %u\r\n", raw);
    printf("  Vsense    : %ld mV\r\n", vsense_mv);
    printf("  Die temp  : %ld.%ld C\r\n", t_int, t_dec);
    printf("========================================\r\n\r\n");
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), temp, temp, Read internal die temperature);