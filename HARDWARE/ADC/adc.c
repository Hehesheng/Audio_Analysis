#include "adc.h"

uint32_t count = 0;
uint16_t adc_res[ADC_RES_SIZE] = {0};  //转化结果

//初始化ADC
//这里我们仅以规则通道为例
//我们默认仅开启通道1
void Adc1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);  //使能GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);   //使能ADC1时钟

    //先初始化IO口
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;    //模拟输入
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;  //下拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);          //初始化

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, ENABLE);   // ADC1复位
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, DISABLE);  //复位结束

    ADC_CommonStructInit(&ADC_CommonInitStructure);  // ADC指令使用默认配置
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
    ADC_CommonInit(&ADC_CommonInitStructure);

    ADC_StructInit(&ADC_InitStructure);  // ADC指令使用默认配置
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);  //开启AD转换器
}
//获得ADC值
// ch:通道值 0~16
//返回值:转换结果
u16 Get_Adc1(u8 ch) {
    //设置指定ADC的规则组通道，一个序列，采样时间
    ADC_RegularChannelConfig(
        ADC1, ch, 1,
        ADC_SampleTime_480Cycles);  // ADC1,ADC通道,480个周期,提高采样时间可以提高精确度

    ADC_SoftwareStartConv(ADC1);  //使能指定的ADC1的软件转换启动功能

    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC))
        ;  //等待转换结束

    return ADC_GetConversionValue(ADC1);  //返回最近一次ADC1规则组的转换结果
}
//获取通道ch的转换值，取times次,然后平均
// ch:通道编号
// times:获取次数
//返回值:通道ch的times次转换结果平均值
u16 Get_Adc1_Average(u8 ch, u8 times) {
    u32 temp_val = 0;
    u8 t;
    for (t = 0; t < times; t++) {
        temp_val += Get_Adc1(ch);
        delay_ms(5);
    }
    return temp_val / times;
}

double ADC_Run(void) {
    u16 adc_bits;
    double res;
    adc_bits = Get_Adc1(ADC_Channel_5);
    res = 3.3 / 255 * adc_bits;
    return res;
}

static void Adc1_DMA_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);  //使能GPIOA时钟

    //先初始化IO口
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;    //模拟输入
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;  //下拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);          //初始化
}

static void Adc1_Mode_Init(void) {
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    Adc1_DMA_GPIO_Init();

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);  //使能ADC1时钟

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, ENABLE);   // ADC1复位
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, DISABLE);  //复位结束

    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_RegularChannelConfig(
        ADC1, ADC_Channel_5, 1,
        ADC_SampleTime_480Cycles);  // ADC1,ADC通道,480个周期,提高采样时间可以提高精确度

    ADC_Cmd(ADC1, ENABLE);  //开启AD转换器
    ADC_DMACmd(ADC1, ENABLE);
}

static void Adc1_DMA_Setting_Init(uint32_t buff_size) {
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
    DMA_DeInit(DMA2_Stream0);  //初始化

    while (DMA_GetCmdStatus(DMA2_Stream0) != DISABLE)
        ;  //等待 DMA 可配置

    DMA_InitStructure.DMA_Channel = DMA_Channel_0;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)adc_res;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = buff_size;
    DMA_InitStructure.DMA_PeripheralInc =
        DMA_PeripheralInc_Disable;  //外设非增量模式
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  //存储器增量模式
    DMA_InitStructure.DMA_PeripheralDataSize =
        DMA_PeripheralDataSize_HalfWord;  //外设数据长度
    DMA_InitStructure.DMA_MemoryDataSize =
        DMA_MemoryDataSize_HalfWord;               //存储器数据长度
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;  // 使用普通模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;  //中等优先级
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;  // FIFO 模式禁止
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;  // FIFO 阈值
    DMA_InitStructure.DMA_MemoryBurst =
        DMA_MemoryBurst_Single;  //存储器突发单次传输
    DMA_InitStructure.DMA_PeripheralBurst =
        DMA_PeripheralBurst_Single;  //外设突发单次传输

    DMA_Init(DMA2_Stream0, &DMA_InitStructure);
}

static void Adc1_DMA_NVIC_Config(void) {
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;  //抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;         //子优先级1
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  // IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);  //根据指定的参数初始化VIC寄存器

    DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);
}

void Adc1_DMA_Init(uint16_t buff_size) {
    Adc1_Mode_Init();
    Adc1_DMA_Setting_Init(buff_size);
    Adc1_DMA_NVIC_Config();
}

void Adc1_DMA_Enable(void) {
    DMA_Cmd(DMA2_Stream0, DISABLE);  //关闭 DMA 传输

    while (DMA_GetCmdStatus(DMA2_Stream0) != DISABLE)  //确保 DMA 可以被设置
        ;
    DMA_SetCurrDataCounter(DMA2_Stream0, ADC_RES_SIZE);  //数据传输量
    ADC_SoftwareStartConv(ADC1);  //使能指定的ADC1的软件转换启动功能
    DMA_Cmd(DMA2_Stream0, ENABLE);  //开启 DMA 传输
}

void DMA2_Stream0_IRQHandler(void) {
    BaseType_t task_woken;
    count = TIM2->CNT;
    if (DMA_GetFlagStatus(DMA2_Stream0, DMA_IT_TCIF0) != RESET) {
        while (xEventGroupSetBitsFromISR(fft_events, ADC_DMA_FINISH,
                                         &task_woken) == pdFALSE)
            ;
        DMA_ClearFlag(DMA2_Stream0, DMA_IT_TCIF0);
    }
    if (task_woken == pdTRUE) {
        taskYIELD();  //任务切换
    }
}
