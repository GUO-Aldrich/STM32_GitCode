/**
  ******************************************************************************
  * @file    cmd.c
  * @brief   Shell 自定义命令实现
  *
  * 本文件包含所有通过 SHELL_EXPORT_CMD / SHELL_EXPORT_VAR 导出到
  * Letter Shell 的用户命令和变量。编译后自动收集到 .shell_command section。
  *
  * 修改说明 (2026-10-02):
  *   - RTC 读写统一使用 RTC_FORMAT_BIN，HAL 自动做 BCD 转换
  *   - rtc_set 用 Zeller 公式自动计算星期，不再硬编码 MONDAY
  *   - 日期显示改为 YYYY-MM-DD 标准格式
  ******************************************************************************
  */

#include "stm32f4xx_hal.h"
#include "shell.h"
#include "stdio.h"
#include "serial.h"
#include "gpio.h"
#include "stdlib.h"
#include "rtc.h"
#include "adc.h"
#include "LCD_OLED_Style.h"

/*============================================================================
 * 硬件寄存器定义
 *============================================================================*/

#define FLASH_SIZE_REG   (*(volatile uint16_t*)0x1FFF7A22)
#define DBGMCU_IDCODE    (*(volatile uint32_t*)0xE0042000)

/*============================================================================
 * 通用工具函数
 *============================================================================*/

/**
  * @brief  将系统运行毫秒数转换为 HH:MM:SS 字符串
  * @param  ms  : HAL_GetTick() 返回值
  * @param  buf : 输出缓冲区 (至少 9 字节)
  * @retval None
  */
static void ms_to_hms(uint32_t ms, char *buf)
{
    uint32_t total_seconds = ms / 1000;
    uint32_t hours   = total_seconds / 3600;
    uint32_t minutes = (total_seconds % 3600) / 60;
    uint32_t seconds = total_seconds % 60;

    sprintf(buf, "%02lu:%02lu:%02lu", hours, minutes, seconds);
}

/**
  * @brief  计算星期几 (Zeller 同余公式, C 语言版本)
  * @param  y : 公历年, 比如 2026
  * @param  m : 月份 1-12
  * @param  d : 日 1-31
  * @retval 0=周日, 1=周一, ..., 6=周六 (与 RTC_WEEKDAY_xxx 定义对齐)
  */
static int zeller(int y, int m, int d)
{
    if (m <= 2) { m += 12; y--; }
    int c  = y / 100;
    int yy = y % 100;
    int w  = (c / 4 - 2 * c + yy + yy / 4 + 13 * (m + 1) / 5 + d - 1) % 7;
    if (w < 0) w += 7;
    return w;
}

/**
  * @brief  将 Zeller 返回值映射为 HAL RTC_WEEKDAY_xxx 枚举
  * @param  zeller_w : 0=周日, 1=周一, ..., 6=周六
  * @retval RTC_WEEKDAY_xxx 值
  */
static uint8_t zeller_to_rtc_weekday(int zeller_w)
{
    static const uint8_t map[] = {
        [0] = RTC_WEEKDAY_SUNDAY,
        [1] = RTC_WEEKDAY_MONDAY,
        [2] = RTC_WEEKDAY_TUESDAY,
        [3] = RTC_WEEKDAY_WEDNESDAY,
        [4] = RTC_WEEKDAY_THURSDAY,
        [5] = RTC_WEEKDAY_FRIDAY,
        [6] = RTC_WEEKDAY_SATURDAY,
    };
    return map[zeller_w];
}

/**
  * @brief  根据 HAL WeekDay 枚举返回星期英文缩写
  * @param  w : RTC_WEEKDAY_xxx 值
  * @retval "Mon"/"Tue"/.../"Sun" 或 "???"
  */
static const char *weekday_name(uint8_t w)
{
    switch (w) {
        case RTC_WEEKDAY_MONDAY:    return "Mon";
        case RTC_WEEKDAY_TUESDAY:   return "Tue";
        case RTC_WEEKDAY_WEDNESDAY: return "Wed";
        case RTC_WEEKDAY_THURSDAY:  return "Thu";
        case RTC_WEEKDAY_FRIDAY:    return "Fri";
        case RTC_WEEKDAY_SATURDAY:  return "Sat";
        case RTC_WEEKDAY_SUNDAY:    return "Sun";
        default: return "???";
    }
}

/**
  * @brief  将 RTC 结构体中的 Year (0-99) 转完整四位年份
  * @param  year : date.Year 字段 (BIN 格式读回, 十进制)
  * @retval 完整年份, 比如 2026
  */
static uint16_t rtc_year_to_full(uint8_t year)
{
    return (year < 50) ? (2000 + year) : (1900 + year);
}

/*============================================================================
 * Shell 命令: reboot
 *============================================================================*/

/**
  * @brief  软件重启 MCU
  * @param  无参数
  * @retval None (函数不会返回)
  */
void reboot(void)
{
    HAL_NVIC_SystemReset();
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), reboot, reboot, the mcu);

