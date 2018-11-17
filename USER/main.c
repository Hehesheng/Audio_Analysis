#include "main.h"

int main() {
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);  //设置中断
    delay_init(168);    // delay初始化，系统主频168MHz
    uart_init(115200);  // USART1波特率设置为115200

    Adc1_Init();  // adc初始化ç
    Dac1_Init();  // DAC初始化

    Adc1_DMA_Init(ADC_RES_SIZE);

    TIM2_Init(0xFFFFFFFF);

    TIM3_PWM_Init(100 - 1, 840 - 1);
    TIM_SetCompare1(TIM3, 50 - 1);

    xTaskCreate((TaskFunction_t)start_task, (const char *)"start_task",
                (uint16_t)START_STK_SIZE, (void *)NULL,
                (UBaseType_t)START_TASK_PRIO, (TaskHandle_t)&StartTask_Handler);

    vTaskStartScheduler();  //任务调度开始

    return 0;
}

void start_task(void *pvParameters) {
    taskENTER_CRITICAL();
    // other create
    fft_events = xEventGroupCreate();
    xEventGroupSetBits(fft_events, FFT_CAL_FREE);  //标志fft计算空闲
    // adc task create
    xTaskCreate((TaskFunction_t)adc_task, (const char *)"adc_task",
                (uint16_t)ADC_STK_SIZE, (void *)NULL,
                (UBaseType_t)ADC_TASK_PRIO, (TaskHandle_t)&AdcTask_Handler);
    // cal task create
    xTaskCreate((TaskHandle_t)calculate_task, (const char *)"cal_task",
                (uint16_t)CAL_STK_SIZE, (void *)NULL,
                (UBaseType_t)CAL_TASK_PRIO,
                (TaskHandle_t)&CalculateTask_Handler);
    taskEXIT_CRITICAL();
    vTaskDelete(NULL);
}
