#include "main.h"

__attribute__((section(".ccmram"))) q15_t pInputBuff[ADC_RES_SIZE] = {0};
__attribute__((section(".ccmram"))) q15_t pOutpuBuff[ADC_RES_SIZE * 2] = {0};

int main()
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); //设置中断
    delay_init(168);                                //delay初始化，系统主频168MHz
    uart_init(115200);                              //USART1波特率设置为115200

    Adc1_Init(); //adc初始化
    Dac1_Init(); //DAC初始化

    Adc1_DMA_Init(ADC_RES_SIZE);

    TIM2_Init(0xFFFFFFFF);

    TIM3_PWM_Init(200 - 1, 48 - 1);
    TIM_SetCompare1(TIM3, 100);

    xTaskCreate((TaskFunction_t)start_task,
                (const char *)"start_task",
                (uint16_t)START_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)START_TASK_PRIO,
                (TaskHandle_t)&StartTask_Handler);

    vTaskStartScheduler(); //任务调度开始

    return 0;
}

void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();
    //adc task create
    xTaskCreate((TaskFunction_t)adc_task,
                (const char *)"adc_task",
                (uint16_t)ADC_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)ADC_TASK_PRIO,
                (TaskHandle_t)&AdcTask_Handler);
    taskEXIT_CRITICAL();
    vTaskDelete(NULL);
}

void FFTCalculate(void)
{
    uint32_t i;
    arm_rfft_instance_q15 S;

    if (arm_rfft_init_q15(&S, ADC_RES_SIZE, 0, 1) == ARM_MATH_SUCCESS)
    {
        arm_rfft_q15(&S, pInputBuff, pOutpuBuff);
        arm_fill_q15(0, pInputBuff, ADC_RES_SIZE);
        arm_cmplx_mag_q15(pOutpuBuff, pInputBuff, ADC_RES_SIZE);

        for (i = 0; i < ADC_RES_SIZE; i++)
        {
            if (pInputBuff[i] > 1)
                printf("point:%4ld Res:%5d\n", i, pInputBuff[i]);
        }
    }
    else
        printf("FFT Error!!!\n");

    arm_fill_q15(0, pInputBuff, ADC_RES_SIZE);
    arm_fill_q15(0, pOutpuBuff, ADC_RES_SIZE * 2);
}

void adc_task(void *pvParameters)
{
    TIM2->CNT = 0;
    Adc1_DMA_Enable();
    while (1)
    {
        if (flag == 1)
        {
            flag = 0;
            TIM2->CNT = 0;
            arm_copy_q15((q15_t *)adc_res, (q15_t *)pInputBuff, ADC_RES_SIZE);
            arm_fill_q15(0, (q15_t *)adc_res, ADC_RES_SIZE);
            FFTCalculate();
            printf("Fill Take: %ld\n", TIM2->CNT);
            printf("TIM: %ld\n", count);
        }
        if (USART_RX_STA & USART_RX_OK)
        {
            USART_RX_STA = 0;
            Adc1_DMA_Init(ADC_RES_SIZE);
            TIM2->CNT = 0;
            Adc1_DMA_Enable();
        }
        delay_ms(100);
    }
}
