#ifndef _ADC_H
#define _ADC_H

#include <stdio.h>
#include <include.h>
#include "pwm.h"
#include <string.h>

#define ADC_SEL_PIN_GET_TEMP 0x01 // 根据热敏电阻一端来配置ADC
#define ADC_SEL_PIN_GET_VOL 0x02  // 根据9脚来配置ADC

// void adc_init();
void adc_scan_according_pin9(); // 采集一次adc值，如果累计采集了10次跳变/未跳变的adc值，则进行滤波，再根据滤波后的结果进行PWM调节

void adc_pin_config(void);      // adc相关的引脚配置，调用完成后，还未能使用adc
void adc_sel_pin(const u8 pin); // 切换adc采集的引脚，并配置好adc
void adc_single_getval(void);   // adc完成一次转换

u32 get_voltage_from_pin(void); // 从引脚上采集滤波后的电压值

void adc_scan(void); 
void set_duty(void);

#endif