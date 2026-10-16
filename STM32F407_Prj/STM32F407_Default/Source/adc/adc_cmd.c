#include "stm32f4xx_hal.h"
#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "adc.h"
#include "serial.h"
#include "rtc_cmd.h"

static volatile uint8_t  s_templog_enable = 0;
static volatile uint32_t s_templog_period = 1000;
static volatile uint8_t  s_templog_period_set = 0;

static int32_t read_temp_x10(void)
{
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        return 0;
    }
    uint16_t raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    int32_t vsense_mv = (int32_t)raw * 3300 / 4096;
    return (vsense_mv - 760) * 2 / 5 + 250;
}

static void format_temp(int32_t temp_x10, int32_t *t_int, int32_t *t_dec)
{
    *t_int = temp_x10 / 10;
    int32_t abs_x10 = (temp_x10 < 0) ? -temp_x10 : temp_x10;
    *t_dec = abs_x10 % 10;
}

static void print_temp_detail(int32_t temp_x10)
{
    int32_t t_int, t_dec;
    format_temp(temp_x10, &t_int, &t_dec);
    printf("\r\n========== Internal Temperature ==========\r\n");
    printf("  Die temp  : %ld.%ld C\r\n", t_int, t_dec);
    printf("========================================\r\n\r\n");
}

void Temp_BackgroundUpdate(void)
{
    if (s_templog_enable && Shell_ConsumeCtrlC()) {
        s_templog_enable = 0;
        char ts[16];
        RTC_FormatTimestamp(ts, sizeof(ts));
        printf("%s [Ctrl+C] temperature logging stopped\r\n", ts);
    }

    if (!s_templog_enable) return;

    static uint32_t last_tick = 0;
    uint32_t now = HAL_GetTick();
    if ((now - last_tick) < s_templog_period) return;
    last_tick = now;

    int32_t temp_x10 = read_temp_x10();
    int32_t t_int, t_dec;
    format_temp(temp_x10, &t_int, &t_dec);
    char ts[16];
    RTC_FormatTimestamp(ts, sizeof(ts));
    printf("%s [templog] T = %ld.%ld C\r\n", ts, t_int, t_dec);
}

int temp(int argc, char **argv)
{
    if (argc < 2) {
        int32_t temp_x10 = read_temp_x10();
        print_temp_detail(temp_x10);
        return 0;
    }

    int val = atoi(argv[1]);

    if (val == 0) {
        if (s_templog_enable) {
            s_templog_enable = 0;
            printf("Temperature logging: OFF\r\n");
        } else {
            int32_t temp_x10 = read_temp_x10();
            print_temp_detail(temp_x10);
        }
    } else if (val == 1) {
        s_templog_enable = 1;
        if (!s_templog_period_set) {
            s_templog_period = 1000;
        }
        printf("Temperature logging: ON (period=%lums)\r\n", s_templog_period);
    } else {
        s_templog_period = val;
        s_templog_period_set = 1;
        s_templog_enable = 1;
        printf("Temperature logging: ON (period=%lums)\r\n", s_templog_period);
    }
    return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), temp, temp, "no_arg=read_once 0=stop 1=start period_ms=start_with_period");

void vbat(void)
{
    HAL_ADC_Start(&hadc1);

    if (HAL_ADC_PollForConversion(&hadc1, 100) != HAL_OK)
    {
        printf("ERROR: Timeout waiting for conversion 1\r\n");
        HAL_ADC_Stop(&hadc1);
        return;
    }
    HAL_ADC_GetValue(&hadc1);

    if (HAL_ADC_PollForConversion(&hadc1, 100) != HAL_OK)
    {
        printf("ERROR: Timeout waiting for conversion 2\r\n");
        HAL_ADC_Stop(&hadc1);
        return;
    }
    uint16_t raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    uint32_t vbat_mv = (uint32_t)raw * 3300UL * 2 / 4096;

    uint32_t v_int = vbat_mv / 1000;
    uint32_t v_dec = (vbat_mv % 1000) / 100;

    printf("\r\n========== VBAT Monitor ==========\r\n");
    printf("  ADC raw   : %u\r\n", raw);
    printf("  VBAT      : %lu.%lu V\r\n", v_int, v_dec);
    printf("=================================\r\n\r\n");
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), vbat, vbat, Read VBAT battery voltage);