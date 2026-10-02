#include "stm32f4xx_hal.h"
#include "shell.h"
#include "stdio.h"
#include "gpio.h"
#include "led_cmd.h"

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
    if (!s_alternate_enable) return;

    static uint32_t last_tick = 0;
    uint32_t now = HAL_GetTick();
    if ((now - last_tick) < 500) return;
    last_tick = now;

    if (s_alternate_step == 0) {
        g_led_red.ops->on(&g_led_red);
        g_led_blue.ops->off(&g_led_blue);
        s_alternate_step = 1;
    } else {
        g_led_red.ops->off(&g_led_red);
        g_led_blue.ops->on(&g_led_blue);
        s_alternate_step = 0;
    }
}

void led(int state)
{
    if (state == 1) {
        Led_Alternate_SetEnabled(1);
        printf("LED alternate blink: ON\r\n");
    } else if (state == 0) {
        Led_Alternate_SetEnabled(0);
        printf("LED alternate blink: OFF\r\n");
    } else {
        g_led_red.ops->toggle(&g_led_red);
        g_led_blue.ops->toggle(&g_led_blue);
    }
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), led, led, 0=stop 1=alternate);