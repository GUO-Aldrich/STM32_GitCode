#ifndef __BSP__W25Q_H
#define __BSP__W25Q_H
/***********************************************************************************************************************************
 ** 【文件名称】  bsp_W25Q128.h
 ** 【编写人员】  魔女开发板团队
 ** 【淘    宝】  魔女开发板      https://demoboard.taobao.com
 ***********************************************************************************************************************************
 ** 【功能描述】  定义引脚、定义全局结构体、声明全局函数
 **
 ** 【适用平台】  STM32F407 + keil5
 **
 ** 【 CubeMX 】  为了移植方便，本示例另写了 bsp_W25Q128.c 和 bsp_W25Q128.h 两个驱动文件
 **               无需使用CubeMX配置SPI引脚，直接调用 bsp_W25Q128.h 所声明的函数，即可实现：初始化、读取、存储
 **
 ** 【移植说明】  1- 无需通过CubeMX配置引脚.
 **               2- 复制本工程的W25Q128文件夹，到目标工程文件夹中;
 **               3- 添加头文件路径： Keil > Option > C/C++ > Include Paths, 选择刚才复制后粘贴的目标文件夹路径;
 **               4- 添加C文件到工程: Keil > 左侧工程管理器中双击目标文件夹 > 选择 bsp_W25Q128.c;
 **               5- 添加文件引用:    #include "bsp_W25Q128.h＂，即哪个文件要用串口功能，就在其代码文件顶部添加引用;
 **
 ** 【代码使用】  所用GPIO引脚和SPI端口，可以在 "bsp_W25Q128.h＂中修改;
 **               整个W25Q128的读写操作，包括中文字库数据，已封装成4个全局函数，只用这4个函数，即可完成对其存取操作
 **               初始化  ：  W25Q128_Init()　
 **               读取数据：  W25Q128_ReadData (uint32_t addr, uint8_t *pData, uint16_t num)　      // 读取数据：数据地址，缓存指针，字节数
 **               写入数据：  W25Q128_WriteData(uint32_t addr, uint8_t *pData, uint16_t num)        // 写入数据：数据地址，缓存指针，字节数
 **               字模读取：  W25Q128_ReadGBK  (uint8_t *typeface, uint8_t size, uint8_t *dataBuf); // 读取字模：从w25qxx的字库中读取出字模数据
 **
 ** 【划 重 点】 1_ 本代码，适用W25Q16、W25Q32、W25Q65、W25Q128，不适用于W25Q256;
 **              2_ 地址的范围取值，如W25Q128，容量：128/8=16M字节，则字节地址范围：0~16777216，用16进制表示为：0x00~0x01000000
 **              3_ 如果使用的是魔女开发板上的W25Q128，存储数据时，请使用前10M空间地址0X0~0x9FFFFF; 芯片后6M空间,已存储汉字字模数据，地址为0x00A00000~0x01000000;
 **              4_ 代码已经多次优化，直接使用即可，而无需过于纠结SPI通信原理、芯片的存储原理，而浪费了时间;
 ** 
 ** 【字库使用】  特别地注意，请慎重使用芯片擦除，魔女开发板的w25q128，在存储区尾部已烧录宋体4种字号大小汉字GBK字模数据
 **               字库存放地址：0x00A00000 - 0x01000000   尽量不要写操作此区域地址
 **               具体的读取操作，可参考c文件中函数
 **
 ** 【更新记录】  2024-02-05  优化函数
 **              2023-01-27  完善注释
 **              2022-12-29  创建文件，从F103移植至F407
 **
***********************************************************************************************************************************/
#include <stdio.h>

#ifdef USE_HAL_DRIVER  
#include "stm32f4xx_hal.h"
#endif

#ifdef USE_STDPERIPH_DRIVER  
#include <stm32f4xx.h>
#endif



