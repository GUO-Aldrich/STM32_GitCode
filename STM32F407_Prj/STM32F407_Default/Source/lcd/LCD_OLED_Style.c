#include "LCD_OLED_Style.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void OLED_Init(OLED_Screen_t *screen, uint16_t x, uint16_t y, uint8_t scale,
               uint16_t logical_w, uint16_t logical_h,
               uint16_t fg_color, uint16_t bg_color)
{
    screen->x         = x;
    screen->y         = y;
    screen->scale     = (scale == 0) ? 1 : scale;
    screen->width     = logical_w;
    screen->height    = logical_h;
    screen->fg_color  = fg_color;
    screen->bg_color  = bg_color;
}

void OLED_Clear(OLED_Screen_t *screen)
{
    uint16_t pw = screen->width  * screen->scale;
    uint16_t ph = screen->height * screen->scale;
    LCD_Fill(screen->x, screen->y,
             screen->x + pw - 1, screen->y + ph - 1,
             screen->bg_color);
}

void OLED_DrawPixel(OLED_Screen_t *screen, uint16_t px, uint16_t py, uint8_t on)
{
    if (px >= screen->width || py >= screen->height) return;
    uint16_t s = screen->scale;
    uint16_t x0 = screen->x + px * s;
    uint16_t y0 = screen->y + py * s;
    uint16_t color = on ? screen->fg_color : screen->bg_color;
    LCD_Fill(x0, y0, x0 + s - 1, y0 + s - 1, color);
}

void OLED_DrawGrid(OLED_Screen_t *screen, uint16_t grid_color)
{
    uint16_t s = screen->scale;
    uint16_t pw = screen->width  * s;
    uint16_t ph = screen->height * s;

    for (uint16_t gx = 0; gx <= screen->width; gx++)
    {
        uint16_t x = screen->x + gx * s;
        for (uint16_t dy = 0; dy < ph; dy++)
        {
            LCD_DrawPoint(x, screen->y + dy, grid_color);
        }
    }
    for (uint16_t gy = 0; gy <= screen->height; gy++)
    {
        uint16_t y = screen->y + gy * s;
        for (uint16_t dx = 0; dx < pw; dx++)
        {
            LCD_DrawPoint(screen->x + dx, y, grid_color);
        }
    }
}

void OLED_ShowChar(OLED_Screen_t *screen, uint16_t px, uint16_t py, char ch)
{
    if (ch < 32 || ch > 126) ch = '?';
    uint8_t num = ch - ' ';

    uint8_t  font_w = 8;
    uint8_t  font_h = 16;
    uint16_t bytes_per_char = 16;
    const uint8_t *font_ptr = &aFontASCII16[num][0];

    uint16_t y0 = py;
    for (uint16_t col = 0; col < font_w; col++)
    {
        for (uint16_t row_byte = 0; row_byte < font_h / 8; row_byte++)
        {
            uint8_t temp = font_ptr[col * 2 + row_byte];
            for (uint8_t bit = 0; bit < 8; bit++)
            {
                uint8_t on = (temp & 0x80) ? 1 : 0;
                OLED_DrawPixel(screen, px + col, py + row_byte * 8 + bit, on);
                temp <<= 1;
            }
        }
    }
    (void)y0;
    (void)bytes_per_char;
}

void OLED_ShowString(OLED_Screen_t *screen, uint16_t px, uint16_t py, const char *str)
{
    uint16_t cx = px;
    while (*str)
    {
        if (*str == '\n')
        {
            cx = px;
            py += 16;
            str++;
            continue;
        }
        OLED_ShowChar(screen, cx, py, *str);
        cx += 8;
        str++;
    }
}

