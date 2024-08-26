#ifndef __MY_CONFIG_H
#define __MY_CONFIG_H

#include "include.h" // 芯片官方提供的头文件
#include "pwm.h"
#include "adc.h"
#include "time0.h" // 定时器0
#include "timer1.h"

#include <stdio.h>

#define USE_MY_DEBUG 0 // 是否使用打印调试
// tmr1配置成每10ms产生一次中断，计数值加一，
// 这里定义时间对应的计数值

#define TMR1_CNT_30_MINUTES 180000UL // 30min（这个是可以在30min后调节PWM占空比的）

// // 测试用的计数值
// #define TMR1_CNT_30_MINUTES 1000 // 10s
// #define TMR1_CNT_30_MINUTES 18000UL // 3min

// 在热敏电阻端检测的电压值与温度对应的关系，电压值单位：mV
#define VOLTAGE_TEMP_75 (3050) // 这一个值在客户那边测试出来是74摄氏度,对应的电压是3.1V

// #define VOLTAGE_TEMP_75 (2990) // 75摄氏度对应的电压值
// #define VOLTAGE_TEMP_65 (3388) // 65摄氏度对应的电压值
// #define VOLTAGE_TEMP_50 (3924) // 50摄氏度对应的电压值

// 温度状态定义
enum
{
    TEMP_NORMAL,   // 正常温度
    TEMP_75,       // 超过75摄氏度（±5摄氏度）
    TEMP_75_30MIN, // 超过75摄氏度（±5摄氏度）30min
};

#endif
