#ifndef __LED_CMD_H
#define __LED_CMD_H

#include "led.h"
#include <stdint.h>

extern Led g_led_red;
extern Led g_led_blue;

void    Led_Cmd_Init(void);

uint8_t Led_Alternate_IsEnabled(void);
void    Led_Alternate_SetEnabled(uint8_t enable);
void    Led_Alternate_Update(void);

#endif