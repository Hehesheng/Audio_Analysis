#include "pwm_qs.h"

//////////////////////////////////////////////////////////////////////////////////
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
// ALIENTEK STM32F407开发板
//定时器PWM 驱动代码
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2014/5/4
//版本：V1.0
//版权所有，盗版必究。
// Copyright(C) 广州市星翼电子科技有限公司 2014-2024
// All rights reserved
//////////////////////////////////////////////////////////////////////////////////

// TIM14 PWM部分初始化
// PWM输出初始化
// arr：自动重装值
// psc：时钟预分频数
// psc:分频系数  arr：重装载值
void TIM14_PWM_Init(u32 arr, u32 psc) {
    //此部分需手动修改IO口设置
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14, ENABLE);  // TIM14时钟使能
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);  //使能PORTF时钟

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7,
                     GPIO_AF_TIM14);  // GPIOF9复用为定时器14

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;           // GPIOF9
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        //复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;  //速度100MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        //上拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);              //初始化PF9

    TIM_TimeBaseStructure.TIM_Prescaler = psc;  //定时器分频
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //向上计数模式
    TIM_TimeBaseStructure.TIM_Period = arr;  //自动重装载值
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;

    TIM_TimeBaseInit(TIM14, &TIM_TimeBaseStructure);  //初始化定时器14

    //初始化TIM14 Channel1 PWM模式
    TIM_OCInitStructure.TIM_OCMode =
        TIM_OCMode_PWM1;  //选择定时器模式:TIM脉冲宽度调制模式2
    TIM_OCInitStructure.TIM_OutputState =
        TIM_OutputState_Enable;  //比较输出使能
    TIM_OCInitStructure.TIM_OCPolarity =
        TIM_OCPolarity_Low;  //输出极性:TIM输出比较极性低
    TIM_OC1Init(TIM14,
                &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM1 4OC1

    TIM_OC1PreloadConfig(
        TIM14, TIM_OCPreload_Enable);  //使能TIM14在CCR1上的预装载寄存器

    TIM_ARRPreloadConfig(TIM14, ENABLE);  // ARPE使能

    TIM_Cmd(TIM14, ENABLE);  //使能TIM14
}

//入口参数 ： frequency（频率，以hz为单位）
// duty_cycle（占空比：1到100）
void PWMMODE_84psc(double frequency, double duty_cycle) {
    u32 arr = 500;
    u16 Compare = 250;
    if (frequency < 0)  //防止输入错误
    {
        frequency = 1000;
    }
    arr = (int)1000000 / frequency;          //	整数化处理
    if (duty_cycle > 100 || duty_cycle < 1)  //防止输入错误
    {
        duty_cycle = 50;
    }
    Compare = arr * (duty_cycle * 0.01);  //输出占空比调制
    TIM14_PWM_Init(arr - 1, 84 - 1);      //	输出频率设定
    TIM_SetCompare1(TIM14, Compare - 1);  //修改比较值，修改占空比
}