void OLED_ShowHex(OLED_Screen_t *screen, uint16_t px, uint16_t py,
                  const uint8_t *data, uint16_t len, uint8_t bytes_per_row)
{
    static const char hex_tbl[] = "0123456789ABCDEF";
    uint16_t cx = px;
    uint16_t cy = py;
    for (uint16_t i = 0; i < len; i++)
    {
        if (i > 0 && (i % bytes_per_row) == 0)
        {
            cx = px;
            cy += 16;
        }
        OLED_ShowChar(screen, cx, cy, hex_tbl[(data[i] >> 4) & 0x0F]);
        OLED_ShowChar(screen, cx + 8, cy, hex_tbl[data[i] & 0x0F]);
        cx += 20;
    }
}

void OLED_PreviewDemo(uint16_t x_offset, uint16_t y_offset)
{
    OLED_Screen_t oled;
    uint8_t  scale = 4;
    uint16_t logical_w = 64;
    uint16_t logical_h = 16;

    OLED_Init(&oled,
              x_offset,
              y_offset,
              scale,
              logical_w,
              logical_h,
              OLED_FG_COLOR,
              OLED_BG_COLOR);

    OLED_Clear(&oled);

    uint16_t tw = 64 * scale;
    uint16_t th = 16 * scale;
    LCD_DrawPoint(oled.x - 1, oled.y - 1, OLED_GRID_COLOR);
    LCD_DrawPoint(oled.x + tw, oled.y - 1, OLED_GRID_COLOR);
    LCD_DrawPoint(oled.x - 1, oled.y + th, OLED_GRID_COLOR);
    LCD_DrawPoint(oled.x + tw, oled.y + th, OLED_GRID_COLOR);

    OLED_ShowString(&oled, 0, 0, "STM32");
}

/*============================================================================
 * Internal: draw one ASCII char at a RAW physical Y offset
 * (no window boundary clamp 鈥?caller controls clipping)
 *============================================================================*/
static void DrawCharPhys(OLED_Screen_t *s, uint16_t col, int16_t phys_y,
                         uint16_t clip_top, uint16_t clip_bottom, char ch)
{
    if (ch < 32 || ch > 126) ch = '?';
    uint8_t num = ch - ' ';
    const uint8_t *font = &aFontASCII16[num][0];

    uint16_t scl = s->scale;
    uint16_t px0 = s->x + col * 8 * scl;

    for (uint16_t fc = 0; fc < 8; fc++)
    {
        for (uint16_t rb = 0; rb < 2; rb++)
        {
            uint8_t b = font[fc * 2 + rb];
            for (uint8_t bit = 0; bit < 8; bit++)
            {
                if (b & 0x80)
                {
                    int32_t py = (int32_t)s->y + (int32_t)phys_y
                               + (int32_t)rb * 8 * scl + (int32_t)bit * scl;
                    int32_t py_end = py + (int32_t)scl - 1;
                    if (py_end < (int32_t)clip_top || py > (int32_t)clip_bottom)
                    {
                        b <<= 1;
                        continue;
                    }
                    uint16_t x = px0 + fc * scl;
                    uint16_t y0 = (py < (int32_t)clip_top) ? clip_top : (uint16_t)py;
                    uint16_t y1 = (py_end > (int32_t)clip_bottom)
                                  ? clip_bottom
                                  : (uint16_t)py_end;
                    LCD_Fill(x, y0, x + scl - 1, y1, s->fg_color);
                }
                b <<= 1;
            }
        }
    }
}

/*============================================================================
 * Slide-in animation (single column):
 *   new char slides DOWNWARD from ABOVE the slot into final position.
 *   phys_y goes from -char_h 鈫?0   (top off-screen 鈫?inside slot)
 *============================================================================*/
