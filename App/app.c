#include "app.h"

__attribute__((section(".ccmram"))) q15_t pInputBuff[ADC_RES_SIZE] = {0};
__attribute__((section(".ccmram"))) q15_t pOutpuBuff[ADC_RES_SIZE * 2] = {0};
__attribute__((section(".ccmram"))) wave_info_t fft_res_save[3 * 100];

EventGroupHandle_t fft_events = NULL;

wave_info_t fft_info[100];
uint32_t freq_num = 0;

uint8_t debug_buff[512] = {0};

void calculate_task(void *pvParameters) {
    uint32_t i;
    double cache, freq_cache, amp_cache;
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
            // for (i = 0; i < ADC_RES_SIZE / 2; i++) {
            //     if (pInputBuff[i] > 0)
            //         printf("point:%4ld Res:%5d\n", i, pInputBuff[i]);
            // }
            // taskENTER_CRITICAL();
            while (fftFindResult(pInputBuff, ADC_RES_SIZE / 2, &freq_cache,
                                 &amp_cache) == pdTRUE) {
                fft_info[freq_num].amp = amp_cache;
                fft_info[freq_num].freq = freq_cache;
                freq_num++;
                printf("The freq: %lf hz\t", freq_cache);
                printf("The amp: %lf int\n", amp_cache);
            }
            // taskEXIT_CRITICAL();
            freq_num--;
            sort_fft(fft_info, freq_num + 1);
            for (i = 0; i < freq_num + 1; i++) {
                printf("AF freq: %lf hz\t", fft_info[i].freq);
                printf("AF amp: %lf int\n", fft_info[i].amp);
            }

            freq_num = 0;
        } else
            printf("FFT Error!!!\n");  // fft初始化出错

        arm_fill_q15(0, pInputBuff, ADC_RES_SIZE);
        arm_fill_q15(0, pOutpuBuff, ADC_RES_SIZE * 2);

        xEventGroupClearBits(fft_events, DATA_COPY_FINISH);  //数据可以被更换了
        xEventGroupSetBits(fft_events, FFT_CAL_FREE);  //计算完成
    }
}

/*
 * @brief  adc任务函数, 负责ADC的DMA的控制
 * @arg    None
 * @retval None
 */
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
        delay_ms(1000);
    }
}
