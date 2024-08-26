#ifndef __MY_CONFIG_H
#define __MY_CONFIG_H

#include "include.h" // оƬ�ٷ��ṩ��ͷ�ļ�
#include "pwm.h"
#include "adc.h"
#include "time0.h" // ��ʱ��0
#include "timer1.h"

#include <stdio.h>

#define USE_MY_DEBUG 0 // �Ƿ�ʹ�ô�ӡ����
// tmr1���ó�ÿ10ms����һ���жϣ�����ֵ��һ��
// ���ﶨ��ʱ���Ӧ�ļ���ֵ

// #define TMR1_CNT_30_MINUTES 180000UL // 30min������ǿ�����30min�����PWMռ�ձȵģ�

#define TMR1_CNT_5_MINUTES 30000UL // 5min

// // �����õļ���ֵ
// #define TMR1_CNT_30_MINUTES 1000 // 10s
// #define TMR1_CNT_30_MINUTES 18000UL // 3min

// ����������˼��ĵ�ѹֵ���¶ȶ�Ӧ�Ĺ�ϵ����ѹֵ��λ��mV
#define VOLTAGE_TEMP_75 (3050) // ��һ��ֵ�ڿͻ��Ǳ߲��Գ�����74���϶�,��Ӧ�ĵ�ѹ��3.1V

// #define VOLTAGE_TEMP_75 (2990) // 75���϶ȶ�Ӧ�ĵ�ѹֵ
// #define VOLTAGE_TEMP_65 (3388) // 65���϶ȶ�Ӧ�ĵ�ѹֵ
// #define VOLTAGE_TEMP_50 (3924) // 50���϶ȶ�Ӧ�ĵ�ѹֵ

// �¶�״̬����
enum
{
    TEMP_NORMAL,   // �����¶�
    TEMP_75,       // ����75���϶ȣ���5���϶ȣ�
    // TEMP_75_30MIN, // ����75���϶ȣ���5���϶ȣ�30min
    TEMP_75_5_MIN, // ����75���϶ȣ���5���϶ȣ�5min
};

#endif