/*****************************************************************************
 ** 引脚定义
 ** 移植时，如果使用SPI1,只需要修改这个区域
****************************************************************************/
#define  GBK_STORAGE_ADDR               0x00A00000       // 汉字GBK字库起始地址,魔女开发板的W25Q128已保存宋体12、16、24、32号字体

#define  W25Q128_SPI                    SPI1             // SPI端口

#ifdef USE_HAL_DRIVER                                    // HAL库
// 引脚复用功能
#define  W25Q128_SPI_AFx                GPIO_AF5_SPI1    // 引脚复用编号
// SCK
#define  W25Q128_SCK_GPIO               GPIOA            // SCK引脚; 时钟同步引脚
#define  W25Q128_SCK_PIN                GPIO_PIN_5    
// MISO                              
#define  W25Q128_MISO_GPIO              GPIOA            // MISO引脚; 主机输入从机输出
#define  W25Q128_MISO_PIN               GPIO_PIN_6                
// MOSI                                  
#define  W25Q128_MOSI_GPIO              GPIOA            // MOSI引脚; 主机输出从机输入
#define  W25Q128_MOSI_PIN               GPIO_PIN_7                
// CS
#define  W25Q128_CS_GPIO                GPIOC            // 片选引脚; SPI总线中，主机拉低哪个设备的CS线，那个设备就接受通信
#define  W25Q128_CS_PIN                 GPIO_PIN_13      
#endif

#ifdef USE_STDPERIPH_DRIVER                              // 标准库
// SCK
#define  W25Q128_SCK_GPIO               GPIOA  
#define  W25Q128_SCK_PIN                GPIO_Pin_5        
#define  W25Q128_SCK_PINSOURCE          GPIO_PinSource5
#define  W25Q128_SCK_AF                 GPIO_AF_SPI1
// MISO                                
#define  W25Q128_MISO_GPIO              GPIOA 
#define  W25Q128_MISO_PIN               GPIO_Pin_6                
#define  W25Q128_MISO_PINSOURCE         GPIO_PinSource6
#define  W25Q128_MISO_AF                GPIO_AF_SPI1
// MOSI                                   
#define  W25Q128_MOSI_GPIO              GPIOA 
#define  W25Q128_MOSI_PIN               GPIO_Pin_7                
#define  W25Q128_MOSI_PINSOURCE         GPIO_PinSource7
#define  W25Q128_MOSI_AF                GPIO_AF_SPI1
// CS
#define  W25Q128_CS_GPIO                GPIOC 
#define  W25Q128_CS_PIN                 GPIO_Pin_13      
#endif




/*****************************************************************************
 ** 声明全局变量
****************************************************************************/
typedef struct
{
    uint8_t   FlagInit;                 // 初始化状态   0:失败, 1:成功
    uint8_t   FlagGBKStorage;           // GBK字库标志; 0=没有, 1=可用; 作用: 用于判断地址段的写保护, 防止字库被错误写履盖; 并可作LCD的中文输出判断
    char      type[20];                 // 型号
    uint16_t  StartupTimes;             // 记录启动次数
} xW25Q_TypeDef;
extern xW25Q_TypeDef  xW25Q128;         // 声明全局结构体, 用于记录w25qxx信息





/*****************************************************************************
 ** 声明全局函数
 ** 为统一代码以方便移植，已封装成4个对外函数，可完成对其所有存取操作
****************************************************************************/
uint8_t W25Q128_Init(void);                                                    // 初始化
void    W25Q128_ReadData(uint32_t addr, uint8_t *pData, uint16_t num);         // 读数据：addr-地址，*pData-读取后数据缓存，num-要读取的字节数
void    W25Q128_WriteData(uint32_t addr, uint8_t *pData, uint16_t num);        // 写数据：addr-地址，*pData-待写的数据缓存，num-要写入的字节数
void    W25Q128_ReadFontData(uint8_t *pFont, uint8_t size, uint8_t *fontData); // 读取字模：*pFont-汉字，size-字号，*fontData-读取到的字模点阵数据



#endif




