#include "pwm.h"
#include "time0.h"

volatile u16 cur_duty = 0;       // 当前设置的占空比(对应的值)
volatile u16 adjust_duty = 6000; // 最终要调节成的占空比(对应的值)
bit jump_flag = 0;
bit max_flag = 0; // 最大占空比的标志位

void pwm_init(void)
{
    STMR_CNTCLR |= STMR_0_CNT_CLR(0x1);
#define STMR0_PEROID_VAL (SYSCLK / 8000 - 1) // == 6000 - 1，即，6000对应100%占空比
    STMR0_PSC = STMR_PRESCALE_VAL(0x07);
    STMR0_PRH = STMR_PRD_VAL_H((STMR0_PEROID_VAL >> 8) & 0xFF);
    STMR0_PRL = STMR_PRD_VAL_L((STMR0_PEROID_VAL >> 0) & 0xFF);
    STMR0_CMPAH = STMR_CMPA_VAL_H(((0) >> 8) & 0xFF); // 比较值
    STMR0_CMPAL = STMR_CMPA_VAL_L(((0) >> 0) & 0xFF); // 比较值
    STMR_PWMVALA |= STMR_0_PWMVALA(0x1);              // 计数CNT大于等于比较值A,PWM输出1,小于输出0

    STMR_CNTMD |= STMR_0_CNT_MODE(0x1); // 连续计数模式
    STMR_LOADEN |= STMR_0_LOAD_EN(0x1); // 自动装载使能
    STMR_CNTCLR |= STMR_0_CNT_CLR(0x1); //
    STMR_CNTEN |= STMR_0_CNT_EN(0x1);   // 使能
    STMR_PWMEN |= STMR_0_PWM_EN(0x1);   // PWM输出使能
    P1_MD1 &= ~GPIO_P16_MODE_SEL(0x03);
    P1_MD1 |= GPIO_P16_MODE_SEL(0x01); // 输出模式
    P1_MD1 &= ~GPIO_P14_MODE_SEL(0x03);
    P1_MD1 |= GPIO_P14_MODE_SEL(0x01); // 输出模式
    FOUT_S14 = GPIO_FOUT_AF_FUNC;      // AF功能输出 // 16脚
    FOUT_S16 = GPIO_FOUT_STMR0_PWMOUT; // stmr0_pwmout // 14脚

    // 15脚--P15的PWM配置，使用stmr1的pwm （还未验证是否可以使用）
    STMR_CNTCLR |= STMR_1_CNT_CLR(0x1); // 清空stmr1的计数
    STMR1_PSC = STMR_PRESCALE_VAL(0x07);
    STMR1_PRH = STMR_PRD_VAL_H((STMR0_PEROID_VAL >> 8) & 0xFF); // 频率与STMR0的一致
    STMR1_PRL = STMR_PRD_VAL_L((STMR0_PEROID_VAL >> 0) & 0xFF);
    STMR1_CMPAH = STMR_CMPA_VAL_H(((0) >> 8) & 0xFF); // 比较值 // 占空比默认为0%
    STMR1_CMPAL = STMR_CMPA_VAL_L(((0) >> 0) & 0xFF); // 比较值
    STMR_PWMVALA |= STMR_1_PWMVALA(0x1);              // 计数CNT大于等于比较值A,PWM输出1,小于输出0
    P1_MD1 &= ~GPIO_P15_MODE_SEL(0x03);
    P1_MD1 |= GPIO_P15_MODE_SEL(0x01); // 输出模式
    FOUT_S15 = GPIO_FOUT_STMR1_PWMOUT; // 15脚--P15--stmr1_pwmout
}

// 更新15脚的PWM占空比（未验证）
void refresh_pin15_pwm_duty(void)
{
    STMR1_CMPAH = STMR_CMPA_VAL_H(((cur_duty) >> 8) & 0xFF); // 比较值
    STMR1_CMPAL = STMR_CMPA_VAL_L(((cur_duty) >> 0) & 0xFF); // 比较值
    STMR_LOADEN |= STMR_1_LOAD_EN(0x1);                      // 自动装载使能
}

