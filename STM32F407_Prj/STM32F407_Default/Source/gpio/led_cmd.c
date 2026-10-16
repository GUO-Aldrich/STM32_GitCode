#include "stm32f4xx_hal.h"
#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "gpio.h"
#include "led_cmd.h"
#include "serial.h"
#include "rtc_cmd.h"

Led g_led_red;
Led g_led_blue;

static volatile uint8_t s_alternate_enable = 0;
static volatile uint8_t s_alternate_step   = 0;

void Led_Cmd_Init(void)
{
    Led_Init(&g_led_blue,   GPIOB, GPIO_PIN_2, 0);
    Led_Init(&g_led_red, GPIOC, GPIO_PIN_5, 0);
}

uint8_t Led_Alternate_IsEnabled(void) { return s_alternate_enable; }
void    Led_Alternate_SetEnabled(uint8_t enable)
{
    s_alternate_enable = enable ? 1 : 0;
    if (!enable) {
        g_led_red.ops->off(&g_led_red);
        g_led_blue.ops->off(&g_led_blue);
    }
}

void Led_Alternate_Update(void)
{
    if (s_alternate_enable && Shell_ConsumeCtrlC()) {
        Led_Alternate_SetEnabled(0);
        char ts[16];
        RTC_FormatTimestamp(ts, sizeof(ts));
        printf("%s [Ctrl+C] LED alternate blink: OFF\r\n", ts);
    }

    if (!s_alternate_enable) return;

    static uint32_t last_tick = 0;
    uint32_t now = HAL_GetTick();
    if ((now - last_tick) < 500) return;
    last_tick = now;

    char ts[16];
    RTC_FormatTimestamp(ts, sizeof(ts));
    if (s_alternate_step == 0) {
        g_led_red.ops->on(&g_led_red);
        g_led_blue.ops->off(&g_led_blue);
        printf("%s LED red ON, blue OFF\r\n", ts);
        s_alternate_step = 1;
    } else {
        g_led_red.ops->off(&g_led_red);
        g_led_blue.ops->on(&g_led_blue);
        printf("%s LED red OFF, blue ON\r\n", ts);
        s_alternate_step = 0;
    }
}

int led(int argc, char **argv)
{
    if (argc < 2) {
        printf("LED alternate: %s (use led 1=start, 0=stop, 2=toggle)\r\n",
               s_alternate_enable ? "ON" : "OFF");
        return 0;
    }

    int val = atoi(argv[1]);
    if (val == 1) {
        Led_Alternate_SetEnabled(1);
        printf("LED alternate blink: ON\r\n");
    } else if (val == 0) {
        Led_Alternate_SetEnabled(0);
        printf("LED alternate blink: OFF\r\n");
    } else {
        g_led_red.ops->toggle(&g_led_red);
        g_led_blue.ops->toggle(&g_led_blue);
    }
    return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), led, led, 0=stop 1=alternate);