void OLED_SlideInChar(OLED_Screen_t *screen, uint16_t col, char ch)
{
    uint16_t scl       = screen->scale;
    uint16_t char_w    = 8 * scl;
    uint16_t char_h    = 16 * scl;
    uint16_t col_x0    = screen->x + col * char_w;
    uint16_t total_frames = char_h / scl;
    uint16_t delay_per_frame = 22;

    uint16_t clip_top    = screen->y;
    uint16_t clip_bottom = screen->y + char_h - 1;

    for (uint16_t frame = 0; frame <= total_frames; frame++)
    {
        LCD_Fill(col_x0, clip_top, col_x0 + char_w - 1, clip_bottom,
                 screen->bg_color);

        int16_t phys_y = -(int16_t)(frame * scl);
        DrawCharPhys(screen, col, phys_y, clip_top, clip_bottom, ch);

        HAL_Delay(delay_per_frame);
    }

    LCD_Fill(col_x0, clip_top, col_x0 + char_w - 1, clip_bottom,
             screen->bg_color);
    DrawCharPhys(screen, col, 0, clip_top, clip_bottom, ch);
}

/*============================================================================
 * PUSH animation 鈥?eliminates white-block ghosting forever.
 * Old char exits UPWARD (phys_y : 0 鈫?-char_h)
 * New char enters FROM BELOW (phys_y : char_h 鈫?0)
 * Every frame: old + new visible regions UNION to EXACTLY char_h pixels.
 * So bg_color NEVER shows through a gap.
 *============================================================================*/
void OLED_PushCols(OLED_Screen_t *screen,
                   const uint16_t *cols,
                   const char     *old_chars,
                   const char     *new_chars,
                   uint16_t        count,
                   const volatile uint8_t *abort_flag)
{
    if (count == 0) return;

    uint16_t scl          = screen->scale;
    uint16_t char_w       = 8 * scl;
    uint16_t char_h       = 16 * scl;
    uint16_t total_frames = char_h / scl;
    uint16_t delay_per_frame = 18;

    uint16_t clip_top    = screen->y;
    uint16_t clip_bottom = screen->y + char_h - 1;

    for (uint16_t frame = 0; frame <= total_frames; frame++)
    {
        if (abort_flag && !*abort_flag) return;

        int16_t old_y = -(int16_t)(frame * scl);
        int16_t new_y = (int16_t)char_h - (int16_t)(frame * scl);

        for (uint16_t i = 0; i < count; i++)
        {
            uint16_t px0 = screen->x + cols[i] * char_w;
            LCD_Fill(px0, clip_top, px0 + char_w - 1, clip_bottom, screen->bg_color);
            DrawCharPhys(screen, cols[i], old_y, clip_top, clip_bottom, old_chars[i]);
            DrawCharPhys(screen, cols[i], new_y, clip_top, clip_bottom, new_chars[i]);
        }

        HAL_Delay(delay_per_frame);
    }

    if (abort_flag && !*abort_flag) return;

    for (uint16_t i = 0; i < count; i++)
    {
        uint16_t px0 = screen->x + cols[i] * char_w;
        LCD_Fill(px0, clip_top, px0 + char_w - 1, clip_bottom, screen->bg_color);
        DrawCharPhys(screen, cols[i], 0, clip_top, clip_bottom, new_chars[i]);
    }
}

/*============================================================================
 * Synchronous slide-in for multiple columns (kept for backward compat).
 *============================================================================*/
void OLED_SlideInCols(OLED_Screen_t *screen,
                      const uint16_t *cols,
                      const char     *chars,
                      uint16_t        count)
{
    if (count == 0) return;

    uint16_t scl          = screen->scale;
    uint16_t char_w       = 8 * scl;
    uint16_t char_h       = 16 * scl;
    uint16_t total_frames = char_h / scl;
    uint16_t delay_per_frame = 22;

    uint16_t clip_top    = screen->y;
    uint16_t clip_bottom = screen->y + char_h - 1;

    for (uint16_t i = 0; i < count; i++)
    {
        uint16_t px0 = screen->x + cols[i] * char_w;
        LCD_Fill(px0, clip_top, px0 + char_w - 1, clip_bottom,
                 screen->bg_color);
    }

    for (uint16_t frame = 0; frame <= total_frames; frame++)
    {
        int16_t phys_y = -(int16_t)(frame * scl);

        for (uint16_t i = 0; i < count; i++)
        {
            uint16_t px0 = screen->x + cols[i] * char_w;
            LCD_Fill(px0, clip_top, px0 + char_w - 1, clip_bottom,
                     screen->bg_color);
            DrawCharPhys(screen, cols[i], phys_y, clip_top, clip_bottom, chars[i]);
        }

        HAL_Delay(delay_per_frame);
    }

    for (uint16_t i = 0; i < count; i++)
    {
        uint16_t px0 = screen->x + cols[i] * char_w;
        LCD_Fill(px0, clip_top, px0 + char_w - 1, clip_bottom,
                 screen->bg_color);
        DrawCharPhys(screen, cols[i], 0, clip_top, clip_bottom, chars[i]);
    }
}

