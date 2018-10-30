#include "main.h"

int main()
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); //设置中断
    delay_init(168);                                //delay初始化，系统主频168MHz
    uart_init(115200);                              //USART1波特率设置为115200

    Adc1_Init(); //adc初始化
    Dac1_Init(); //DAC初始化

    Adc1_DMA_Init(ADC_RES_SIZE);

    TIM2_Init(0xFFFFFFFF);

    TIM3_PWM_Init(200 - 1, 480 - 1);
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

void adc_task(void *pvParameters)
{
    TIM2->CNT = 0;
    Adc1_DMA_Enable();
    while (1)
    {
        if (flag == 1)
        {
            flag = 0;
            for (uint32_t i = 0; i < ADC_RES_SIZE; i++)
            {
                printf("%4d,", adc_res[i]);
                if ((i + 1) % 16 == 0)
                {
                    printf("\n");
                }
            }
            arm_fill_q15(0, adc_res, ADC_RES_SIZE);
            Adc1_DMA_Init(ADC_RES_SIZE);
            printf("TIM: %ld\n", count);
            TIM2->CNT = 0;
            Adc1_DMA_Enable();
        }
        delay_ms(100);
    }
}
