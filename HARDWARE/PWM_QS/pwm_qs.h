#ifndef _QS_H
#define _QS_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////
//������ֻ��ѧϰʹ�ã�δ���������ɣ��������������κ���;
//ALIENTEK STM32F407������
//��ʱ�� ��������
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2014/6/16
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) �������������ӿƼ����޹�˾ 2014-2024
//All rights reserved
//////////////////////////////////////////////////////////////////////////////////
// char s[] = "0110110101100001011001000110010101100010011110010111000101101001011100110110100001101001";

void PWMMODE_84psc(double frequency,double duty_cycle);
void TIM14_PWM_Init(u32 arr,u32 psc);
#endif