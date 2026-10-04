#ifndef __SERIAL_H
#define __SERIAL_H

#include "log_color.h"
#include <stdint.h>

extern volatile uint8_t g_ctrl_c_pressed;

void shell_init(void);
void shell_task(void);
uint8_t Shell_ConsumeCtrlC(void);

#ifndef __GNU__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNU__ */

#endif /* __SERIAL_H */