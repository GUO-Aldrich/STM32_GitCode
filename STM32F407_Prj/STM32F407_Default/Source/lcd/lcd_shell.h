#ifndef __LCD_SHELL_H
#define __LCD_SHELL_H

#include <stdint.h>

void LcdShell_Init(void);
void LcdShell_Clear(void);
void LcdShell_PutChar(char ch);
void LcdShell_PutString(const char *str, uint16_t len);

uint8_t LcdShell_IsEnabled(void);
void    LcdShell_SetEnabled(uint8_t on);

uint8_t LcdShell_GetAutoStart(void);
void    LcdShell_SetAutoStart(uint8_t enable);

#endif