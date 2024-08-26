#include "adc.h"
#include "my_config.h"

float Vol_val = 0;
u16 Vol_val_all[10];   // ���adֵ����������
u16 Vol_val_jump[10];  // ���adֵ���������
u8 index_jump = 0;     // ����������Ԫ�ؼ���ֵ
u8 index = 0;          // adֵ�����������Ԫ�ؼ���ֵ
volatile u16 adc0_val; // adc�ɼ����ĵ�ѹֵ

// ����¶�״̬�ı���
volatile u8 temp_status = TEMP_NORMAL;

// ð����������
void Pubble_Sort(u16 *arr, u8 length)
{
    int i = 0;
    int j = 0;
    for (i = 0; i < length - 1; i++)
    {
        for (j = 0; j < length - 1 - i; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

// adc��ص���������
void adc_pin_config(void)
{
    // P30--8������Ϊģ������ģʽ
    P3_MD0 |= GPIO_P30_MODE_SEL(0x3);

    // P27--9������Ϊģ������ģʽ
    P2_MD1 |= GPIO_P27_MODE_SEL(0x3);
}

// �л�adc�ɼ������ţ����ú�adc
// ��������ѡ��
// ADC_SEL_PIN_GET_TEMP
// ADC_SEL_PIN_GET_VOL
void adc_sel_pin(const u8 adc_sel)
{
    // �л��ɼ�����ʱ����֮ǰ�ɼ�����adֵ���
    adc0_val = 0;

    switch (adc_sel)
    {
    case ADC_SEL_PIN_GET_TEMP: // �ɼ����������Ӧ�ĵ�ѹ�����ţ�8�ţ�

        // ADC����
        ADC_ACON1 &= ~(ADC_VREF_SEL(0x7) | ADC_EXREF_SEL(0) | ADC_INREF_SEL(0)); // �ر��ⲿ�ο���ѹ���ر��ڲ��ο���ѹ
        ADC_ACON1 |= ADC_VREF_SEL(0x6) |                                         // ѡ���ڲ��ο���ѹVCCA
                     ADC_TEN_SEL(0x3);                                           // �رղ����ź�
        ADC_ACON0 = ADC_CMP_EN(0x1) |                                            // ��ADC�е�CMPʹ���ź�
                    ADC_BIAS_EN(0x1) |                                           // ��ADCƫ�õ�����ʹ�ź�
                    ADC_BIAS_SEL(0x1);                                           // ƫ�õ�����1x

        ADC_CHS0 = ADC_ANALOG_CHAN(0x18) | // ѡ�����Ŷ�Ӧ��ͨ����0x18--P30��
                   ADC_EXT_SEL(0x0);       // ѡ���ⲿͨ��
        ADC_CFG0 |= ADC_CHAN0_EN(0x1) |    // ʹ��ͨ��0ת��
                    ADC_EN(0x1);           // ʹ��A/Dת��
        break;

    case ADC_SEL_PIN_GET_VOL: // ����·��ѹ�����ţ�9�ţ�

        // ADC����
        ADC_ACON1 &= ~(ADC_VREF_SEL(0x7) | ADC_EXREF_SEL(0) | ADC_INREF_SEL(0)); // �ر��ⲿ�ο���ѹ
        ADC_ACON1 |= ADC_VREF_SEL(0x6) |                                         // ѡ���ڲ��ο���ѹVCCA
                     ADC_TEN_SEL(0x3);
        ADC_ACON0 = ADC_CMP_EN(0x1) |      // ��ADC�е�CMPʹ���ź�
                    ADC_BIAS_EN(0x1) |     // ��ADCƫ�õ�����ʹ�ź�
                    ADC_BIAS_SEL(0x1);     // ƫ�õ�����1x
        ADC_CHS0 = ADC_ANALOG_CHAN(0x17) | // ѡ�����Ŷ�Ӧ��ͨ����0x17--P27��
                   ADC_EXT_SEL(0x0);       // ѡ���ⲿͨ��
        ADC_CFG0 |= ADC_CHAN0_EN(0x1) |    // ʹ��ͨ��0ת��
                    ADC_EN(0x1);           // ʹ��A/Dת��
        break;
    }

    delay_ms(1); // �ȴ�ADC�ȶ�
}

// adc���һ��ת��
// ת���õ�ֵ����ȫ�ֱ��� adc0_val ��
// ��Ҫע�⣬���оƬ��adc����Ƶ���ɼ�����Ҫ��ʱһ���ٲɼ�һ��
void adc_single_getval(void)
{
    ADC_CFG0 |= ADC_CHAN0_TRG(0x1); // ����ADC0ת��
    while (!(ADC_STA & ADC_CHAN0_DONE(0x1)))
        ;                                             // �ȴ�ת�����
    adc0_val = (ADC_DATAH0 << 4) | (ADC_DATAL0 >> 4); // ��ȡchannel0��ֵ
    ADC_STA = ADC_CHAN0_DONE(0x1);                    // ���ADC0ת����ɱ�־λ
}

// ����������Լ���־λ
// �ڲ�����9�ŵ�ѹ������PWMռ�ձ�ʱʹ��
void __clear_buff(void)
{
    // ���������������ݺ��±�
    memset(Vol_val_jump, 0, sizeof(Vol_val_jump));
    index_jump = 0;
    // �������ֵ��������ݺ��±�
    memset(Vol_val_all, 0, sizeof(Vol_val_all));
    index = 0;
}

// �ɼ�һ��adcֵ������ۼƲɼ���10������/δ�����adcֵ��������˲����ٸ����˲���Ľ������PWM����
void adc_scan_according_pin9(void)
{
    ADC_CFG0 |= ADC_CHAN0_TRG(0x1); // ����ADC0ת��
    while (!(ADC_STA & ADC_CHAN0_DONE(0x1)))
        ;                                             // �ȴ�ת�����
    ADC_STA = ADC_CHAN0_DONE(0x1);                    // ���ADC0ת����ɱ�־λ
    adc0_val = (ADC_DATAH0 << 4) | (ADC_DATAL0 >> 4); // ��ȡchannel0��ֵ

    // �ж�adֵ�Ƿ����䣬�����˷�����������飬δ�����������ֵ����    0.5v����adֵ�����400���ң�����趨100   0.12v
    if (((adc0_val - Vol_val) > 100) || ((Vol_val - adc0_val) > 100))
    {
        Vol_val_jump[index_jump] = adc0_val;
        index_jump++;
    }
    else
    {
        Vol_val_all[index] = adc0_val;
        index++;

        // ���������������ݺ��±�
        memset(Vol_val_jump, 0, sizeof(Vol_val_jump));
        index_jump = 0;
    }

    // �ж������Ĵ����������ֵ�ȵ���10����������ֵ�ȵ���10��������ֵ�ȵ���ֵ˵���ǵ���ѹ���µģ���֮�����ڵ�ѹ�������µģ�������������ֵ����
    if (index >= 10)
    {
        u32 sum = 0;
        u8 i = 0;
        // ���������������ݺ��±�
        memset(Vol_val_jump, 0, sizeof(Vol_val_jump));
        index_jump = 0;

        Pubble_Sort(Vol_val_all, 10); // ð������
#if 0
		printf("--------------------------------\n");
		for(i = 0 ; i<10 ; i++)
		{
			printf( "Vol_val_all[%bd] %d\n",i,Vol_val_all[i]);
		}
		printf("--------------------------------\n");
#endif
        for (i = 1; i < 9; i++) // ȥ�����ֵ����Сֵ
        {
            sum += Vol_val_all[i];
        }

        Vol_val = sum >> 3; // �൱�ڳ���8���������е�Ԫ������ƽ��ֵ

        sum = 0;
        index = 0;
        _My_Adjust_Pwm(Vol_val); // �����˲��õĵ�ѹֵ����PWM�ĵ���
    }
    else
    {
        if (index_jump >= 10)
        {
            u32 sum = 0;
            u8 i = 0;
            // �������ֵ��������ݺ��±�
            memset(Vol_val_all, 0, sizeof(Vol_val_all));
            index = 0;

            Pubble_Sort(Vol_val_jump, 10); // ð������

            for (i = 1; i < 9; i++) // ȥ�����ֵ����Сֵ
            {
                sum += Vol_val_jump[i];
            }

            Vol_val = sum >> 3; // �൱�ڳ���8���������е�Ԫ������ƽ��ֵ

            sum = 0;
            index_jump = 0;
            _My_Adjust_Pwm(Vol_val); // �����˲��õĵ�ѹֵ����PWM�ĵ���
        }
    }

    adc0_val = 0;
}

// �������ϲɼ��˲���ĵ�ѹֵ
u32 get_voltage_from_pin(void)
{
    u8 i = 0;                      // ѭ������ֵ
    volatile u32 adc_aver_val = 0; // ���adc�˲����ֵ
    // �ɼ���������ĵ�ѹ
    for (i = 0; i < 3; i++)
    {
        adc_single_getval(); // adc����ת��

        // adc�˲�
        adc_aver_val += adc0_val;
        if (i >= 1)
        {
            adc_aver_val >>= 1;
        }
        delay_ms(20); // Ҫ������ʱ������Ƶ���ɼ�adc���кܴ�����
    }

    // ���˲����adcֵת���ɵ�ѹֵ
    // 4095��adcת���󣬿��ܳ��ֵ�����ֵ�� * 0.00122 == 4.9959��Լ����5V��VCC��
    // return adc_aver_val * 122 / 100;

    // 4095��adcת���󣬿��ܳ��ֵ�����ֵ�� * 0.0012 == 4.914��Լ����5V��VCC��
    return adc_aver_val * 12 / 10;
}

void adc_scan(void)
{
    volatile u32 voltage = 0; // ���adc�ɼ����ĵ�ѹ����λ��mV

    if (TEMP_75_30MIN == temp_status)
    {
        // ����Ѿ�����75���϶��ҳ���30min�������ټ��8�ŵĵ�ѹ���ȴ��û��Ų�ԭ���������������ϵ磩
        return;
    }

    adc0_val = 0;
    adc_sel_pin(ADC_SEL_PIN_GET_TEMP); // ���л������������Ӧ�����ŵ�adc����
    // voltage = get_voltage_from_pin();  // �ɼ����������ϵĵ�ѹ

#if USE_MY_DEBUG
    printf("PIN-8 voltage: %lu mV\n", voltage);
#endif // USE_MY_DEBUG

    // ���֮ǰ���¶�Ϊ����������Ƿ񳬹�75���϶ȣ���5���϶ȣ�
    if (TEMP_NORMAL == temp_status && voltage < VOLTAGE_TEMP_75)
    {
        // �����⵽�¶ȴ���75���϶ȣ���õĵ�ѹֵҪС��75���϶ȶ�Ӧ�ĵ�ѹֵ��

        // ���10�Σ����10�ζ�С�������ѹֵ����˵���¶���Ĵ���75���϶�
        u8 i = 0;
        for (i = 0; i < 10; i++)
        {
            voltage = get_voltage_from_pin(); // �ɼ����������ϵĵ�ѹ
            if (voltage > VOLTAGE_TEMP_75)
            {
                // ֻҪ��һ���¶�С��75���϶ȣ�����Ϊ�¶�û�д���75���϶�
                temp_status = TEMP_NORMAL;
                return;
            }
        }

        // ������е����˵���¶�ȷʵ����75���϶�
#if USE_MY_DEBUG
        printf("�¶ȳ�����75���϶�\n");
        printf("��ʱ�ɼ����ĵ�ѹֵ��%lu mV", voltage);
#endif
        temp_status = TEMP_75; // ״̬��־����Ϊ����75���϶�
        __clear_buff();        // ���9�ŵ�ѹ���ʹ�õ��Ļ������ͱ�־λ
        return;                // �������أ��õ���ռ�ձȵĺ����Ƚ��е���
    }
    else if (temp_status == TEMP_75)
    {
        // ���֮ǰ���¶ȳ���75���϶�
        static bit tmr1_is_open = 0;

        if (0 == tmr1_is_open)
        {
            tmr1_is_open = 1;
            tmr1_cnt = 0;
            tmr1_enable(); // �򿪶�ʱ������ʼ��¼�Ƿ����75���϶��ҳ���30min
        }

        // while (1) // ���whileѭ����Ӱ�쵽9�ŵ���16�ŵ�ѹ�Ĺ���
        // {
#if 0 // ����Ĵ����ڿͻ��Ǳ߷����������⣬����90���϶���1��Сʱ��û�н�PWM����25%��
      // �������û��Ǳߵĵ�ѹ�����䣬������������˶�ʱ������
            if (voltage > VOLTAGE_TEMP_75)
            {
                // ֻҪ��һ���¶�С��75���϶ȣ�����Ϊ�¶�û�д���75���϶�
                temp_status = TEMP_75; // �¶ȱ��Ϊ����75���϶ȣ�����û���ۼ�30min
                tmr1_disable();        // �رն�ʱ��
                tmr1_cnt = 0;          // ���ʱ�����ֵ
#if USE_MY_DEBUG
                printf("���¶ȳ�����75���϶�ʱ����⵽��һ���¶�û�г���75���϶�\n");
                printf("��ʱ�ɼ����ĵ�ѹֵ��%lu mV\n", voltage);
#endif
                return;
            }
#endif

        if (tmr1_cnt >= (u32)TMR1_CNT_30_MINUTES)
        {
            // �������75���϶Ȳ��ҹ���30min���ټ���¶��Ƿ񳬹�75���϶�
#if USE_MY_DEBUG
            printf("�¶ȳ�����75���϶��ҳ�����30min\n");
            printf("��ʱ�ɼ����ĵ�ѹֵ��%lu mV\n", voltage);
#endif
            u8 i = 0;
            for (i = 0; i < 10; i++)
            {
                voltage = get_voltage_from_pin(); // �ɼ����������ϵĵ�ѹ
                if (voltage > VOLTAGE_TEMP_75)
                {
                    // ֻҪ��һ���¶�С��75���϶ȣ�����Ϊ�¶�û�д���75���϶�
                    temp_status = TEMP_75;
                    return;
                }
            }

            // ������е����˵��������������μ�⵽���¶ȶ�����75���϶�
            temp_status = TEMP_75_30MIN;
            tmr1_disable(); // �رն�ʱ��
            tmr1_cnt = 0;   // ���ʱ�����ֵ
            tmr1_is_open = 0;
            return;
        }
        // }  // while(1)
    }
}

// �����¶ȣ���ѹֵɨ�裩��9�ŵ�״̬���趨ռ�ձ�
void set_duty(void)
{
    static bit tmr0_is_open = 0;

    // ����¶�����������9�ŵ�״̬������PWMռ�ձ�
    if (TEMP_NORMAL == temp_status)
    {
        if (tmr0_is_open == 0)
        {
            tmr0_is_open = 1;
            tmr0_enable(); // �򿪶�ʱ��0����ʼ����9�ŵ�״̬������PWM����
        }

        if (tmr0_flag == 1)
        {
            tmr0_flag = 0;
            adc0_val = 0;                     // ���֮ǰ�ɼ����ĵ�ѹֵ
            adc_sel_pin(ADC_SEL_PIN_GET_VOL); // �л���9�Ŷ�Ӧ��adc����
            adc_scan_according_pin9();
            // �趨ռ�ձ�
            while (c_duty != adjust_duty)
            {
                Adaptive_Duty(); // ����ռ�ձ�
            }
        }

#if USE_MY_DEBUG
        // printf("cur duty: %d\n", c_duty);
#endif
    }
    else if (TEMP_75 == temp_status)
    {
        // ����¶ȳ�����75���϶����ۼ�10min
        tmr0_disable(); // �رն�ʱ��0������9�ŵĵ�ѹ������PWM
        tmr0_is_open = 0;
        // �趨ռ�ձ�
        adjust_duty = PWM_DUTY_50_PERCENT;
        while (c_duty != adjust_duty)
        {
            Adaptive_Duty(); // ����ռ�ձ�
        }
    }
    else if (TEMP_75_30MIN == temp_status)
    {
        tmr0_disable(); // �رն�ʱ��0������9�ŵĵ�ѹ������PWM
        tmr0_is_open = 0;
        // �趨ռ�ձ�
        adjust_duty = PWM_DUTY_25_PERCENT;
        while (c_duty != adjust_duty)
        {
            Adaptive_Duty(); // ����ռ�ձ�
        }
    }
}
