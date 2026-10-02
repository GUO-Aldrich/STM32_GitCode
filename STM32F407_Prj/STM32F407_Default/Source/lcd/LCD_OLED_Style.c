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
 * (no window boundary clamp — caller controls clipping)
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
 *   phys_y goes from -char_h → 0   (top off-screen → inside slot)
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
 * PUSH animation — eliminates white-block ghosting forever.
 * Old char exits UPWARD (phys_y : 0 → -char_h)
 * New char enters FROM BELOW (phys_y : char_h → 0)
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
 * Analog clock helper: draw a line of given thickness by drawing multiple
 * parallel 1px lines offset perpendicular to the main direction.
 *============================================================================*/
static void Clock_ThickLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                            uint16_t thickness, uint16_t color)
{
    int32_t dx = (int32_t)x2 - x1;
    int32_t dy = (int32_t)y2 - y1;
    int32_t len = sqrtf((float)(dx*dx + dy*dy));
    if (len == 0) { LCD_DrawPoint(x1, y1, color); return; }

    float nx = -(float)dy / len;
    float ny =  (float)dx / len;

    uint16_t half = thickness / 2;
    for (int16_t i = -(int16_t)half; i <= (int16_t)half; i++)
    {
        uint16_t ox1 = (uint16_t)((float)x1 + nx * i + 0.5f);
        uint16_t oy1 = (uint16_t)((float)y1 + ny * i + 0.5f);
        uint16_t ox2 = (uint16_t)((float)x2 + nx * i + 0.5f);
        uint16_t oy2 = (uint16_t)((float)y2 + ny * i + 0.5f);
        LCD_Line(ox1, oy1, ox2, oy2, color);
    }
}

static void DrawHand(uint16_t cx, uint16_t cy,
                     float angle_deg, float len,
                     uint16_t thick, uint16_t color)
{
    float rad = angle_deg * 3.14159265f / 180.0f - 3.14159265f / 2.0f;
    uint16_t tx = (uint16_t)((float)cx + cosf(rad) * len + 0.5f);
    uint16_t ty = (uint16_t)((float)cy + sinf(rad) * len + 0.5f);
    Clock_ThickLine(cx, cy, tx, ty, thick, color);
}

/*============================================================================
 * Clock face — draw once at startup
 *============================================================================*/
void Clock_DrawFace(uint16_t cx, uint16_t cy, uint16_t r,
                    uint16_t fg, uint16_t bg)
{
    /* 1. clear a square area large enough for the dial */
    LCD_Fill(cx - r - 2, cy - r - 2, cx + r + 2, cy + r + 2, bg);

    /* 2. outer ring + inner decoration ring */
    LCD_Circle(cx, cy, r, fg);
    LCD_Circle(cx, cy, r - 3, fg);

    /* 3. 60 tick marks (every 6 degrees) */
    for (uint16_t i = 0; i < 60; i++)
    {
        float ang = (float)(i * 6) * 3.14159265f / 180.0f - 3.14159265f / 2.0f;
        float ca = cosf(ang);
        float sa = sinf(ang);

        uint16_t r1, r2;
        if (i % 15 == 0)       /* quarter hour — longest tick */
        { r1 = (uint16_t)(r * 0.75f);  r2 = (uint16_t)(r * 0.97f); }
        else if (i % 5 == 0)   /* hour — medium tick */
        { r1 = (uint16_t)(r * 0.82f);  r2 = (uint16_t)(r * 0.97f); }
        else                   /* minute — short tick */
        { r1 = (uint16_t)(r * 0.89f);  r2 = (uint16_t)(r * 0.97f); }

        uint16_t x1 = (uint16_t)((float)cx + ca * r1 + 0.5f);
        uint16_t y1 = (uint16_t)((float)cy + sa * r1 + 0.5f);
        uint16_t x2 = (uint16_t)((float)cx + ca * r2 + 0.5f);
        uint16_t y2 = (uint16_t)((float)cy + sa * r2 + 0.5f);
        LCD_Line(x1, y1, x2, y2, fg);
    }

    /* 4. 12 hour numbers around the dial */
    for (uint16_t h = 1; h <= 12; h++)
    {
        float ang = (float)(h * 30) * 3.14159265f / 180.0f - 3.14159265f / 2.0f;
        float ca = cosf(ang);
        float sa = sinf(ang);

        uint16_t nr = (uint16_t)(r * 0.63f);
        uint16_t nx = (uint16_t)((float)cx + ca * nr + 0.5f);
        uint16_t ny = (uint16_t)((float)cy + sa * nr + 0.5f);

        char num[4];
        sprintf(num, "%u", h);

        /* center the 1- or 2-digit number on (nx, ny) */
        uint16_t tw = (h >= 10) ? 16 : 8;
        uint16_t th = 16;
        LCD_String(nx - tw / 2, ny - th / 2, num, 16, fg, bg);
    }

    /* 5. pivot dot */
    LCD_Circle(cx, cy, 3, fg);
}

