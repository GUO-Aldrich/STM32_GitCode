#ifndef __LCD_OLED_STYLE_H
#define __LCD_OLED_STYLE_H

#include "LCD_ILI9341.h"

#ifdef USE_HAL_DRIVER
#include "stm32f4xx_hal.h"
#endif

#ifdef USE_STDPERIPH_DRIVER
#include "stm32f4xx.h"
#endif

extern const unsigned char aFontASCII16[95][16];

#define OLED_BG_COLOR       BLACK
#define OLED_FG_COLOR       WHITE
#define OLED_GRID_COLOR     0x0006

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint8_t  scale;
    uint16_t width;
    uint16_t height;
    uint16_t fg_color;
    uint16_t bg_color;
} OLED_Screen_t;

void OLED_Init(OLED_Screen_t *screen, uint16_t x, uint16_t y, uint8_t scale,
               uint16_t logical_w, uint16_t logical_h,
               uint16_t fg_color, uint16_t bg_color);

void OLED_Clear(OLED_Screen_t *screen);

void OLED_DrawPixel(OLED_Screen_t *screen, uint16_t px, uint16_t py, uint8_t on);

void OLED_DrawGrid(OLED_Screen_t *screen, uint16_t grid_color);

void OLED_ShowChar(OLED_Screen_t *screen, uint16_t px, uint16_t py, char ch);

void OLED_ShowString(OLED_Screen_t *screen, uint16_t px, uint16_t py, const char *str);

void OLED_ShowHex(OLED_Screen_t *screen, uint16_t px, uint16_t py,
                  const uint8_t *data, uint16_t len, uint8_t bytes_per_row);

void OLED_PreviewDemo(uint16_t x_offset, uint16_t y_offset);

/* New char slides DOWNWARD from above the slot into place (single column) */
void OLED_SlideInChar(OLED_Screen_t *screen, uint16_t col, char ch);

/**
 * PUSH animation: new chars ENTER from BELOW, old chars EXIT above.
 * Every frame the two characters (old + new) are drawn at Y positions
 * whose union EXACTLY covers the column slot 鈥?so there is NEVER a
 * "hole" where bg_color shows through.  This eliminates the white-block
 * ghosting that plagues naive slide-in implementations.
 *
 *   frame=0:   [ OLD FULLY visible ]    [ NEW fully below slot  ]
 *   frame=8:   [ OLD top half     ]    [ NEW bottom half       ]
 *   frame=16:  [ OLD fully above  ]    [ NEW FULLY visible     ]
 *
 * total visible height always = char_h 鈫?no gaps, no white blocks.
 */
void OLED_PushCols(OLED_Screen_t *screen,
                   const uint16_t *cols,
                   const char     *old_chars,
                   const char     *new_chars,
                   uint16_t        count,
                   const volatile uint8_t *abort_flag);

/* Multiple columns slide in SYNCHRONOUSLY from above */
void OLED_SlideInCols(OLED_Screen_t *screen,
                      const uint16_t *cols,
                      const char     *chars,
                      uint16_t        count);

/*============================================================================
 * Big pixel-style digital clock (xt command 鈥?no analog dial)
 *============================================================================*/

/* Turn the big pixel clock on (1) or off (0). */
void PixelClock_SetOn(uint8_t on);

/* Query current state. */
uint8_t PixelClock_IsOn(void);

/* Call this every main loop iteration when PixelClock_IsOn() is true.
 * Same parameter format as Clock_Render. */
void PixelClock_Render(uint8_t h, uint8_t m, uint8_t s,
                       uint8_t year, uint8_t month, uint8_t day);

#endif
