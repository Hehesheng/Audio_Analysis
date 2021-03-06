#include "usart.h"
#include "gpio.h"
#include "main.h"
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////
//如果使用os,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "FreeRTOS.h"  //ucos 使用
#endif

//串口中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误
u8 USART_RX_BUF[USART_REC_LEN];  //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
// bit15,   接收完成标志
// bit14~8, reserved
// bit7~0,  接收到的有效字节数目
u16 USART_RX_STA = 0;  //接收状态标记

/*
 * To implement the STDIO functions you need to create
 * the _read and _write functions and hook them to the
 * USART you are using. This example also has a buffered
 * read function for basic line editing.
 */
int _write(int fd, char *ptr, int len);
int _read(int fd, char *ptr, int len);
void get_buffered_line(void);

/*
 * Called by libc stdio fwrite functions
 */
int _write(int fd, char *ptr, int len) {
    int i = 0;

    /*
     * write "len" of char from "ptr" to file id "fd"
     * Return number of char written.
     *
     * Only work for STDOUT, STDIN, and STDERR
     */
    if (fd > 2) {
        return -1;
    }

    while (*ptr && (i < len)) {
        USART_SendData(USART1, *ptr);

        if (*ptr == '\n') {
            USART_SendData(USART1, '\r');
        }

        i++;
        ptr++;
    }

    return i;
}

/* back up the cursor one space */
static inline void back_up(void) {
    USART_SendData(USART1, '\010');
    USART_SendData(USART1, ' ');
    USART_SendData(USART1, '\010');
    USART_RX_BUF[(USART_RX_STA & 0X7FFF) - 1] = 0;
    USART_RX_STA--;
}

//初始化IO 串口1
// bound:波特率
void uart_init(u32 bound) {
    // GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);   //使能GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);  //使能USART1时钟

    //串口1对应引脚复用映射
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9,
                     GPIO_AF_USART1);  // GPIOA9复用为USART1
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10,
                     GPIO_AF_USART1);  // GPIOA10复用为USART1

    // USART1端口配置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;  // GPIOA9与GPIOA10
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;             //复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;        //速度50MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;    //上拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);          //初始化PA9，PA10

    // USART1 初始化设置
    USART_InitStructure.USART_BaudRate = bound;  //波特率设置
    USART_InitStructure.USART_WordLength =
        USART_WordLength_8b;  //字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;  //一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;     //无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl =
        USART_HardwareFlowControl_None;  //无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  //收发模式
    USART_Init(USART1, &USART_InitStructure);  //初始化串口1

    USART_Cmd(USART1, ENABLE);  //使能串口1

    // USART_ClearFlag(USART1, USART_FLAG_TC);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);  //开启相关中断

    // Usart1 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;  //串口1中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 13;  //抢占优先级13
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;          //子优先级1
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  // IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);  //根据指定的参数初始化VIC寄存器
}

void USART1_IRQHandler(void) {  //串口1中断服务程序
    u8 Res;
    BaseType_t task_woken;
    //接收中断(接收到的数据必须是0x0d 或 0x0a结尾)
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        Res = USART_ReceiveData(USART1);  //读取接收到的数据

        if ((USART_RX_STA & 0x8000) == 0) {    //接收未完成
            if (Res == 0x0d || Res == 0x0a) {  //接收到了回车
                USART_RX_BUF[USART_RX_STA] = '\0';
                USART_RX_STA |= 0x8000;
                xEventGroupSetBitsFromISR(fft_events, USART_RX_COMMAND,
                                          &task_woken);
                portYIELD_FROM_ISR(task_woken);
            } else {
                // /* or DEL erase a character */
                // if ((Res == '\010') || (Res == '\177')) {
                //     if ((USART_RX_STA & 0X3FFF) == 0) {
                //         USART_SendData(USART1, '\a');
                //     } else {
                //         back_up();
                //     }
                //     /* erases a word */
                // } else {  //正常接收数据
                //     if ((USART_RX_STA & 0x00FF) < USART_REC_LEN) { //数据超长
                //         USART_RX_BUF[USART_RX_STA & 0X3FFF] = Res;
                //         USART_SendData(USART1, Res);
                //         USART_RX_STA++;
                //         if (USART_RX_STA > (USART_REC_LEN - 1))
                //             USART_RX_STA = 0;  //数据出错, 重新接收
                //     }
                // }
                if ((USART_RX_STA & 0x00FF) < USART_REC_LEN) {  //数据超长
                    USART_RX_BUF[USART_RX_STA & 0X3FFF] = Res;
                    USART_RX_STA++;
                    if (USART_RX_STA > (USART_REC_LEN - 1))
                        USART_RX_STA = 0;  //数据出错, 重新接收
                }
            }
        }
    }
}
