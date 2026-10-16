#ifndef __LCD_ILI9341_H
#define __LCD_ILI9341_H
/**==================================================================================================================
 **【文件名称】  bsp_LCD_ILI9341.h
 **【功能测试】  2.8寸显示屏
 **==================================================================================================================
 **【适用平台】  STM32F407 + KEIL5.27 + 2.8寸显示屏_ILI9341
 **
 **【划 重 点】  1-本示例代码，适配市面大部分2.8寸ILI9341的34针屏
 **              2_本示例代码，包含了两种文本显示方式：使用自行转模的字模数据、使用Flash中的字模数据。
 **              3_如果使用Flash中的字模数据，需要添加bsp_W25Q128.c到工程中，并添加头文件bsp_LCD_ILI9341.h至LCD中;
 **
 **【更新记录】  2024-07-15  优化字模函数、完善其注释
 **              2024-01-25  从标准库工程移植至HAL库工程，使用寄存器操作实现，以使用移植更方便
 **              2023-01-12  增加自行建模的文本显示函数、完善注释
 **              2022-12-30  完善中文显示函数、图片显示函数
 **
 **【备注说明】  代码版权归魔女科技所有，请勿商用，谢谢！
 **              https://demoboard.taobao.com
====================================================================================================================*/
#include "bsp_W25Q128.h"                      // 中文字库
#include "stdlib.h"

#ifdef USE_HAL_DRIVER                         // HAL库 配置
#include "stm32f4xx_hal.h"                   
#endif                                       
                                             
#ifdef USE_STDPERIPH_DRIVER                   // 标准库 配置
#include "stm32f4xx.h"                       
#endif



/*****************************************************************************
 ** 移植配置
 **
****************************************************************************/
#ifdef USE_HAL_DRIVER                         // HAL库 配置
#define LCD_BL_GPIO   GPIOA                   // 背光控制引脚
#define LCD_BL_PIN    GPIO_PIN_15            
#endif                                       
                                             
#ifdef USE_STDPERIPH_DRIVER                   // 标准库 配置
#define LCD_BL_GPIO   GPIOA                   // 背光控制引脚
#define LCD_BL_PIN    GPIO_Pin_15
#endif


#define LCD_WIDTH     240                     // 屏的宽度像素点
#define LCD_HEIGHT    320                     // 屏的高度像素点

//#define LCD_DIR       6                     // 四种显示方向，0-正竖屏，3-倒竖屏，5-正横屏, 6-倒横屏; 注意：如果使用触摸屏，每次更换方向后，都需要重新校准



/*****************************************************************************
 ** 全局变量
 ** (不用修改)
****************************************************************************/
typedef struct                                // LCD重要参数集
{
    uint16_t width;                           // LCD 宽度
    uint16_t height;                          // LCD 高度
    uint16_t id;                              // LCD ID
    uint8_t  dir;                             // 横屏还是竖屏控制：0，竖屏；1，横屏。
    uint8_t  FlagInit;                        // 初始化完成标志
} xLCD_TypeDef;
extern xLCD_TypeDef xLCD;                     // 管理LCD重要参数


/******************************* 定义常用颜色值 *****************************/
#define      WHITE               0xFFFF       // 白色
#define      BLACK               0x0000       // 黑色 
#define      GREY                0xF7DE       // 灰色 
#define      GRAY                0X8430       // 灰色
#define      RED                 0xF800       // 红 
#define      MAGENTA             0xF81F       // 洋红色 
#define      GRED                0xFFE0       // 深红色
#define      BROWN               0XBC40       // 棕色
#define      BRRED               0XFC07       // 棕红色
#define      GREEN               0x07E0       // 绿 
#define      CYAN                0x7FFF       // 青色 
#define      YELLOW              0xFFE0       // 黄色 
#define      LIGHTGREEN          0X841F       // 浅绿色 
#define      BLUE                0x001F       // 蓝 
#define      GBLUE               0x07FF       // 浅蓝 1
#define      LIGHTBLUE           0X7D7C       // 浅蓝 2
#define      BLUE2               0x051F       // 浅蓝 3
#define      GRAYBLUE            0X5458       // 灰蓝 
#define      DARKBLUE            0X01CF       // 深蓝
#define      LGRAY               0XC618       // 浅灰色,窗体背景色
#define      LGRAYBLUE           0XA651       // 浅灰蓝色(中间层颜色)
#define      LBBLUE              0X2B12       // 浅棕蓝色(选择条目的反色)



/*****************************************************************************
 ** 声明全局函数
 ** (不用修改)
****************************************************************************/
void LCD_Init(void);                                                                                       // 初始化
void LCD_SetDir(uint8_t dir);                                                                              // 设置显示方向
void LCD_GUI(void);                                                                                        // 简单的显示界面
void LCD_DisplayOn(void);                                                                                  // 开显示
void LCD_DisplayOff(void);                                                                                 // 关显示
void LCD_DrawPoint(uint16_t  x, uint16_t  y, uint16_t _color);                                             // 画点函数
void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color);                         // 填充单色
void LCD_Line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);                         // 画线
void LCD_Circle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius, uint16_t _color);                           // 画圆
void LCD_String(uint16_t x, uint16_t y, char *pFont, uint8_t size, uint32_t fColor, uint32_t bColor);      // 显示任意字符串(支持英文、汉字)
void LCD_Image(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *image);             // 图片
void LCD_ShowChinese(uint8_t x, uint8_t y, uint8_t size, uint8_t indux, uint32_t fColor, uint32_t bColor); // 显示自行转模的文本
void LCD_Cross(uint16_t x, uint16_t y, uint16_t len, uint32_t fColor);                                     // 用于触摸校准：画十字线
void LCD_DispFlush(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *pData);        // 在指定区域填充数据，适用于图片、LVGL等


#endif