// 14脚的PWM调节，更新14脚的PWM占空比
void refresh_pin14_pwm_duty(void)
{
    STMR0_CMPAH = STMR_CMPA_VAL_H(((cur_duty) >> 8) & 0xFF); // 比较值
    STMR0_CMPAL = STMR_CMPA_VAL_L(((cur_duty) >> 0) & 0xFF); // 比较值
    STMR_LOADEN |= STMR_0_LOAD_EN(0x1);                      // 自动装载使能
}

// 根据9脚的电压来调节PWM
void _My_Adjust_Pwm(float Val)
{
    float P9_Vol = Val * 0.0012;

#if USE_MY_DEBUG
    printf("9脚检测到的电压： %f V\n", P9_Vol);
#endif

    // 当9脚电压大于1.6V时，
    // 若使用PWM1，14脚输出100%占空比的PWM信号
    // 若使用PWM2，15脚输出100%占空比的PWM信号
    // 若使用PWM1+PWM2，14脚、15脚各输出50%占空比的PWM信号
    if (P9_Vol > 1.6) // 输出100%
    {
        //	printf(" P9_Vol : %f.... 100\n",P9_Vol);
        if (FLAG_CUR_PWM1 == flag_cur_use_pwm ||
            FLAG_CUR_PWM2 == flag_cur_use_pwm)
        {
            adjust_duty = PWM_DUTY_100_PERCENT;
        }
        else if (FLAG_CUR_PWM1_PWM2 == flag_cur_use_pwm)
        {
            adjust_duty = PWM_DUTY_100_PERCENT / 2;
        }

        max_flag = 1;
    }
#if 0                      // 客户删去的功能
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

        if (cur_duty >= adjust_duty)
            jump_flag = 1;
    }
#endif                     // 客户删去的功能
    else if (P9_Vol < 1.6) // 缓降50%  并且维持50%
    {
        // 9脚电压小于1.6V，
        // 若使用PWM1，14脚输出50%占空比的PWM信号
        // 若使用PWM2，15脚输出50%占空比的PWM信号
        // 若使用PWM1+PWM2，14脚、15脚各输出25%占空比的PWM信号
        if (FLAG_CUR_PWM1 == flag_cur_use_pwm ||
            FLAG_CUR_PWM2 == flag_cur_use_pwm)
        {
            adjust_duty = PWM_DUTY_50_PERCENT;
            max_flag = 0;
        }
        else if (FLAG_CUR_PWM1_PWM2 == flag_cur_use_pwm)
        {
            adjust_duty = PWM_DUTY_50_PERCENT / 2;
            max_flag = 0;
        }

        if (cur_duty >= adjust_duty)
            jump_flag = 1;
    }

    ///////////////控制16脚//////////////////
    // 当9脚电压高于2.7V时，16脚输出1KHz 高电平,用于控制Q2的导通（用于控制外围的器件，实现关机）
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
void adjust_pwm_duty(void)
{
    if (cur_duty > adjust_duty)
    {
        cur_duty--;
    }
    if (cur_duty < adjust_duty)
    {
        cur_duty++;
    }

    if (FLAG_CUR_PWM1 == flag_cur_use_pwm)
    {
        refresh_pin14_pwm_duty();
    }
    else if (FLAG_CUR_PWM2 == flag_cur_use_pwm)
    {
        refresh_pin15_pwm_duty();
    }
    else if (FLAG_CUR_PWM1_PWM2 == flag_cur_use_pwm)
    {
        refresh_pin14_pwm_duty();
        refresh_pin15_pwm_duty();
    }

    if (FLAG_CUR_PWM1 == flag_cur_use_pwm || FLAG_CUR_PWM2 == flag_cur_use_pwm)
    {
        if (cur_duty >= 5800)
        {
            // delay_ms(15); // 时间还需要测试调整一下
            delay_ms(7);
        }
        else
        {
            // delay_ms(5);
            delay_ms(3);
        }
    }
    else if (FLAG_CUR_PWM1_PWM2 == flag_cur_use_pwm)
    {
        if (cur_duty >= 2900)
        {
            delay_ms(7);
        }
        else
        {
            delay_ms(3);
        }
    }
}
