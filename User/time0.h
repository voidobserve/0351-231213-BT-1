#ifndef _TIME0_H
#define _TIME0_H

#include "include.h"

#include <stdio.h>
#include "adc.h"

extern volatile bit tmr0_flag;

void tmr0_config(void);
void tmr0_enable(void);  // ������ʱ������ʼ��ʱ
void tmr0_disable(void); // �رն�ʱ������ռ���ֵ


#endif
