#include "stm32f2xx_hal.h"
#include "shell.h"
#include "stdio.h"
#include "gpio.h"

void func(int i, char ch, char *str)
{
    printf("input int: %d, char: %c, string: %s\r\n", i, ch, str);
}
SHELL_EXPORT_CMD(func, func, test);

void reboot(void)
{
		NVIC_SystemReset(); // Reset the microcontroller
}
SHELL_EXPORT_CMD(reboot, reboot, reboot the mcu);

int plus(int a, int b)
{
		return a + b;
}
SHELL_EXPORT_CMD(plus, plus, plus);

void led(int state)
{
	if (state == 0)
		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_10);
	else
		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_11);
}

SHELL_EXPORT_CMD(led, led, set led state);

