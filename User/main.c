/**
 ******************************************************************************
 * @file    main.c
 * @author  HUGE-IC Application Team
 * @version V1.0.0
 * @date    02-09-2022
 * @brief   Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2021 HUGE-IC</center></h2>
 *
 * 版权说明后续补上
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "my_config.h"
#include "include.h"
#include <math.h>
#include <stdio.h>

float step = 70;
float mi;  // 幂
float rus; // 10的幂次方
float r_ms = 0;
// #define USER_BAUD (115200UL)
// #define USER_UART_BAUD ((SYSCLK - USER_BAUD) / (USER_BAUD))

#if USE_MY_DEBUG

#define UART0_BAUD 115200
#define USER_UART0_BAUD ((SYSCLK - UART0_BAUD) / (UART0_BAUD))
// 重写puchar()函数
char putchar(char c)
{
    while (!(UART0_STA & UART_TX_DONE(0x01)))
        ;
    UART0_DATA = c;
    return c;
}

void my_debug_config(void)
{
    // P15作为发送引脚
    P1_MD1 &= (GPIO_P15_MODE_SEL(0x3));
    P1_MD1 |= GPIO_P15_MODE_SEL(0x1);            // P15配置为输出模式
    FOUT_S15 |= GPIO_FOUT_UART0_TX;              // 配置为UART0_TX
    UART0_BAUD1 = (USER_UART0_BAUD >> 8) & 0xFF; // 配置波特率高八位
    UART0_BAUD0 = USER_UART0_BAUD & 0xFF;        // 配置波特率低八位
    UART0_CON0 = UART_STOP_BIT(0x0) |
                 UART_EN(0x1); // 8bit数据，1bit停止位
}
#endif // USE_MY_DEBUG

void main(void)
{
    // 看门狗默认打开, 复位时间2s
    WDT_KEY = WDT_KEY_VAL(0xDD); //  关闭看门狗 (如需配置看门狗请查看“WDT\WDT_Reset”示例)

    system_init();

#if USE_MY_DEBUG
    // 初始化打印
    my_debug_config();

#endif

    // 过压保护  16脚-----P14
    //		P1_MD1   &= ~GPIO_P14_MODE_SEL(0x03);
    //		P1_MD1   |=  GPIO_P14_MODE_SEL(0x01);
    //		FOUT_S14  =  GPIO_FOUT_AF_FUNC;
    ///////////////////////////////////////////

#if 1
    adc_pin_config(); // 配置使用到adc的引脚
    // adc_sel_pin(ADC_SEL_PIN_GET_TEMP);
    tmr0_config(); // 配置定时器，默认关闭
    pwm_init();    // 配置pwm输出的引脚
    tmr1_config();
#endif

// ===================================================================
#if 1        // 开机缓慢启动（PWM信号变化平缓）
    P14 = 0; // 16脚先输出低电平
    c_duty = 0;
    while (c_duty < 6000)
    {
        if (jump_flag == 1)
        {
            break;
        }
        if (c_duty < 6000)
        {
            mi = (step - 1) / (253 / 3) - 1;
            step += 0.5;
            c_duty = pow(5, mi) * 60;
        }
        if (c_duty >= 6000)
        {
            c_duty = 6000;
        }
        // printf("c_duty %d\n",c_duty);
        set_pwm_duty();
        delay_ms(16); // 每16ms调整一次PWM的脉冲宽度
    }
#endif
    // ===================================================================
1
    while (1)
    {
        adc_scan();                       // 检测热敏电阻一端的电压值
        set_duty();                       // 设定到要调节到的脉宽
        according_pin9_to_adjust_pin16(); // 根据9脚的电压来设定16脚的电平
        // Adaptive_Duty(); // 调节脉宽
    }
}

/**
 * @}
 */

/*************************** (C) COPYRIGHT 2022 HUGE-IC ***** END OF FILE *****/
