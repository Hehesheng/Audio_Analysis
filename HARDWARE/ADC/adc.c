#include "adc.h"
#include "delay.h"

//初始化ADC
//这里我们仅以规则通道为例
//我们默认仅开启通道1
void Adc1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); //使能GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);  //使能ADC1时钟

    //先初始化IO口
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;   //模拟输入
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN; //下拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);         //初始化

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, ENABLE);  //ADC1复位
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, DISABLE); //复位结束

    ADC_CommonStructInit(&ADC_CommonInitStructure); // ADC指令使用默认配置
    
    ADC_CommonInit(&ADC_CommonInitStructure);

    ADC_StructInit(&ADC_InitStructure); // ADC指令使用默认配置
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE); //开启AD转换器
}
//获得ADC值
//ch:通道值 0~16
//返回值:转换结果
u16 Get_Adc1(u8 ch)
{
    //设置指定ADC的规则组通道，一个序列，采样时间
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_480Cycles); //ADC1,ADC通道,480个周期,提高采样时间可以提高精确度

    ADC_SoftwareStartConv(ADC1); //使能指定的ADC1的软件转换启动功能

    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)); //等待转换结束

    return ADC_GetConversionValue(ADC1); //返回最近一次ADC1规则组的转换结果
}
//获取通道ch的转换值，取times次,然后平均
//ch:通道编号
//times:获取次数
//返回值:通道ch的times次转换结果平均值
u16 Get_Adc1_Average(u8 ch, u8 times)
{
    u32 temp_val = 0;
    u8 t;
    for (t = 0; t < times; t++)
    {
        temp_val += Get_Adc1(ch);
        delay_ms(5);
    }
    return temp_val / times;
}

double ADC_Run(void)
{
    u16 adc_bits;
    double res;
    adc_bits = Get_Adc1(ADC_Channel_5);
    res = 3.3/255*adc_bits;
    return res;
}
