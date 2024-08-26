#include "pwm.h"
#include "time0.h"

volatile u16 c_duty = 0;         // ��ǰ���õ�ռ�ձ�
volatile u16 adjust_duty = 6000; // ����Ҫ���ڳɵ�ռ�ձ�
bit jump_flag = 0;
bit max_flag = 0; // ���ռ�ձȵı�־λ

void pwm_init(void)
{
    STMR_CNTCLR |= STMR_0_CNT_CLR(0x1);
#define STMR0_PEROID_VAL (SYSCLK / 8000 - 1)
    STMR0_PSC = STMR_PRESCALE_VAL(0x07);
    STMR0_PRH = STMR_PRD_VAL_H((STMR0_PEROID_VAL >> 8) & 0xFF);
    STMR0_PRL = STMR_PRD_VAL_L((STMR0_PEROID_VAL >> 0) & 0xFF);
    STMR0_CMPAH = STMR_CMPA_VAL_H(((0) >> 8) & 0xFF); // �Ƚ�ֵ
    STMR0_CMPAL = STMR_CMPA_VAL_L(((0) >> 0) & 0xFF); // �Ƚ�ֵ
    STMR_PWMVALA |= STMR_0_PWMVALA(0x1);

    STMR_CNTMD |= STMR_0_CNT_MODE(0x1); // ��������ģʽ
    STMR_LOADEN |= STMR_0_LOAD_EN(0x1); // �Զ�װ��ʹ��
    STMR_CNTCLR |= STMR_0_CNT_CLR(0x1); //
    STMR_CNTEN |= STMR_0_CNT_EN(0x1);   // ʹ��
    STMR_PWMEN |= STMR_0_PWM_EN(0x1);   // PWM���ʹ��
    P1_MD1 &= ~GPIO_P16_MODE_SEL(0x03);
    P1_MD1 |= GPIO_P16_MODE_SEL(0x01);
    P1_MD1 &= ~GPIO_P14_MODE_SEL(0x03);
    P1_MD1 |= GPIO_P14_MODE_SEL(0x01);
    FOUT_S14 = GPIO_FOUT_AF_FUNC;      // AF�������
    FOUT_S16 = GPIO_FOUT_STMR0_PWMOUT; // stmr0_pwmout
}

// 14�ŵ�PWM����
void set_pwm_duty(void)
{
    STMR0_CMPAH = STMR_CMPA_VAL_H(((c_duty) >> 8) & 0xFF); // �Ƚ�ֵ
    STMR0_CMPAL = STMR_CMPA_VAL_L(((c_duty) >> 0) & 0xFF); // �Ƚ�ֵ
    STMR_LOADEN |= STMR_0_LOAD_EN(0x1);                    // �Զ�װ��ʹ��
}

// ����9�ŵĵ�ѹ������PWM
void _My_Adjust_Pwm(float Val)
{
    float P9_Vol = Val * 0.0012;

#if USE_MY_DEBUG
    printf("9�ż�⵽�ĵ�ѹ�� %f V\n", P9_Vol);
#endif

    //	printf("P9_Vol %d\n",P9_Vol);
    ////////////����14��///////////////////   100 -> 80   7.5ms          80 -> 50  5ms     100 -> 50  3ms

    // ��9�ŵ�ѹ����1.9Vʱ��14�����100%��PWM�ź�
    if (P9_Vol > 1.90) // ���100%
    {
        //	printf(" P9_Vol : %f.... 100\n",P9_Vol);
        adjust_duty = 6000;
        max_flag = 1;
    }
    else if (P9_Vol > 1.6 && P9_Vol < 1.9) // ����80%
    {
        // 9�ŵ�ѹ��1.6~1.9Vʱ��14�������ռ�ձȴ�100%������80%

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
    else if (P9_Vol < 1.6) // ����50%  ����ά��50%
    {
        // 9�ŵ�ѹС��1.6V��14�����ռ�ձȴ�80%������50%��������50%

        // printf(" P9_Vol : %f...... 50\n",P9_Vol);
        // adjust_duty = 3000;
        adjust_duty = PWM_DUTY_50_PERCENT;
        max_flag = 0;
        if (c_duty >= adjust_duty)
            jump_flag = 1;
    }
    ///////////////����16��//////////////////
    // ��9�ŵ�ѹ����2.7Vʱ��16�����1KHz �ߵ�ƽ,���ڿ���Q2�ĵ�ͨ��
    if (P9_Vol > 2.7) // 16�����1KHZ�ĸߵ�ƽ
    {
        P14 = 1;
    }
    else // 16�����1KHZ�ĵ͵�ƽ
    {
        P14 = 0;
    }
}

// ����9�ŵĵ�ѹ���趨16�ŵĵ�ƽ
void according_pin9_to_adjust_pin16(void)
{
    u8 i = 0;                         // ѭ������ֵ
    u8 cnt = 0;                       // ����ֵ
    volatile u32 adc_aver_val = 0;    // ���adc�˲����ֵ
    adc_sel_pin(ADC_SEL_PIN_GET_VOL); // �л���9�Ŷ�Ӧ��adc����

    // �ɼ���ѹ
    for (i = 0; i < 10; i++)
    {
        adc_aver_val = get_voltage_from_pin();
        if (adc_aver_val >= 2700)
        {
            // �����9���ϲɼ��ĵ�ѹ����2.7V
            cnt++;
        }
    }

    // ��9�ŵ�ѹ����2.7Vʱ��16�����1KHz �ߵ�ƽ,���ڿ���Q2�ĵ�ͨ�����ڹػ�����
    if (cnt == 10)
    {
        // �����μ�ⶼ��������������Ϊ9�ŵĵ�ѹȷʵ����2.7V
        P14 = 1;
    }
    else
    {
        P14 = 0;
    }
}

// ��������ռ�ձȣ����������ͻ����½���
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

        delay_ms(15); // ʱ�仹��Ҫ���Ե���һ��
    }
    else
    {
        delay_ms(5);
    }
}