/*============================================================================
 * Shell 命令: plus a b
 *============================================================================*/

/**
  * @brief  简单加法, 用于测试 Shell 参数传递
  * @param  a : 加数 1
  * @param  b : 加数 2
  * @retval a + b
  */
int plus(int a, int b)
{
    return a + b;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), plus, plus, test);

/*============================================================================
 * Shell 命令: Debug
 *============================================================================*/

/*============================================================================
 * LED 交替闪烁控制 (给主循环用)
 *============================================================================*/

volatile uint8_t led_alternate_enable = 0;   // 1=交替闪烁中, 0=停止
volatile uint8_t led_alternate_step   = 0;   // 内部状态 (0=PB2亮, 1=PC5亮)

/*============================================================================
 * Shell 命令: led [0|1]
 *============================================================================*/

/**
  * @brief  控制 LED 行为
  * @param  state : 0=停止交替闪烁, 1=开始交替闪烁
  *                无参数或其他值 = 单次翻转
  * @retval None
  *
  * 注意: Shell 命令运行在中断上下文, 只设标志, 真正的闪烁
  *       逻辑在 main.c 的主循环里执行, 避免阻塞中断
  */
void led(int state)
{
    if (state == 1) {
        led_alternate_enable = 1;
        printf("LED alternate blink: ON\r\n");
    } else if (state == 0) {
        led_alternate_enable = 0;
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
        printf("LED alternate blink: OFF\r\n");
    } else {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_5);
    }
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), led, led, 0=stop 1=alternate);

/*============================================================================
 * Shell 命令: time
 *============================================================================*/

/**
  * @brief  打印系统自启动以来的运行时长 (基于 HAL_GetTick)
  * @param  无参数
  * @retval None
  */
void time(void)
{
    char time_str[9];
    ms_to_hms(HAL_GetTick(), time_str);
    printf("系统运行时间: %s\r\n", time_str);
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), time, time, test);

/*============================================================================
 * Shell 命令: cfg
 *============================================================================*/

/**
  * @brief  打印芯片完整信息: ID, Flash, RAM, 时钟频率, 运行时长
  * @param  argc / argv : 未使用
  * @retval None
  */
void show_chip_info(int argc, char *argv[])
{
    uint32_t dev_id   = DBGMCU_IDCODE & 0x0FFF;
    uint32_t rev_id   = (DBGMCU_IDCODE >> 16) & 0xFFFF;
    uint32_t flash_kb = FLASH_SIZE_REG;
    uint32_t sysclk   = HAL_RCC_GetSysClockFreq();
    uint32_t ms       = HAL_GetTick();
    uint32_t total_sec = ms / 1000;
    uint32_t days  = total_sec / 86400;
    uint32_t hours = (total_sec % 86400) / 3600;
    uint32_t mins  = (total_sec % 3600) / 60;
    uint32_t secs  = total_sec % 60;

    printf("\r\n========== STM32F407VET6 Info ==========\r\n");
    printf("  Chip ID    : 0x%03lX  (Rev: 0x%04lX)\r\n", dev_id, rev_id);
    printf("  Flash      : %lu KB\r\n", flash_kb);
    printf("  RAM        : 128 KB (SRAM1) + 64 KB (SRAM2)\r\n");
    printf("  Core       : ARM Cortex-M4 @ %lu Hz (%.1f MHz)\r\n",
           sysclk, (float)sysclk / 1000000.0f);
    printf("  HCLK       : %lu Hz\r\n", HAL_RCC_GetHCLKFreq());
    printf("  PCLK1      : %lu Hz\r\n", HAL_RCC_GetPCLK1Freq());
    printf("  PCLK2      : %lu Hz\r\n", HAL_RCC_GetPCLK2Freq());
    printf("  Uptime     : ");
    if (days > 0) printf("%lud ", days);
    printf("%02lu:%02lu:%02lu\r\n", hours, mins, secs);
    printf("==========================================\r\n\r\n");
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), cfg, show_chip_info, test);

/*============================================================================
 * Shell 命令: rtc
 *============================================================================*/

/**
  * @brief  读取并打印 RTC 当前日期和时间
  *
  *  使用 RTC_FORMAT_BIN 读取, HAL 自动把寄存器里的 BCD 编码转成十进制值
  *  WeekDay 字段 HAL 不做转换 (寄存器本身存的是 1-7 二进制), 直接使用
  *
  * @param  无参数
  * @retval None
  */
void rtc_read(void)
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

    uint16_t full_year = rtc_year_to_full(date.Year);

    printf("\r\n========== RTC Calendar ==========\r\n");
    printf("  Date  : %04u-%02u-%02u (%s)\r\n",
           full_year, date.Month, date.Date, weekday_name(date.WeekDay));
    printf("  Time  : %02u:%02u:%02u\r\n",
           time.Hours, time.Minutes, time.Seconds);
    printf("==================================\r\n\r\n");
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), rtc, rtc_read, Read RTC date & time);

