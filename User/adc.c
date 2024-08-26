#include "adc.h"
#include "my_config.h"

float Vol_val = 0;
u16 Vol_val_all[10];   // 存放ad值正常的数组
u16 Vol_val_jump[10];  // 存放ad值跳变的数组
u8 index_jump = 0;     // 跳变的数组的元素计数值
u8 index = 0;          // ad值正常的数组的元素计数值
volatile u16 adc0_val; // adc采集到的电压值

// 存放温度状态的变量
volatile u8 temp_status = TEMP_NORMAL;

// 冒泡排序（升序）
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

// adc相关的引脚配置
void adc_pin_config(void)
{
    // P30--8脚配置为模拟输入模式
    P3_MD0 |= GPIO_P30_MODE_SEL(0x3);

    // P27--9脚配置为模拟输入模式
    P2_MD1 |= GPIO_P27_MODE_SEL(0x3);
}

// 切换adc采集的引脚，配置好adc
// 参数可以选择：
// ADC_SEL_PIN_GET_TEMP
// ADC_SEL_PIN_GET_VOL
void adc_sel_pin(const u8 adc_sel)
{
    // 切换采集引脚时，把之前采集到的ad值清空
    adc0_val = 0;

    switch (adc_sel)
    {
    case ADC_SEL_PIN_GET_TEMP: // 采集热敏电阻对应的电压的引脚（8脚）

        // ADC配置
        ADC_ACON1 &= ~(ADC_VREF_SEL(0x7) | ADC_EXREF_SEL(0) | ADC_INREF_SEL(0)); // 关闭外部参考电压，关闭内部参考电压
        ADC_ACON1 |= ADC_VREF_SEL(0x6) |                                         // 选择内部参考电压VCCA
                     ADC_TEN_SEL(0x3);                                           // 关闭测试信号
        ADC_ACON0 = ADC_CMP_EN(0x1) |                                            // 打开ADC中的CMP使能信号
                    ADC_BIAS_EN(0x1) |                                           // 打开ADC偏置电流能使信号
                    ADC_BIAS_SEL(0x1);                                           // 偏置电流：1x

        ADC_CHS0 = ADC_ANALOG_CHAN(0x18) | // 选则引脚对应的通道（0x18--P30）
                   ADC_EXT_SEL(0x0);       // 选择外部通道
        ADC_CFG0 |= ADC_CHAN0_EN(0x1) |    // 使能通道0转换
                    ADC_EN(0x1);           // 使能A/D转换
        break;

    case ADC_SEL_PIN_GET_VOL: // 检测回路电压的引脚（9脚）

        // ADC配置
        ADC_ACON1 &= ~(ADC_VREF_SEL(0x7) | ADC_EXREF_SEL(0) | ADC_INREF_SEL(0)); // 关闭外部参考电压
        ADC_ACON1 |= ADC_VREF_SEL(0x6) |                                         // 选择内部参考电压VCCA
                     ADC_TEN_SEL(0x3);
        ADC_ACON0 = ADC_CMP_EN(0x1) |      // 打开ADC中的CMP使能信号
                    ADC_BIAS_EN(0x1) |     // 打开ADC偏置电流能使信号
                    ADC_BIAS_SEL(0x1);     // 偏置电流：1x
        ADC_CHS0 = ADC_ANALOG_CHAN(0x17) | // 选则引脚对应的通道（0x17--P27）
                   ADC_EXT_SEL(0x0);       // 选择外部通道
        ADC_CFG0 |= ADC_CHAN0_EN(0x1) |    // 使能通道0转换
                    ADC_EN(0x1);           // 使能A/D转换
        break;
    }

    delay_ms(1); // 等待ADC稳定
}

// adc完成一次转换
// 转换好的值放入全局变量 adc0_val 中
// 需要注意，这款芯片的adc不能频繁采集，需要延时一下再采集一次
void adc_single_getval(void)
{
    ADC_CFG0 |= ADC_CHAN0_TRG(0x1); // 触发ADC0转换
    while (!(ADC_STA & ADC_CHAN0_DONE(0x1)))
        ;                                             // 等待转换完成
    adc0_val = (ADC_DATAH0 << 4) | (ADC_DATAL0 >> 4); // 读取channel0的值
    ADC_STA = ADC_CHAN0_DONE(0x1);                    // 清除ADC0转换完成标志位
}

// 清除缓冲区以及标志位
// 在不根据9脚电压来调节PWM占空比时使用
void __clear_buff(void)
{
    // 清空跳变数组的内容和下标
    memset(Vol_val_jump, 0, sizeof(Vol_val_jump));
    index_jump = 0;
    // 清空正常值数组的内容和下标
    memset(Vol_val_all, 0, sizeof(Vol_val_all));
    index = 0;
}

