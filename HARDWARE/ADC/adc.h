#ifndef __ADC_H
#define __ADC_H
#include "sys.h"
#include "main.h"

#define ADC_RES_SIZE 8192U

extern uint32_t count;
extern uint16_t adc_res[ADC_RES_SIZE];
extern uint8_t flag;

void Adc1_Init(void);                  //ADC通道初始化
u16 Get_Adc1(u8 ch);                   //获得某个通道值
u16 Get_Adc1_Average(u8 ch, u8 times); //得到某个通道给定次数采样的平均值
double ADC_Run(void);

void Adc1_DMA_Enable(void);
void Adc1_DMA_Init(uint16_t buff_size);

#endif
