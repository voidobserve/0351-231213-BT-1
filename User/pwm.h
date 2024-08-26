#ifndef _PWM_H
#define _PWM_H

#include "include.h"
#include <stdio.h>

extern bit jump_flag;
extern u16 adjust_duty; // Ҫ��������ռ�ձ�
extern u16 c_duty;      // ��ǰ���õ�ռ�ձ�

#define MAX_DUTY (6000) // 100%ռ�ձ�
enum
{
    PWM_DUTY_100_PERCENT = 6000, // 100%ռ�ձ�
    PWM_DUTY_75_PERCENT = 4500,  // 75%ռ�ձ�
    PWM_DUTY_50_PERCENT = 3000,  // 50%ռ�ձ�
    PWM_DUTY_25_PERCENT = 1500   // 25%ռ�ձ�
};

void pwm_init(void);
void _My_Adjust_Pwm(float Val);
void set_pwm_duty(void);
void Adaptive_Duty(void);

void according_pin9_to_adjust_pin16(void); // ����9�ŵĵ�ѹ���趨16�ŵĵ�ƽ

#endif