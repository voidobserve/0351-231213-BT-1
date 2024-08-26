#ifndef _ADC_H
#define _ADC_H

#include <stdio.h>
#include <include.h>
#include "pwm.h"
#include <string.h>

#define ADC_SEL_PIN_GET_TEMP 0x01 // ������������һ��������ADC
#define ADC_SEL_PIN_GET_VOL 0x02  // ����9��������ADC

// void adc_init();
void adc_scan_according_pin9(); // �ɼ�һ��adcֵ������ۼƲɼ���10������/δ�����adcֵ��������˲����ٸ����˲���Ľ������PWM����

void adc_pin_config(void);      // adc��ص��������ã�������ɺ󣬻�δ��ʹ��adc
void adc_sel_pin(const u8 pin); // �л�adc�ɼ������ţ������ú�adc
void adc_single_getval(void);   // adc���һ��ת��

u32 get_voltage_from_pin(void); // �������ϲɼ��˲���ĵ�ѹֵ

void adc_scan(void); 
void set_duty(void);

#endif