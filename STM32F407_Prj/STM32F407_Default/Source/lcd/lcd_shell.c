#include "lcd_shell.h"
#include "LCD_ILI9341.h"
#include "rtc.h"
#include "stm32f4xx_hal.h"
#include "shell.h"
#include <stdio.h>
#include <string.h>

#define SHELL_AUTOSTART_BKP     RTC_BKP_DR2
#define SHELL_AUTOSTART_MAGIC   0xA5A5

#define FONT_SIZE       12
#define CHAR_W          (FONT_SIZE / 2)
#define CHAR_H          FONT_SIZE

static uint16_t s_cols;
static uint16_t s_rows;
static char    *s_buffer;
static uint8_t  s_row;
static uint8_t  s_col;
static uint8_t  s_enabled;
static uint8_t  s_inited;

static uint8_t s_ansi_state;
static char    s_ansi_buf[16];
static uint8_t s_ansi_len;

static void ensure_ready(void)
{
    if (s_inited) return;

    s_cols = xLCD.width / CHAR_W;
    if (s_cols == 0) s_cols = 40;
    s_rows = xLCD.height / CHAR_H;
    if (s_rows == 0) s_rows = 20;

    static char buf[1200];
    s_buffer = buf;

    memset(s_buffer, ' ', (uint32_t)s_rows * s_cols);
    s_row = 0;
    s_col = 0;
    s_ansi_state = 0;
    s_ansi_len = 0;
    s_inited = 1;
}

static void draw_row(uint16_t row)
{
    char *line = s_buffer + (uint32_t)row * s_cols;
    char tmp[64];
    uint16_t copy_len = s_cols;
    if (copy_len >= sizeof(tmp)) copy_len = sizeof(tmp) - 1;
    memcpy(tmp, line, copy_len);
    tmp[copy_len] = '\0';
    LCD_String(0, row * CHAR_H, tmp, FONT_SIZE, WHITE, BLACK);
}

static void redraw_all(void)
{
    LCD_Fill(0, 0, xLCD.width - 1, xLCD.height - 1, BLACK);
    for (uint16_t r = 0; r < s_rows; r++) draw_row(r);
}

static void term_scroll_up(void)
{
    uint32_t row_bytes = s_cols;
    memmove(s_buffer, s_buffer + row_bytes, row_bytes * (s_rows - 1));
    memset(s_buffer + row_bytes * (s_rows - 1), ' ', row_bytes);
    redraw_all();
}

static void term_clear_all(void)
{
    LCD_Fill(0, 0, xLCD.width - 1, xLCD.height - 1, BLACK);
    memset(s_buffer, ' ', (uint32_t)s_rows * s_cols);
    s_row = 0;
    s_col = 0;
}

static void term_clear_row_data(uint16_t row)
{
    memset(s_buffer + (uint32_t)row * s_cols, ' ', s_cols);
    draw_row(row);
}

void LcdShell_Init(void)
{
    s_enabled = 0;
    s_inited = 0;
}

void LcdShell_Clear(void)
{
    ensure_ready();
    term_clear_all();
}

static void handle_ansi(void)
{
    if (s_ansi_len == 0) return;

    if (s_ansi_len >= 2 && s_ansi_buf[0] == '2' && s_ansi_buf[1] == 'J') {
        term_clear_all();
    } else if (s_ansi_len >= 2 && s_ansi_buf[0] == '2' && s_ansi_buf[1] == 'K') {
        term_clear_row_data(s_row);
        s_col = 0;
    } else if (s_ansi_len >= 2 && s_ansi_buf[0] == '1' && s_ansi_buf[1] == 'H') {
        s_row = 0;
        s_col = 0;
    } else if (s_ansi_len >= 1 && s_ansi_buf[s_ansi_len - 1] == 'H') {
        s_row = 0;
        s_col = 0;
    }

    s_ansi_len = 0;
}

