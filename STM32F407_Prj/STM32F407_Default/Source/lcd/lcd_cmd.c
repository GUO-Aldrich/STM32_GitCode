#include "stm32f4xx_hal.h"
#include "shell.h"
#include "stdio.h"
#include "LCD_OLED_Style.h"
#include "lcd_shell.h"

void pixelclock_toggle(int on)
{
    if (LcdShell_IsEnabled()) {
        printf("Please disable LCD shell first: shell 0\r\n");
        return;
    }
    if (on == 1)
    {
        PixelClock_SetOn(1);
        printf("PixelClock: ON\r\n");
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

void lcd_shell_cmd(int on)
{
    if (on == 1)
    {
        PixelClock_SetOn(0);
        LcdShell_SetEnabled(1);
    }
    else if (on == 0)
    {
        LcdShell_SetEnabled(0);
        PixelClock_SetOn(1);
    }
    else
    {
        printf("Usage: shell 0 | shell 1   (current: %s)\r\n",
               LcdShell_IsEnabled() ? "ON" : "OFF");
    }
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), shell, lcd_shell_cmd, "0=off 1=on project shell to LCD");

void shell_autostart_cmd(int on)
{
    if (on == 1)
    {
        LcdShell_SetAutoStart(1);
        printf("Auto-start LCD shell on boot: ENABLED\r\n");
    }
    else if (on == 0)
    {
        LcdShell_SetAutoStart(0);
        printf("Auto-start LCD shell on boot: DISABLED\r\n");
    }
    else
    {
        printf("Usage: shell_init 0 | shell_init 1   (current: %s)\r\n",
               LcdShell_GetAutoStart() ? "ON" : "OFF");
    }
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), shell_init, shell_autostart_cmd, "0=disable 1=enable auto enter LCD shell on boot");