/*============================================================================
 * Shell 命令: rtc_set yyyy mm dd hh mm ss
 *============================================================================*/

/**
  * @brief  设置 RTC 日期和时间, 并写入备份寄存器防止复位覆盖
  *
  *  用法: rtc_set 2026 10 02 16 30 00
  *  范围: 年 2000-2099, 月 1-12, 日 1-31, 时 0-23, 分/秒 0-59
  *  星期: 自动用 Zeller 公式计算, 无需手动指定
  *
  * @param  argv[1]: yyyy 四位年份
  * @param  argv[2]: mm   月份
  * @param  argv[3]: dd   日期
  * @param  argv[4]: hh   小时 (24h 制)
  * @param  argv[5]: mm   分钟
  * @param  argv[6]: ss   秒
  * @retval None
  */
void rtc_set(int argc, char *argv[])
{
    if (argc < 7) {
        printf("Usage: rtc_set yyyy mm dd hh mm ss\r\n");
        printf("  e.g. rtc_set 2026 10 02 16 30 00\r\n");
        return;
    }

    int year  = atoi(argv[1]);
    int month = atoi(argv[2]);
    int day   = atoi(argv[3]);
    int hh    = atoi(argv[4]);
    int mm    = atoi(argv[5]);
    int ss    = atoi(argv[6]);

    if (year < 2000 || year > 2099 || month < 1 || month > 12 ||
        day < 1 || day > 31 || hh > 23 || mm > 59 || ss > 59) {
        printf("ERROR: Invalid date/time value\r\n");
        return;
    }

    RTC_DateTypeDef d = {0};
    d.Year   = (year >= 2050) ? (year - 1900) : (year - 2000);
    d.Month  = month;
    d.Date   = day;
    d.WeekDay = zeller_to_rtc_weekday(zeller(year, month, day));

    RTC_TimeTypeDef t = {0};
    t.Hours    = hh;
    t.Minutes  = mm;
    t.Seconds  = ss;
    t.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    t.StoreOperation = RTC_STOREOPERATION_RESET;

    HAL_RTC_SetDate(&hrtc, &d, RTC_FORMAT_BIN);
    HAL_RTC_SetTime(&hrtc, &t, RTC_FORMAT_BIN);

    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x1234);

    printf("RTC set to: %04d-%02d-%02d %02d:%02d:%02d (%s)\r\n",
           year, month, day, hh, mm, ss,
           weekday_name(d.WeekDay));
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), rtc_set, rtc_set, Set RTC: rtc_set yyyy mm dd hh mm ss);

/*============================================================================
 * Shell 命令: temp
 *============================================================================*/

/**
  * @brief  读取 STM32 内部温度传感器并打印 die 温度
  *
  *  使用公式 (参考 STM32F4 参考手册 RM0090):
  *    T(°C) = {(Vsense - V25) / Avg_Slope} + 25
  *  其中:
  *    V25       = 0.76V   (Vsense 在 25°C 的典型值)
  *    Avg_Slope = 2.5mV/°C (典型值)
  *    Vsense    = raw × 3.3V / 4096
  *
  *  CubeMX 已配置 ADC1 Channel 16 (TempSensor), 480 cycles 采样时间
  *
  *  @param  无参数
  *  @retval None
  */
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

/*============================================================================
 * Shell command: t 1 / t 0
 *============================================================================*/
void clock_toggle(int on)
{
    if (on == 1)
    {
        if (PixelClock_IsOn()) PixelClock_SetOn(0);
        Clock_SetOn(1);
        printf("Clock: ON  (dial + digital)\r\n");
    }
    else if (on == 0)
    {
        Clock_SetOn(0);
        printf("Clock: OFF\r\n");
    }
    else
    {
        printf("Usage: t 0 | t 1   (current: %s)\r\n",
               Clock_IsOn() ? "ON" : "OFF");
    }
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), t, clock_toggle, 0=off 1=on);

/*============================================================================
 * Shell command: xt 1 / xt 0   (big pixel clock, no dial)
 *============================================================================*/
void pixelclock_toggle(int on)
{
    if (on == 1)
    {
        if (Clock_IsOn()) Clock_SetOn(0);
        PixelClock_SetOn(1);
        printf("PixelClock: ON  (big digital, pixel style)\r\n");
    }
    else if (on == 0)
    {
        PixelClock_SetOn(0);
        printf("PixelClock: OFF\r\n");
    }
    else
    {
        printf("Usage: xt 0 | xt 1   (current: %s)\r\n",
               PixelClock_IsOn() ? "ON" : "OFF");
    }
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), xt, pixelclock_toggle, 0=off 1=on);

/*============================================================================
 * Shell 导出变量
 *============================================================================*/

int varInt = 0;
SHELL_EXPORT_VAR(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_VAR_INT), varInt, &varInt, test);

char str[] = "test string";
SHELL_EXPORT_VAR(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_VAR_STRING), varStr, str, test);