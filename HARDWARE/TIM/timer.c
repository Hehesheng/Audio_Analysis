#include "timer.h"

// arr：自动重装值。
// psc：时钟预分频数
//定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft us.
// Ft=定时器工作频率,单位:Mhz
void TIM2_Init(uint32_t arr) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);  ///使能TIM时钟

    TIM_TimeBaseInitStructure.TIM_Period = arr;   //自动重装载值
    TIM_TimeBaseInitStructure.TIM_Prescaler = 0;  //定时器分频
    TIM_TimeBaseInitStructure.TIM_CounterMode =
        TIM_CounterMode_Up;  //向上计数模式
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;

    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);  //初始化TIM

    TIM_Cmd(TIM2, ENABLE);  //使能定时器
}