void LcdShell_PutChar(char ch)
{
    if (!s_enabled) return;
    ensure_ready();

    if (s_ansi_state == 1) {
        if (ch == '[') {
            s_ansi_state = 2;
            s_ansi_len = 0;
        } else {
            s_ansi_state = 0;
        }
        return;
    }
    if (s_ansi_state == 2) {
        if ((ch >= '0' && ch <= '9') || ch == ';') {
            if (s_ansi_len < sizeof(s_ansi_buf) - 1) {
                s_ansi_buf[s_ansi_len++] = ch;
            }
        } else {
            s_ansi_buf[s_ansi_len++] = ch;
            handle_ansi();
            s_ansi_state = 0;
        }
        return;
    }
    if (ch == 0x1B) {
        s_ansi_state = 1;
        return;
    }

    if (ch == '\r') {
        s_col = 0;
        return;
    }
    if (ch == '\n') {
        if (s_row + 1 >= s_rows) {
            term_scroll_up();
        } else {
            s_row++;
        }
        return;
    }
    if (ch == '\b') {
        if (s_col > 0) {
            s_col--;
            s_buffer[(uint32_t)s_row * s_cols + s_col] = ' ';
            draw_row(s_row);
        }
        return;
    }
    if (ch < 0x20 || ch > 0x7E) return;

    if (s_col >= s_cols) {
        if (s_row + 1 >= s_rows) {
            term_scroll_up();
        } else {
            s_row++;
        }
        s_col = 0;
    }

    s_buffer[(uint32_t)s_row * s_cols + s_col] = ch;
    draw_row(s_row);
    s_col++;
}

void LcdShell_PutString(const char *str, uint16_t len)
{
    if (!s_enabled) return;
    ensure_ready();

    uint8_t row_need_redraw = 0;

    for (uint16_t i = 0; i < len; i++)
    {
        char ch = str[i];

        if (s_ansi_state != 0)
        {
            if (s_ansi_state == 1) {
                if (ch == '[') { s_ansi_state = 2; s_ansi_len = 0; }
                else { s_ansi_state = 0; }
                continue;
            }
            if (s_ansi_state == 2) {
                if ((ch >= '0' && ch <= '9') || ch == ';') {
                    if (s_ansi_len < sizeof(s_ansi_buf) - 1)
                        s_ansi_buf[s_ansi_len++] = ch;
                } else {
                    s_ansi_buf[s_ansi_len++] = ch;
                    handle_ansi();
                    s_ansi_state = 0;
                    row_need_redraw = 1;
                }
                continue;
            }
        }
        if (ch == 0x1B) {
            s_ansi_state = 1;
            continue;
        }
        if (ch == '\r') {
            s_col = 0;
            continue;
        }
        if (ch == '\n') {
            if (s_row + 1 >= s_rows) term_scroll_up();
            else s_row++;
            row_need_redraw = 1;
            continue;
        }
        if (ch == '\b') {
            if (s_col > 0) {
                s_col--;
                s_buffer[(uint32_t)s_row * s_cols + s_col] = ' ';
                row_need_redraw = 1;
            }
            continue;
        }
        if (ch < 0x20 || ch > 0x7E) continue;

        if (s_col >= s_cols) {
            if (s_row + 1 >= s_rows) term_scroll_up();
            else s_row++;
            s_col = 0;
        }

        s_buffer[(uint32_t)s_row * s_cols + s_col] = ch;
        s_col++;
        row_need_redraw = 1;
    }

    if (row_need_redraw) {
        if (s_row < s_rows) draw_row(s_row);
    }
}

uint8_t LcdShell_IsEnabled(void) { return s_enabled; }

void LcdShell_SetEnabled(uint8_t on)
{
    s_enabled = on ? 1 : 0;
    if (on) {
        ensure_ready();
        term_clear_all();
    }
}

uint8_t LcdShell_GetAutoStart(void)
{
    HAL_PWR_EnableBkUpAccess();
    return (HAL_RTCEx_BKUPRead(&hrtc, SHELL_AUTOSTART_BKP) == SHELL_AUTOSTART_MAGIC) ? 1 : 0;
}

void LcdShell_SetAutoStart(uint8_t enable)
{
    HAL_PWR_EnableBkUpAccess();
    if (enable) {
        HAL_RTCEx_BKUPWrite(&hrtc, SHELL_AUTOSTART_BKP, SHELL_AUTOSTART_MAGIC);
    } else {
        HAL_RTCEx_BKUPWrite(&hrtc, SHELL_AUTOSTART_BKP, 0);
    }
}