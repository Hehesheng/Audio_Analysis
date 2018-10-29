#include "main.h"

int main()
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); //设置中断
    delay_init(168);                                //delay初始化，系统主频168MHz
    uart_init(115200);                              //USART1波特率设置为115200

    Adc1_Init(); //adc初始化
    Dac1_Init(); //DAC初始化
    // GPIO_ALLInit();

    TIM3_PWM_Init(2000 - 1, 48000 - 1);
    TIM_SetCompare1(TIM3, 1000); // 比较值CCR1为500

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
    //led task create
    xTaskCreate((TaskFunction_t)led_task,
                (const char *)"led_task",
                (uint16_t)LED_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)LED_TASK_PRIO,
                (TaskHandle_t)&LedTask_Handler);
    taskEXIT_CRITICAL();
    vTaskDelete(NULL);
}

void led_task(void *pvParameters)
{
    while (1)
    {
        GPIO_SetBits(GPIOA, GPIO_Pin_7);
        delay_ms(500);
        GPIO_ResetBits(GPIOA, GPIO_Pin_7);
        delay_ms(500);
    }
}
