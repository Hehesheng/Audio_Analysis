#ifndef __USART_H
#define __USART_H
#include "stdio.h"
#include "stm32f4xx_conf.h"
#include "sys.h"

#ifdef __GNUC__
#define printf(format, str...) \
    do {                       \
        printf(format, ##str); \
        fflush(stdout);        \
    } while (0)
#endif

#define scanfSupport 0        //是否支持scanf
#define USART_REC_LEN 128     //定义最大接收字节数 128 可选值:0~255
#define USART_RX_OK (0X8000)  //接受成功标志位

//接收缓冲,最大USART_REC_LEN个字节.末字节为换行符
extern u8 USART_RX_BUF[USART_REC_LEN];
extern u16 USART_RX_STA;  //接收状态标记

void uart_init(u32 bound);

#endif
