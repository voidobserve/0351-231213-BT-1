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
 * ��Ȩ˵����������
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "my_config.h"
#include "include.h"
#include <math.h>
#include <stdio.h>

float step = 70;
float mi;  // ��
float rus; // 10���ݴη�
float r_ms = 0;
// #define USER_BAUD (115200UL)
// #define USER_UART_BAUD ((SYSCLK - USER_BAUD) / (USER_BAUD))

#if USE_MY_DEBUG

#define UART0_BAUD 115200
#define USER_UART0_BAUD ((SYSCLK - UART0_BAUD) / (UART0_BAUD))
// ��дpuchar()����
char putchar(char c)
{
    while (!(UART0_STA & UART_TX_DONE(0x01)))
        ;
    UART0_DATA = c;
    return c;
}

void my_debug_config(void)
{
    // P15��Ϊ��������
    P1_MD1 &= (GPIO_P15_MODE_SEL(0x3));
    P1_MD1 |= GPIO_P15_MODE_SEL(0x1);            // P15����Ϊ���ģʽ
    FOUT_S15 |= GPIO_FOUT_UART0_TX;              // ����ΪUART0_TX
    UART0_BAUD1 = (USER_UART0_BAUD >> 8) & 0xFF; // ���ò����ʸ߰�λ
    UART0_BAUD0 = USER_UART0_BAUD & 0xFF;        // ���ò����ʵͰ�λ
    UART0_CON0 = UART_STOP_BIT(0x0) |
                 UART_EN(0x1); // 8bit���ݣ�1bitֹͣλ
}
#endif // USE_MY_DEBUG

void main(void)
{
    // ���Ź�Ĭ�ϴ�, ��λʱ��2s
    WDT_KEY = WDT_KEY_VAL(0xDD); //  �رտ��Ź� (�������ÿ��Ź���鿴��WDT\WDT_Reset��ʾ��)

    system_init();

#if USE_MY_DEBUG
    // ��ʼ����ӡ
    my_debug_config();

#endif

    // ��ѹ����  16��-----P14
    //		P1_MD1   &= ~GPIO_P14_MODE_SEL(0x03);
    //		P1_MD1   |=  GPIO_P14_MODE_SEL(0x01);
    //		FOUT_S14  =  GPIO_FOUT_AF_FUNC;
    ///////////////////////////////////////////

#if 1
    adc_pin_config(); // ����ʹ�õ�adc������
    // adc_sel_pin(ADC_SEL_PIN_GET_TEMP);
    tmr0_config(); // ���ö�ʱ����Ĭ�Ϲر�
    pwm_init();    // ����pwm���������
    tmr1_config();
#endif

// ===================================================================
#if 1        // ��������������PWM�źű仯ƽ����
    P14 = 0; // 16��������͵�ƽ
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
        delay_ms(16); // ÿ16ms����һ��PWM��������
    }
#endif
    // ===================================================================
1
    while (1)
    {
        adc_scan();                       // �����������һ�˵ĵ�ѹֵ
        set_duty();                       // �趨��Ҫ���ڵ�������
        according_pin9_to_adjust_pin16(); // ����9�ŵĵ�ѹ���趨16�ŵĵ�ƽ
        // Adaptive_Duty(); // ��������
    }
}

/**
 * @}
 */

/*************************** (C) COPYRIGHT 2022 HUGE-IC ***** END OF FILE *****/
