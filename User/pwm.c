#include "pwm.h"
#include "time0.h"

volatile u16 c_duty = 0;         // 当前设置的占空比
volatile u16 adjust_duty = 6000; // 最终要调节成的占空比
bit jump_flag = 0;
bit max_flag = 0; // 最大占空比的标志位

void pwm_init(void)
{
    STMR_CNTCLR |= STMR_0_CNT_CLR(0x1);
#define STMR0_PEROID_VAL (SYSCLK / 8000 - 1)
    STMR0_PSC = STMR_PRESCALE_VAL(0x07);
    STMR0_PRH = STMR_PRD_VAL_H((STMR0_PEROID_VAL >> 8) & 0xFF);
    STMR0_PRL = STMR_PRD_VAL_L((STMR0_PEROID_VAL >> 0) & 0xFF);
    STMR0_CMPAH = STMR_CMPA_VAL_H(((0) >> 8) & 0xFF); // 比较值
    STMR0_CMPAL = STMR_CMPA_VAL_L(((0) >> 0) & 0xFF); // 比较值
    STMR_PWMVALA |= STMR_0_PWMVALA(0x1);

    STMR_CNTMD |= STMR_0_CNT_MODE(0x1); // 连续计数模式
    STMR_LOADEN |= STMR_0_LOAD_EN(0x1); // 自动装载使能
    STMR_CNTCLR |= STMR_0_CNT_CLR(0x1); //
    STMR_CNTEN |= STMR_0_CNT_EN(0x1);   // 使能
    STMR_PWMEN |= STMR_0_PWM_EN(0x1);   // PWM输出使能
    P1_MD1 &= ~GPIO_P16_MODE_SEL(0x03);
    P1_MD1 |= GPIO_P16_MODE_SEL(0x01);
    P1_MD1 &= ~GPIO_P14_MODE_SEL(0x03);
    P1_MD1 |= GPIO_P14_MODE_SEL(0x01);
    FOUT_S14 = GPIO_FOUT_AF_FUNC;      // AF功能输出
    FOUT_S16 = GPIO_FOUT_STMR0_PWMOUT; // stmr0_pwmout
}

// 14脚的PWM调节
void set_pwm_duty(void)
{
    STMR0_CMPAH = STMR_CMPA_VAL_H(((c_duty) >> 8) & 0xFF); // 比较值
    STMR0_CMPAL = STMR_CMPA_VAL_L(((c_duty) >> 0) & 0xFF); // 比较值
    STMR_LOADEN |= STMR_0_LOAD_EN(0x1);                    // 自动装载使能
}

// 根据9脚的电压来调节PWM
void _My_Adjust_Pwm(float Val)
{
    float P9_Vol = Val * 0.0012;

#if USE_MY_DEBUG
    printf("9脚检测到的电压： %f V\n", P9_Vol);
#endif

    //	printf("P9_Vol %d\n",P9_Vol);
    ////////////控制14脚///////////////////   100 -> 80   7.5ms          80 -> 50  5ms     100 -> 50  3ms

    // 当9脚电压高于1.9V时，14脚输出100%的PWM信号
    if (P9_Vol > 1.90) // 输出100%
    {
        //	printf(" P9_Vol : %f.... 100\n",P9_Vol);
        adjust_duty = 6000;
        max_flag = 1;
    }
    else if (P9_Vol > 1.6 && P9_Vol < 1.9) // 缓降80%
    {
        // 9脚电压在1.6~1.9V时，14脚输出的占空比从100%缓降到80%

        //	printf(" P9_Vol : %f...... 80\n",P9_Vol);
        if (max_flag == 1)
        {
            if (P9_Vol < 1.85)
            {
                adjust_duty = 4800;
                max_flag = 0;
            }
        }
        else
        {
            adjust_duty = 4800;
        }

        if (c_duty >= adjust_duty)
            jump_flag = 1;
    }
    else if (P9_Vol < 1.6) // 缓降50%  并且维持50%
    {
        // 9脚电压小于1.6V，14输出的占空比从80%缓降到50%，并保持50%

        // printf(" P9_Vol : %f...... 50\n",P9_Vol);
        // adjust_duty = 3000;
        adjust_duty = PWM_DUTY_50_PERCENT;
        max_flag = 0;
        if (c_duty >= adjust_duty)
            jump_flag = 1;
    }
    ///////////////控制16脚//////////////////
    // 当9脚电压高于2.7V时，16脚输出1KHz 高电平,用于控制Q2的导通。
    if (P9_Vol > 2.7) // 16脚输出1KHZ的高电平
    {
        P14 = 1;
    }
    else // 16脚输出1KHZ的低电平
    {
        P14 = 0;
    }
}

// 根据9脚的电压来设定16脚的电平
void according_pin9_to_adjust_pin16(void)
{
    u8 i = 0;                         // 循环计数值
    u8 cnt = 0;                       // 计数值
    volatile u32 adc_aver_val = 0;    // 存放adc滤波后的值
    adc_sel_pin(ADC_SEL_PIN_GET_VOL); // 切换到9脚对应的adc配置

    // 采集电压
    for (i = 0; i < 10; i++)
    {
        adc_aver_val = get_voltage_from_pin();
        if (adc_aver_val >= 2700)
        {
            // 如果从9脚上采集的电压大于2.7V
            cnt++;
        }
    }

    // 当9脚电压高于2.7V时，16脚输出1KHz 高电平,用于控制Q2的导通（用于关机）。
    if (cnt == 10)
    {
        // 如果多次检测都满足条件，才认为9脚的电压确实大于2.7V
        P14 = 1;
    }
    else
    {
        P14 = 0;
    }
}

// 缓慢调节占空比（缓慢提升和缓慢下降）
void Adaptive_Duty(void)
{
    if (c_duty > adjust_duty)
    {
        c_duty--;
    }
    if (c_duty < adjust_duty)
    {
        c_duty++;
    }
    set_pwm_duty();

    if (c_duty >= 5800)
    {

        delay_ms(15); // 时间还需要测试调整一下
    }
    else
    {
        delay_ms(5);
    }
}