/*============================================================================
 * Big pixel-style digital clock (xt command)
 *   Pure digital 鈥?no analog dial, no hands.
 *   Big scale so each digit is chunky and visible from across the room.
 *============================================================================*/
static OLED_Screen_t s_pix_time;
static OLED_Screen_t s_pix_date;
static uint8_t       s_pix_on        = 0;
static uint8_t       s_pix_last_sec  = 0xFF;
static uint32_t      s_pix_last_date = 0xFFFFFFFF;
static char          s_pix_last_tbuf[9] = {0};

static uint8_t  s_pix_push_active = 0;
static uint16_t s_pix_push_frame  = 0;
static uint16_t s_pix_push_total  = 0;
static uint16_t s_pix_push_cols[8];
static char     s_pix_push_oldc[8];
static char     s_pix_push_newc[8];
static uint16_t s_pix_push_cnt    = 0;

void PixelClock_SetOn(uint8_t on)
{
    if (on == s_pix_on) return;
    s_pix_on = on;

    if (on)
    {
        uint16_t w = xLCD.width;
        uint16_t h = xLCD.height;
        uint16_t scl     = 5;
        uint16_t time_w  = 8 * 8 * scl;
        uint16_t time_h  = 16 * scl;
        uint16_t date_w  = 10 * 8 * 2;
        uint16_t date_h  = 16 * 2;
        uint16_t gap     = 16;
        uint16_t total_h = time_h + gap + date_h;
        uint16_t time_x  = (w - time_w) / 2;
        uint16_t date_x  = (w - date_w) / 2;
        uint16_t time_y  = (h - total_h) / 2;
        uint16_t date_y  = time_y + time_h + gap;

        LCD_Fill(0, 0, w - 1, h - 1, BLACK);

        OLED_Init(&s_pix_time, time_x, time_y, scl,
                  64, 16, OLED_FG_COLOR, OLED_BG_COLOR);
        OLED_Clear(&s_pix_time);

        OLED_Init(&s_pix_date, date_x, date_y, 2,
                  80, 16, OLED_FG_COLOR, OLED_BG_COLOR);
        OLED_Clear(&s_pix_date);

        s_pix_last_sec  = 0xFF;
        s_pix_last_date = 0xFFFFFFFF;
        memset(s_pix_last_tbuf, 0, sizeof(s_pix_last_tbuf));

        s_pix_push_active = 0;
        s_pix_push_frame  = 0;
        s_pix_push_cnt    = 0;
    }
    else
    {
        LCD_Fill(0, 0, xLCD.width - 1, xLCD.height - 1, BLACK);
    }
}

uint8_t PixelClock_IsOn(void) { return s_pix_on; }