/*============================================================================
 * Clock hands — redraw every second
 *   Strategy: remember previous h/m/s, erase old hands with bg,
 *   then draw new hands with fg.  Dial is never redrawn → no flicker.
 *============================================================================*/
void Clock_DrawHands(uint16_t cx, uint16_t cy, uint16_t r,
                     uint8_t h, uint8_t m, uint8_t s,
                     uint16_t fg, uint16_t bg)
{
    static uint8_t s_ph = 0xFF, s_pm = 0xFF, s_ps = 0xFF;

    float h_len = (float)r * 0.42f;
    float m_len = (float)r * 0.60f;
    float s_len = (float)r * 0.76f;

    float h_ang = (float)((h % 12) * 30 + m * 30 / 60);
    float m_ang = (float)(m * 6 + s * 6 / 60);
    float s_ang = (float)(s * 6);

    if (s_ps != 0xFF)
    {
        float old_h_ang = (float)((s_ph % 12) * 30 + s_pm * 30 / 60);
        float old_m_ang = (float)(s_pm * 6 + s_ps * 6 / 60);
        float old_s_ang = (float)(s_ps * 6);
        DrawHand(cx, cy, old_h_ang, h_len, 3, bg);
        DrawHand(cx, cy, old_m_ang, m_len, 2, bg);
        DrawHand(cx, cy, old_s_ang, s_len, 1, bg);
    }

    DrawHand(cx, cy, h_ang, h_len, 3, fg);
    DrawHand(cx, cy, m_ang, m_len, 2, fg);
    DrawHand(cx, cy, s_ang, s_len, 1, fg);

    s_ph = h; s_pm = m; s_ps = s;
}
/*============================================================================
 * High-level clock state (encapsulated)
 *============================================================================*/
static OLED_Screen_t s_clock_time;
static OLED_Screen_t s_clock_date;
static uint8_t       s_clock_on        = 0;
static uint8_t       s_clock_last_sec  = 0xFF;
static uint8_t       s_clock_last_date = 0xFF;
static char          s_clock_last_tbuf[9] = {0};

static const uint16_t CLK_CX  = 160;
static const uint16_t CLK_CY  = 80;
static const uint16_t CLK_R   = 65;

void Clock_SetOn(uint8_t on)
{
    if (on == s_clock_on) return;
    s_clock_on = on;

    if (on)
    {
        Clock_DrawFace(CLK_CX, CLK_CY, CLK_R, WHITE, BLACK);

        OLED_Init(&s_clock_date, 120, 158, 1, 80, 16, OLED_FG_COLOR, OLED_BG_COLOR);
        OLED_Clear(&s_clock_date);

        OLED_Init(&s_clock_time, 128, 190, 1, 64, 16, OLED_FG_COLOR, OLED_BG_COLOR);
        OLED_Clear(&s_clock_time);

        s_clock_last_sec  = 0xFF;
        s_clock_last_date = 0xFF;
        memset(s_clock_last_tbuf, 0, sizeof(s_clock_last_tbuf));
    }
    else
    {
        LCD_Fill(CLK_CX - CLK_R - 2, CLK_CY - CLK_R - 2,
                 CLK_CX + CLK_R + 2, CLK_CY + CLK_R + 2, BLACK);
        LCD_Fill(0, 150, 319, 220, BLACK);
    }
}

uint8_t Clock_IsOn(void) { return s_clock_on; }

