#ifndef _PWM_H
#define _PWM_H

#include "include.h"
#include <stdio.h>

extern bit jump_flag;
extern u16 adjust_duty; // 要调整到的占空比
extern u16 c_duty;      // 当前设置的占空比

#define MAX_DUTY (6000) // 100%占空比
enum
{
    PWM_DUTY_100_PERCENT = 6000, // 100%占空比
    PWM_DUTY_75_PERCENT = 4500,  // 75%占空比
    PWM_DUTY_50_PERCENT = 3000,  // 50%占空比
    PWM_DUTY_25_PERCENT = 1500   // 25%占空比
};

void pwm_init(void);
void _My_Adjust_Pwm(float Val);
void set_pwm_duty(void);
void Adaptive_Duty(void);

void according_pin9_to_adjust_pin16(void); // 根据9脚的电压来设定16脚的电平

#endif