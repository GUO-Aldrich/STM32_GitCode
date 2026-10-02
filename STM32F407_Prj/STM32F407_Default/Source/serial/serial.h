#ifndef __SERIAL_H
#define __SERIAL_H

#include "log_color.h"

void shell_init();
signed short shell_write(char* bytes, unsigned short len);
void shell_parse_input(uint16_t size);

#ifndef __GNU__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNU__ */

#endif /* __SERIAL_H */