static void pix_push_step(void)
{
    uint16_t scl       = s_pix_time.scale;
    uint16_t char_w    = 8 * scl;
    uint16_t char_h    = 16 * scl;
    uint16_t clip_top  = s_pix_time.y;
    uint16_t clip_bot  = s_pix_time.y + char_h - 1;

    uint16_t frame = s_pix_push_frame;

    if (frame <= s_pix_push_total)
    {
        int16_t old_y = -(int16_t)(frame * scl);
        int16_t new_y = (int16_t)char_h - (int16_t)(frame * scl);

        for (uint16_t i = 0; i < s_pix_push_cnt; i++)
        {
            uint16_t px0 = s_pix_time.x + s_pix_push_cols[i] * char_w;
            LCD_Fill(px0, clip_top, px0 + char_w - 1, clip_bot, BLACK);
            DrawCharPhys(&s_pix_time, s_pix_push_cols[i], old_y,
                         clip_top, clip_bot, s_pix_push_oldc[i]);
            DrawCharPhys(&s_pix_time, s_pix_push_cols[i], new_y,
                         clip_top, clip_bot, s_pix_push_newc[i]);
        }
        s_pix_push_frame++;
    }
    else
    {
        for (uint16_t i = 0; i < s_pix_push_cnt; i++)
        {
            uint16_t px0 = s_pix_time.x + s_pix_push_cols[i] * char_w;
            LCD_Fill(px0, clip_top, px0 + char_w - 1, clip_bot, BLACK);
            DrawCharPhys(&s_pix_time, s_pix_push_cols[i], 0,
                         clip_top, clip_bot, s_pix_push_newc[i]);
        }
        s_pix_push_active = 0;
    }
}

void PixelClock_Render(uint8_t h, uint8_t m, uint8_t s,
                       uint8_t year, uint8_t month, uint8_t day)
{
    if (!s_pix_on) return;

    if (s != s_pix_last_sec)
    {
        char buf[9];
        uint16_t cnt = 0;
        uint16_t i;

        buf[0] = '0' + h / 10;
        buf[1] = '0' + h % 10;
        buf[2] = ':';
        buf[3] = '0' + m / 10;
        buf[4] = '0' + m % 10;
        buf[5] = ':';
        buf[6] = '0' + s / 10;
        buf[7] = '0' + s % 10;
        buf[8] = '\0';

        if (s_pix_last_sec == 0xFF)
        {
            OLED_ShowString(&s_pix_time, 0, 0, buf);
        }
        else
        {
            for (i = 0; i < 8; i++)
            {
                if (buf[i] != s_pix_last_tbuf[i] && cnt < 8)
                {
                    s_pix_push_cols[cnt] = i;
                    s_pix_push_oldc[cnt] = s_pix_last_tbuf[i];
                    s_pix_push_newc[cnt] = buf[i];
                    cnt++;
                }
            }
            if (cnt > 0)
            {
                s_pix_push_cnt   = cnt;
                s_pix_push_frame = 0;
                s_pix_push_total = 16;
                s_pix_push_active = 1;
            }
        }
        memcpy(s_pix_last_tbuf, buf, 9);
        s_pix_last_sec = s;
    }

    {
        uint32_t dc = ((uint32_t)year << 16) | ((uint32_t)month << 8) | (uint32_t)day;
        if (dc != s_pix_last_date)
        {
            char dbuf[16];
            dbuf[0]  = '2';
            dbuf[1]  = '0';
            dbuf[2]  = '0' + year / 10;
            dbuf[3]  = '0' + year % 10;
            dbuf[4]  = '/';
            dbuf[5]  = '0' + month / 10;
            dbuf[6]  = '0' + month % 10;
            dbuf[7]  = '/';
            dbuf[8]  = '0' + day / 10;
            dbuf[9]  = '0' + day % 10;
            dbuf[10] = '\0';
            OLED_Clear(&s_pix_date);
            OLED_ShowString(&s_pix_date, 0, 0, dbuf);
            s_pix_last_date = dc;
        }
    }

    {
        static uint32_t s_last_tick = 0;
        uint32_t now = HAL_GetTick();
        if (now - s_last_tick >= 40)
        {
            s_last_tick = now;
            if (s_pix_push_active) pix_push_step();
        }
    }
}