// 采集一次adc值，如果累计采集了10次跳变/未跳变的adc值，则进行滤波，再根据滤波后的结果进行PWM调节
void adc_scan_according_pin9(void)
{
    ADC_CFG0 |= ADC_CHAN0_TRG(0x1); // 触发ADC0转换
    while (!(ADC_STA & ADC_CHAN0_DONE(0x1)))
        ;                                             // 等待转换完成
    ADC_STA = ADC_CHAN0_DONE(0x1);                    // 清除ADC0转换完成标志位
    adc0_val = (ADC_DATAH0 << 4) | (ADC_DATAL0 >> 4); // 读取channel0的值

    // 判断ad值是否跳变，跳变了放在跳变的数组，未跳变放在正常值数组    0.5v换算ad值大概是400左右，因此设定100   0.12v
    if (((adc0_val - Vol_val) > 100) || ((Vol_val - adc0_val) > 100))
    {
        Vol_val_jump[index_jump] = adc0_val;
        index_jump++;
    }
    else
    {
        Vol_val_all[index] = adc0_val;
        index++;

        // 清空跳变数组的内容和下标
        memset(Vol_val_jump, 0, sizeof(Vol_val_jump));
        index_jump = 0;
    }

    // 判断连续的次数，跳变的值先到达10还是正常的值先到达10，若跳变值先到达值说明是调电压导致的，反之是由于电压波动导致的，则继续输出正常值数组
    if (index >= 10)
    {
        u32 sum = 0;
        u8 i = 0;
        // 清空跳变数组的内容和下标
        memset(Vol_val_jump, 0, sizeof(Vol_val_jump));
        index_jump = 0;

        Pubble_Sort(Vol_val_all, 10); // 冒泡排序
#if 0
		printf("--------------------------------\n");
		for(i = 0 ; i<10 ; i++)
		{
			printf( "Vol_val_all[%bd] %d\n",i,Vol_val_all[i]);
		}
		printf("--------------------------------\n");
#endif
        for (i = 1; i < 9; i++) // 去掉最大值和最小值
        {
            sum += Vol_val_all[i];
        }

        Vol_val = sum >> 3; // 相当于除以8，对数组中的元素求了平均值

        sum = 0;
        index = 0;
        _My_Adjust_Pwm(Vol_val); // 根据滤波好的电压值进行PWM的调节
    }
    else
    {
        if (index_jump >= 10)
        {
            u32 sum = 0;
            u8 i = 0;
            // 清空正常值数组的内容和下标
            memset(Vol_val_all, 0, sizeof(Vol_val_all));
            index = 0;

            Pubble_Sort(Vol_val_jump, 10); // 冒泡排序

            for (i = 1; i < 9; i++) // 去掉最大值和最小值
            {
                sum += Vol_val_jump[i];
            }

            Vol_val = sum >> 3; // 相当于除以8，对数组中的元素求了平均值

            sum = 0;
            index_jump = 0;
            _My_Adjust_Pwm(Vol_val); // 根据滤波好的电压值进行PWM的调节
        }
    }

    adc0_val = 0;
}

// 从引脚上采集滤波后的电压值
u32 get_voltage_from_pin(void)
{
    u8 i = 0;                      // 循环计数值
    volatile u32 adc_aver_val = 0; // 存放adc滤波后的值
    // 采集热敏电阻的电压
    for (i = 0; i < 3; i++)
    {
        adc_single_getval(); // adc单次转换

        // adc滤波
        adc_aver_val += adc0_val;
        if (i >= 1)
        {
            adc_aver_val >>= 1;
        }
        delay_ms(20); // 要加上延时，否则频繁采集adc会有很大的误差
    }

    // 将滤波后的adc值转换成电压值
    // 4095（adc转换后，可能出现的最大的值） * 0.00122 == 4.9959，约等于5V（VCC）
    // return adc_aver_val * 122 / 100;

    // 4095（adc转换后，可能出现的最大的值） * 0.0012 == 4.914，约等于5V（VCC）
    return adc_aver_val * 12 / 10;
}

