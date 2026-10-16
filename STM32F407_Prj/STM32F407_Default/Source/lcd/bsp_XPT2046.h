#ifndef  __BSP_XPT2046_H
#define  __BSP_XPT2046_H
/***********************************************************************************************************************************
 ** 【代码编写】  魔女开发板团队
 ** 【代码更新】
 ** 【淘    宝】  魔女开发板      https://demoboard.taobao.com
 ***********************************************************************************************************************************
 ** 【文件名称】  bsp_XPT2046.h
 **
 ** 【文件功能】  定义引脚、定义全局结构体、声明全局函数
 **               本文件简化的XPT2046的初始化; 初始化后调用函数即可使用；
 **
 ** 【适用平台】  STM32F407 + keil5 
 **
 ** 【移植说明】  1- 复制本工程的LCD_ILI9341、W25Q128、XPT2046文件夹，到目标工程文件夹中;
 **               2- 添加头文件路径： Keil > Option > C/C++ > Include Paths, 添加上述3个目标文件夹的目录路径;
 **               3- 添加C文件到工程: Keil > 左侧工程管理器中双击目标文件夹 > 选择 bsp_LCD_ILI9341.c、bsp_W25Q128.c、bsp_XPT2046;
 **               4- 添加文件引用:    #include "bsp_XPT2046.h"，即哪个文件中需要使用其函数功能，就在其代码文件顶部添加引用;
 **
 ** 【 CubeMX 】  1- 无需使用CubeMX配置引脚
 **               2- 在bsp_XPT2046.h文件中修改使用的引脚
 **               
 ** 【代码使用】  1- 关于触摸的状态检测、坐标获取，本示例通过描点输出，已把机制描述得很直观了。经常还是有人反馈：不明白，已经无力解释~~。
 **               2- 本示例通过触摸描点，已把基础函数编写好了，用户可以在这个基础上增添设计，如：通过画点函数设计按钮等器件，再通过触摸检测来完善触发。
 **               3- 如果不懂得如何自行绘制需要的控件、不懂得设计控件的事件，建议移植使用LVGL; LVGL的优点：轻松绘制控制、事件处理，减少了交互的底层设计; LVGL的缺点：占用大量资源;
 **               4- 触摸的应用，底层操作仅四步，适用于裸机前后台、FreeRTOS、LVGL：
 **                  初 始 化: XPT2046_Init(xLCD.width, xLCD.height, xLCD.dir);   // 初始化：宽度，高度，方向
 **                  触摸判断：XPT2046_IsPressed();                               // 检查触摸屏当前是否被按下; 返回：0-未按下、1-按下; 如果按下，可以调用以下两个函数获取坐标;
 **                  获取坐标：XPT2046_GetX();                                    // 获取按下的位置的坐标：X; 返回值类型：uint16_t
 **                            XPT2046_GetX();                                    // 获取按下的位置的坐标：Y; 返回值类型：uint16_t
 **               5- 如果发现触摸位置不准确, 即按下的位置与画点的位置相距很明显，可以进行重新校准，烧录运行本示例后，用串口助手发送"XPT2046", 程序即可进入重新校准。
 **               6- 特别地：必须 #include "bsp_W25Q128.h"，因为它里面记录了电阻屏的校准数据; 如果使用固定的显示屏，固定的显示方向，可以直接把它存储到程序中;
 **
 ** 【更新记录】
 **
 ** 【备注说明】  版权归魔女科技所有，限于学习测试，切勿商用，谢谢！
 **               https://demoboard.taobao.com
 
************************************************************************************************************************************/
#include "lcd_ili9341.h"
#include "stdio.h"


#ifdef USE_HAL_DRIVER                                        // HAL库 配置
#include "stm32f4xx_hal.h"
#endif

#ifdef USE_STDPERIPH_DRIVER                                  // 标准库 配置
#include "stm32f4xx.h"
#endif




/*****************************************************************************
 ** 引脚宠定义
 ** 备注：使用的是模拟SPI, INT引脚使用状态轮询, 不使用中断;
****************************************************************************/
#ifdef USE_HAL_DRIVER                                        // HAL库 配置
#define    XPT2046_IRQ_GPIO         GPIOE                    // 触摸信号指示引脚(不使用中断) 
#define    XPT2046_IRQ_PIN          GPIO_PIN_4               
#define    XPT2046_IRQ_PORT_CLK     RCC_AHB1Periph_GPIOE     
                                                             