void Clock_Render(uint8_t h, uint8_t m, uint8_t s,
                  uint8_t year, uint8_t month, uint8_t day)
{
    if (!s_clock_on) return;

    if (s != s_clock_last_sec)
    {
        char buf[9];
        buf[0] = '0' + h / 10;
        buf[1] = '0' + h % 10;
        buf[2] = ':';
        buf[3] = '0' + m / 10;
        buf[4] = '0' + m % 10;
        buf[5] = ':';
        buf[6] = '0' + s / 10;
        buf[7] = '0' + s % 10;
        buf[8] = '\0';

        if (s_clock_last_sec == 0xFF)
        {
            OLED_ShowString(&s_clock_time, 0, 0, buf);
            memcpy(s_clock_last_tbuf, buf, 9);
        }
        else
        {
            uint16_t cols[8];
            char     oldc[8], newc[8];
            uint16_t cnt = 0;
            for (int i = 0; i < 8; i++)
            {
                if (buf[i] != s_clock_last_tbuf[i])
                {
                    cols[cnt]  = i;
                    oldc[cnt]  = s_clock_last_tbuf[i];
                    newc[cnt]  = buf[i];
                    cnt++;
                }
            }
            if (cnt > 0)
            {
                OLED_PushCols(&s_clock_time, cols, oldc, newc, cnt, &s_clock_on);
            }
            memcpy(s_clock_last_tbuf, buf, 9);
        }

        Clock_DrawHands(CLK_CX, CLK_CY, CLK_R, h, m, s, WHITE, BLACK);
        s_clock_last_sec = s;
    }

    uint8_t dc = (year << 4) | (month & 0x0F);
    if (dc != s_clock_last_date)
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
        OLED_Clear(&s_clock_date);
        OLED_ShowString(&s_clock_date, 0, 0, dbuf);
        s_clock_last_date = dc;
    }
}

/*============================================================================
 * Big pixel-style digital clock (xt command)
 *   Pure digital — no analog dial, no hands.
 *   Big scale so each digit is chunky and visible from across the room.
 *============================================================================*/
static OLED_Screen_t s_pix_time;
static OLED_Screen_t s_pix_date;
static uint8_t       s_pix_on        = 0;
static uint8_t       s_pix_last_sec  = 0xFF;
static uint8_t       s_pix_last_date = 0xFF;
static char          s_pix_last_tbuf[9] = {0};

static const uint16_t PIX_TIME_X     = 32;
static const uint16_t PIX_TIME_Y     = 78;
static const uint8_t  PIX_TIME_SCALE = 4;

static const uint16_t PIX_DATE_X     = 80;
static const uint16_t PIX_DATE_Y     = 158;
static const uint8_t  PIX_DATE_SCALE = 2;

void PixelClock_SetOn(uint8_t on)
{
    if (on == s_pix_on) return;
    s_pix_on = on;

    if (on)
    {
        if (Clock_IsOn()) Clock_SetOn(0);

        LCD_Fill(0, 0, xLCD.width - 1, xLCD.height - 1, BLACK);

        OLED_Init(&s_pix_time, PIX_TIME_X, PIX_TIME_Y, PIX_TIME_SCALE,
                  64, 16, OLED_FG_COLOR, OLED_BG_COLOR);
        OLED_Clear(&s_pix_time);

        OLED_Init(&s_pix_date, PIX_DATE_X, PIX_DATE_Y, PIX_DATE_SCALE,
                  80, 16, OLED_FG_COLOR, OLED_BG_COLOR);
        OLED_Clear(&s_pix_date);

        s_pix_last_sec  = 0xFF;
        s_pix_last_date = 0xFF;
        memset(s_pix_last_tbuf, 0, sizeof(s_pix_last_tbuf));
    }
    else
    {
        LCD_Fill(0, 0, xLCD.width - 1, xLCD.height - 1, BLACK);
    }
}

uint8_t PixelClock_IsOn(void) { return s_pix_on; }

void PixelClock_Render(uint8_t h, uint8_t m, uint8_t s,
                       uint8_t year, uint8_t month, uint8_t day)
{
    if (!s_pix_on) return;

    if (s != s_pix_last_sec)
    {
        char buf[9];
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
            memcpy(s_pix_last_tbuf, buf, 9);
        }
        else
        {
            uint16_t cols[8];
            char     oldc[8], newc[8];
            uint16_t cnt = 0;
            for (int i = 0; i < 8; i++)
            {
                if (buf[i] != s_pix_last_tbuf[i])
                {
                    cols[cnt]  = i;
                    oldc[cnt]  = s_pix_last_tbuf[i];
                    newc[cnt]  = buf[i];
                    cnt++;
                }
            }
            if (cnt > 0)
            {
                OLED_PushCols(&s_pix_time, cols, oldc, newc, cnt, &s_pix_on);
            }
            memcpy(s_pix_last_tbuf, buf, 9);
        }
        s_pix_last_sec = s;
    }

    uint8_t dc = (year << 4) | (month & 0x0F);
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