void adc_scan(void)
{
    volatile u32 voltage = 0; // 存放adc采集到的电压，单位：mV

    if (TEMP_75_30MIN == temp_status)
    {
        // 如果已经超过75摄氏度且超过30min，不用再检测8脚的电压，等待用户排查原因，再重启（重新上电）
        return;
    }

    adc0_val = 0;
    adc_sel_pin(ADC_SEL_PIN_GET_TEMP); // 先切换成热敏电阻对应的引脚的adc配置
    // voltage = get_voltage_from_pin();  // 采集热敏电阻上的电压

#if USE_MY_DEBUG
    printf("PIN-8 voltage: %lu mV\n", voltage);
#endif // USE_MY_DEBUG

    // 如果之前的温度为正常，检测是否超过75摄氏度（±5摄氏度）
    if (TEMP_NORMAL == temp_status && voltage < VOLTAGE_TEMP_75)
    {
        // 如果检测到温度大于75摄氏度（测得的电压值要小于75摄氏度对应的电压值）

        // 检测10次，如果10次都小于这个电压值，才说明温度真的大于75摄氏度
        u8 i = 0;
        for (i = 0; i < 10; i++)
        {
            voltage = get_voltage_from_pin(); // 采集热敏电阻上的电压
            if (voltage > VOLTAGE_TEMP_75)
            {
                // 只要有一次温度小于75摄氏度，就认为温度没有大于75摄氏度
                temp_status = TEMP_NORMAL;
                return;
            }
        }

        // 如果运行到这里，说明温度确实大于75摄氏度
#if USE_MY_DEBUG
        printf("温度超过了75摄氏度\n");
        printf("此时采集到的电压值：%lu mV", voltage);
#endif
        temp_status = TEMP_75; // 状态标志设置为超过75摄氏度
        __clear_buff();        // 清除9脚电压检测使用到的缓冲区和标志位
        return;                // 函数返回，让调节占空比的函数先进行调节
    }
    else if (temp_status == TEMP_75)
    {
        // 如果之前的温度超过75摄氏度
        static bit tmr1_is_open = 0;

        if (0 == tmr1_is_open)
        {
            tmr1_is_open = 1;
            tmr1_cnt = 0;
            tmr1_enable(); // 打开定时器，开始记录是否大于75摄氏度且超过30min
        }

        // while (1) // 这个while循环会影响到9脚调节16脚电压的功能
        // {
#if 0 // 这里的代码在客户那边反而出现问题，超过90摄氏度且1个小时都没有将PWM降到25%，
      // 可能是用户那边的电压有跳变，导致这里清空了定时器计数
            if (voltage > VOLTAGE_TEMP_75)
            {
                // 只要有一次温度小于75摄氏度，就认为温度没有大于75摄氏度
                temp_status = TEMP_75; // 温度标记为超过75摄氏度，但是没有累计30min
                tmr1_disable();        // 关闭定时器
                tmr1_cnt = 0;          // 清空时间计数值
#if USE_MY_DEBUG
                printf("在温度超过了75摄氏度时，检测到有一次温度没有超过75摄氏度\n");
                printf("此时采集到的电压值：%lu mV\n", voltage);
#endif
                return;
            }
#endif

        if (tmr1_cnt >= (u32)TMR1_CNT_30_MINUTES)
        {
            // 如果超过75摄氏度并且过了30min，再检测温度是否超过75摄氏度
#if USE_MY_DEBUG
            printf("温度超过了75摄氏度且超过了30min\n");
            printf("此时采集到的电压值：%lu mV\n", voltage);
#endif
            u8 i = 0;
            for (i = 0; i < 10; i++)
            {
                voltage = get_voltage_from_pin(); // 采集热敏电阻上的电压
                if (voltage > VOLTAGE_TEMP_75)
                {
                    // 只要有一次温度小于75摄氏度，就认为温度没有大于75摄氏度
                    temp_status = TEMP_75;
                    return;
                }
            }

            // 如果运行到这里，说明上面连续、多次检测到的温度都大于75摄氏度
            temp_status = TEMP_75_30MIN;
            tmr1_disable(); // 关闭定时器
            tmr1_cnt = 0;   // 清空时间计数值
            tmr1_is_open = 0;
            return;
        }
        // }  // while(1)
    }
}

// 根据温度（电压值扫描）或9脚的状态来设定占空比
void set_duty(void)
{
    static bit tmr0_is_open = 0;

    // 如果温度正常，根据9脚的状态来调节PWM占空比
    if (TEMP_NORMAL == temp_status)
    {
        if (tmr0_is_open == 0)
        {
            tmr0_is_open = 1;
            tmr0_enable(); // 打开定时器0，开始根据9脚的状态来调节PWM脉宽
        }

        if (tmr0_flag == 1)
        {
            tmr0_flag = 0;
            adc0_val = 0;                     // 清除之前采集到的电压值
            adc_sel_pin(ADC_SEL_PIN_GET_VOL); // 切换到9脚对应的adc配置
            adc_scan_according_pin9();
            // 设定占空比
            while (c_duty != adjust_duty)
            {
                Adaptive_Duty(); // 调节占空比
            }
        }

#if USE_MY_DEBUG
        // printf("cur duty: %d\n", c_duty);
#endif
    }
    else if (TEMP_75 == temp_status)
    {
        // 如果温度超过了75摄氏度且累计10min
        tmr0_disable(); // 关闭定时器0，不以9脚的电压来调节PWM
        tmr0_is_open = 0;
        // 设定占空比
        adjust_duty = PWM_DUTY_50_PERCENT;
        while (c_duty != adjust_duty)
        {
            Adaptive_Duty(); // 调节占空比
        }
    }
    else if (TEMP_75_30MIN == temp_status)
    {
        tmr0_disable(); // 关闭定时器0，不以9脚的电压来调节PWM
        tmr0_is_open = 0;
        // 设定占空比
        adjust_duty = PWM_DUTY_25_PERCENT;
        while (c_duty != adjust_duty)
        {
            Adaptive_Duty(); // 调节占空比
        }
    }
}