#define    XPT2046_CS_GPIO          GPIOD                    // 模拟SPI_CS
#define    XPT2046_CS_PIN           GPIO_PIN_13              
#define    XPT2046_CS_PORT_CLK      RCC_AHB1Periph_GPIOD     
                                                             
#define    XPT2046_CLK_GPIO         GPIOE                    // 模拟SPI_CLK
#define    XPT2046_CLK_PIN          GPIO_PIN_0               
#define    XPT2046_CLK_PORT_CLK     RCC_AHB1Periph_GPIOE     
                                                             
#define    XPT2046_MOSI_GPIO        GPIOE                    // 模拟SPI_MOSI
#define    XPT2046_MOSI_PIN         GPIO_PIN_2               
#define    XPT2046_MOSI_PORT_CLK    RCC_AHB1Periph_GPIOE     
                                                             
#define    XPT2046_MISO_GPIO        GPIOE                    // 模拟SPI_MISO
#define    XPT2046_MISO_PIN         GPIO_PIN_3  
#define    XPT2046_MISO_PORT_CLK    RCC_AHB1Periph_GPIOE
#endif

#ifdef USE_STDPERIPH_DRIVER                                  // 标准库 配置
#define    XPT2046_IRQ_GPIO         GPIOE                    // 触摸信号指示引脚(不使用中断) 
#define    XPT2046_IRQ_PIN          GPIO_Pin_4               
#define    XPT2046_IRQ_PORT_CLK     RCC_AHB1Periph_GPIOE     
                                                             
#define    XPT2046_CS_GPIO          GPIOD                    // 模拟SPI_CS
#define    XPT2046_CS_PIN           GPIO_Pin_13              
#define    XPT2046_CS_PORT_CLK      RCC_AHB1Periph_GPIOD     
                                                             
#define    XPT2046_CLK_GPIO         GPIOE                    // 模拟SPI_CLK
#define    XPT2046_CLK_PIN          GPIO_Pin_0               
#define    XPT2046_CLK_PORT_CLK     RCC_AHB1Periph_GPIOE     
                                                             
#define    XPT2046_MOSI_GPIO        GPIOE                    // 模拟SPI_MOSI
#define    XPT2046_MOSI_PIN         GPIO_Pin_2               
#define    XPT2046_MOSI_PORT_CLK    RCC_AHB1Periph_GPIOE     
                                                             
#define    XPT2046_MISO_GPIO        GPIOE                    // 模拟SPI_MISO
#define    XPT2046_MISO_PIN         GPIO_Pin_3  
#define    XPT2046_MISO_PORT_CLK    RCC_AHB1Periph_GPIOE
#endif



/*****************************************************************************
 ** 声明全局函数声明                 
 *****************************************************************************/
// 初始化
void      XPT2046_Init (uint16_t lcdWidth, uint16_t lcdHeight, uint8_t dir ); // 初始化(引脚, 校准检测);  参数: 显示屏的显示方向, 取值范围: 1,2,3,4
// 重新校准
uint8_t   XPT2046_ReCalibration(void);                                        // 触摸坐标重新校准; 当发现坐标不对时，调用一次本函数，用笔尖依次点击十字光标线即可。
// 触摸检测、获取坐标; 
uint8_t   XPT2046_IsPressed(void);                                            // 判断触摸屏按下状态; 返回:0-释放、1-按下; 移植LVGL时，在lv_port_indev.c文件的touchpad_is_pressed()函数里调用此函数即可;
uint16_t  XPT2046_GetX(void);                                                 // 获取按下的X坐标值; 移植LVGL时，在lv_port_indev.c文件的touchpad_get_xy()调用本函数，可获得(*X)值;
uint16_t  XPT2046_GetY(void);                                                 // 获取按下的Y坐标值; 移植LVGL时，在lv_port_indev.c文件的touchpad_get_xy()调用本函数，可获得(*Y)值;  
// 裸机示范用的函数; LVGL下用不到，可以删除, 不影响底层
void      XPT2046_Cmd(uint8_t status);                                        // 触摸检测开关; 非必要的; 初始化后默认是开启的; 可以手动关闭，暂停检测以实现一些操作，然后再手动开启
void      XPT2046_TouchDown(void);                                            // 空白函数, 调用按下时的处理
void      XPT2046_TouchUp(void);                                              // 空白函数, 调用释放时的处理   




#endif 

