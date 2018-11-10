#include "main.h"

__attribute__((section(".ccmram"))) q15_t pInputBuff[ADC_RES_SIZE] = {0};
__attribute__((section(".ccmram"))) q15_t pOutpuBuff[ADC_RES_SIZE * 2] = {0};

EventGroupHandle_t fft_events = NULL;

uint8_t debug_buff[512] = {0};

int main() {
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);  //设置中断
    delay_init(168);    // delay初始化，系统主频168MHz
    uart_init(115200);  // USART1波特率设置为115200

    Adc1_Init();  // adc初始化ç
    Dac1_Init();  // DAC初始化

    Adc1_DMA_Init(ADC_RES_SIZE);

    TIM2_Init(0xFFFFFFFF);

    TIM3_PWM_Init(100 - 1, 840 - 1);
    TIM_SetCompare1(TIM3, 50);

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

void calculate_task(void *pvParameters) {
    uint32_t i;
    double cache;
    arm_rfft_instance_q15 S;
    while (1) {
        xEventGroupWaitBits(fft_events, DATA_COPY_FINISH, pdFALSE, pdTRUE,
                            portMAX_DELAY);  //等待数据拷贝完成
        if (arm_rfft_init_q15(&S, ADC_RES_SIZE, 0, 1) == ARM_MATH_SUCCESS) {
            xEventGroupClearBits(fft_events, FFT_CAL_FREE);  //数据OK, 开始计算

            arm_rfft_q15(&S, pInputBuff, pOutpuBuff);
            arm_fill_q15(0, pInputBuff, ADC_RES_SIZE);

            for (i = 0; i < ADC_RES_SIZE; i++) {  //计算实际值
                cache = pow((double)pOutpuBuff[2 * i], 2);
                cache += pow((double)pOutpuBuff[2 * i + 1], 2);
                pInputBuff[i] = (q15_t)sqrt(cache);
            }
            arm_offset_q15(pInputBuff, -16, pInputBuff,
                           ADC_RES_SIZE);  //误差偏移
            for (i = 0; i < ADC_RES_SIZE / 2; i++) {
                if (pInputBuff[i] > 0)
                    printf("point:%4ld Res:%5d\n", i, pInputBuff[i]);
            }
        } else
            printf("FFT Error!!!\n");  // fft初始化出错

        arm_fill_q15(0, pInputBuff, ADC_RES_SIZE);
        arm_fill_q15(0, pOutpuBuff, ADC_RES_SIZE * 2);

        xEventGroupClearBits(fft_events, DATA_COPY_FINISH);  //数据可以被更换了
        xEventGroupSetBits(fft_events, FFT_CAL_FREE);  //计算完成
    }
}

void adc_task(void *pvParameters) {
    TIM2->CNT = 0;

    Adc1_DMA_Enable();

    while (1) {
        xEventGroupWaitBits(fft_events, ADC_DMA_FINISH | FFT_CAL_FREE, pdFALSE,
                            pdTRUE, portMAX_DELAY);  //等待DMA传送, 以及FFT空闲
        TIM2->CNT = 0;
        arm_copy_q15((q15_t *)adc_res, (q15_t *)pInputBuff, ADC_RES_SIZE);
        arm_fill_q15(0, (q15_t *)adc_res, ADC_RES_SIZE);
        xEventGroupSetBits(fft_events, DATA_COPY_FINISH);  //数据拷贝完成

        xEventGroupClearBits(fft_events, ADC_DMA_FINISH);  // DMA开始
        Adc1_DMA_Init(ADC_RES_SIZE);
        TIM2->CNT = 0;
        Adc1_DMA_Enable();

        vTaskList((char *)debug_buff);
        printf("%s\n", debug_buff);
        delay_ms(2000);
    }
}
