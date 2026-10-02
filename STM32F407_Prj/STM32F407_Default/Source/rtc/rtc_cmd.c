#include "stm32f4xx_hal.h"
#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "rtc.h"

static int zeller(int y, int m, int d)
{
    if (m <= 2) { m += 12; y--; }
    int c  = y / 100;
    int yy = y % 100;
    int w  = (c / 4 - 2 * c + yy + yy / 4 + 13 * (m + 1) / 5 + d - 1) % 7;
    if (w < 0) w += 7;
    return w;
}

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

static uint16_t rtc_year_to_full(uint8_t year)
{
    return (year < 50) ? (2000 + year) : (1900 + year);
}

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