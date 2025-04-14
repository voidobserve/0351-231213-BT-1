#include "rfdecode.h"

// #include <stdbit.h>

// 这里使用tmr2产生50us的中断，扫描rf信号

#define RECV_RF_PIN P00            // 接收rf信号的引脚
volatile bit is_recv_rf_data = 0;  // 是否接收完成一次rf信号的标志位,0--未接收完成，1--接收完成
volatile u32 rf_data = 0xFFFFFFFF; // 存放接收到的rf数据

/**
 * @brief 配置定时器TMR2
 */
void tmr2_config(void)
{
    __EnableIRQ(TMR2_IRQn); // 使能timer中断
    IE_EA = 1;              // 使能总中断

#define PEROID_VAL (SYSCLK / 128 / 20000 - 1) // 周期值=系统时钟/分频/频率 - 1
    // 设置timer的计数功能，配置一个频率为20kHz的中断，50us产生一次中断
    TMR_ALLCON = TMR2_CNT_CLR(0x1);                        // 清除计数值
    TMR2_PRH = TMR_PERIOD_VAL_H((PEROID_VAL >> 8) & 0xFF); // 周期值
    TMR2_PRL = TMR_PERIOD_VAL_L((PEROID_VAL >> 0) & 0xFF);
    TMR2_CONH = TMR_PRD_PND(0x1) | TMR_PRD_IRQ_EN(0x1);                          // 计数等于周期时允许发生中断
    TMR2_CONL = TMR_SOURCE_SEL(0x7) | TMR_PRESCALE_SEL(0x7) | TMR_MODE_SEL(0x1); // 选择系统时钟，128分频，计数模式

    TMR2_CONL &= ~(TMR_SOURCE_SEL(0x07)); // 清除定时器的时钟源配置寄存器
    TMR2_CONL |= TMR_SOURCE_SEL(0x06);    // 配置定时器的时钟源，使用系统时钟（约21MHz）
}

// 扫描rf信号
void rf_scan(void)
{
    if (is_recv_rf_data)
    {
        // 处理接收到的rf数据

        is_recv_rf_data = 0;
    }
}

// rf信号处理
void rf_handle(void)
{

}

// 定时器TMR中断服务函数
void TIMR2_IRQHandler(void) interrupt TMR2_IRQn
{
    static bit bit_end;
    static u8 Pluse_H_cnt, Pluse_L_cnt; // 高低电平计数
    static u8 rf_bit_cnt;               // 数据位计数（总共应为24bits）

    static bit f_levelbuf;
    bit flagerror = 0;   // 标志位，表示接收错误
    bit flagsuccess = 0; // 标志位，

#define MAX_PULSE 30
#define MIN_PULSE 3     // 最小脉宽不应小于滤波宽度
#define FILTER_CNT 0X07 // 滤波因数 0b_0111
    static u8 filter;
    static bit f_level;

    // 进入中断设置IP，不可删除
    __IRQnIPnPush(TMR2_IRQn);

    // ---------------- 用户函数处理 -------------------
    // 周期中断
    if (TMR2_CONH & TMR_PRD_PND(0x1))
    {
        TMR2_CONH |= TMR_PRD_PND(0x1); // 清除pending

#if 1
        filter <<= 1;
        if (RECV_RF_PIN)
            filter |= 0x01;

        filter &= FILTER_CNT;
        if (filter == FILTER_CNT)
        {
            f_level = 1; // 高电平
        }
        else if (filter == 0x00)
        {
            f_level = 0; // 低电平
        }

        if (f_level) // 高电平判断（如果当前是高电平）
        {
            if (bit_end) // 一位结束
            {
                bit_end = 0;
                if (Pluse_L_cnt >= MIN_PULSE)
                {
                    rf_data <<= 1;
                    if (Pluse_H_cnt > Pluse_L_cnt)
                    {
                        rf_data |= 0x01;
                    }
                    else
                    {
                        rf_data &= ~(0x01);
                    }
                    Pluse_H_cnt = 0;
                }
                else // 低电平小于150us认为出错
                {
                    flagerror = 1;
                }
            }
            else
            {
                f_levelbuf = 1;
                Pluse_H_cnt++;
                if (Pluse_H_cnt >= MAX_PULSE)
                {
                    flagerror = 1;
                }
                Pluse_L_cnt = 0;
            }
        }
        else // 低电平判断（如果当前是低电平）
        {
            Pluse_L_cnt++;
            if (Pluse_L_cnt >= MAX_PULSE) // 低电平超出最大值
            {
                if (rf_bit_cnt != 25)
                {
                    flagerror = 1;
                    rf_data = 0xFFFFFFFF;
                }
                else // 解码成功
                {
                    rf_data &= 0xFFFFFF; // 只保留低24位的数据，清除之前的数据残留
                    is_recv_rf_data = 1; // 表示接收完成一次数据，让其他任务来扫描该标志位，进行相应处理
                    flagsuccess = 1;     //
                }
            }
            else
            {
                if (f_levelbuf)
                {
                    f_levelbuf = 0;
                    Pluse_L_cnt = 0;
                    bit_end = 1;
                    if (Pluse_H_cnt >= MIN_PULSE)
                    {
                        rf_bit_cnt++;
                        if (rf_bit_cnt >= 26)
                        {
                            flagerror = 1;
                        }
                    }
                    else
                    {
                        flagerror = 1;
                    }
                }
            }
        }

        if (flagerror || flagsuccess) // 出错/收尾处理
        {
            rf_bit_cnt = 0;
            Pluse_H_cnt = 0;
            Pluse_L_cnt = 0;
            f_levelbuf = 0;
            bit_end = 0;
        }
#endif
    } // if (TMR2_CONH & TMR_PRD_PND(0x1))

    // 退出中断设置IP，不可删除
    __IRQnIPnPop(TMR2_IRQn);
}
