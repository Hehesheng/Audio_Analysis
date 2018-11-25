#include "app.h"

__attribute__((section(".ccmram"))) q15_t pInputBuff[ADC_RES_SIZE] = {0};
__attribute__((section(".ccmram"))) q15_t pOutpuBuff[ADC_RES_SIZE * 2] = {0};
__attribute__((section(".ccmram"))) wave_info_t fft_res_save[5 * 100];

EventGroupHandle_t fft_events = NULL;

wave_info_t fft_info[100];
uint32_t freq_num = 0;
uint8_t save_place = 0;

// uint8_t debug_buff[512] = {0};

void calculate_task(void *pvParameters) {
    uint32_t i;
    double cache, freq_cache, amp_cache;
    arm_rfft_instance_q15 S;

    uint8_t data_ok = 0;
    q15_t data_max = 0;
    uint16_t Vol_Select = 0;

    while (1) {
        xEventGroupWaitBits(fft_events, DATA_COPY_FINISH, pdFALSE, pdTRUE,
                            portMAX_DELAY);  //等待数据拷贝完成
        if (arm_rfft_init_q15(&S, ADC_RES_SIZE, 0, 1) == ARM_MATH_SUCCESS) {
            xEventGroupClearBits(fft_events, FFT_CAL_FREE);  //数据OK, 开始计算

            for (i = 0; i < ADC_RES_SIZE; i++) {  //找最大值
                if (pInputBuff[i] > data_max) data_max = pInputBuff[i];
            }
            //分段
            //算的太烦,先放着
            data_ok = 1;
            if (data_max < Vol_Select == 0) {
                Vol_Select = 50;
                Dac2_Set_Vol(Vol_Select);
            }

            if (data_ok == 1) {  //数据OK才进行计算
                arm_rfft_q15(&S, pInputBuff, pOutpuBuff);
                arm_fill_q15(0, pInputBuff, ADC_RES_SIZE);

                for (i = 0; i < ADC_RES_SIZE; i++) {  //计算实际值
                    cache = pow((double)pOutpuBuff[2 * i], 2);
                    cache += pow((double)pOutpuBuff[2 * i + 1], 2);
                    pInputBuff[i] = 2 * (q15_t)sqrt(cache);
                }
                arm_offset_q15(pInputBuff, -32, pInputBuff,
                               ADC_RES_SIZE);  //误差偏移

                while (fftFindResult(pInputBuff, ADC_RES_SIZE / 2, &freq_cache,
                                     &amp_cache) == pdTRUE) {
                    fft_info[freq_num].amp = amp_cache;
                    fft_info[freq_num].freq = freq_cache;
                    freq_num++;
                    // printf("The freq: %lf hz\t", freq_cache);
                    // printf("The amp: %lf int\n", amp_cache);
                }

                freq_num--;
                fftCalculateWatt(fft_info, freq_num + 1, 1);  //计算功率
                sort_fft(fft_info, freq_num + 1);
                // for (i = 0; i < freq_num + 1; i++) {
                //     printf("AF freq: %lf hz\t", fft_info[i].freq);
                //     printf("AF watt: %lf mw\n", fft_info[i].watt);
                // }
                if (xEventGroupGetBits(fft_events) & DATA_SHOULD_SAVE) {
                    fillZeroWaveInfoBuff(fft_res_save + save_place * 100, 100);
                    copyWaveInfoBuff(fft_info, fft_res_save + save_place * 100,
                                     freq_num + 1);
                    xEventGroupClearBits(fft_events, DATA_SHOULD_SAVE);
                    xEventGroupSetBits(fft_events, FFT_INFO_COPY);
                }
            }
            data_ok = 0;
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
    Adc1_DMA_Enable();

    while (1) {
        xEventGroupWaitBits(fft_events, ADC_DMA_FINISH | FFT_CAL_FREE, pdFALSE,
                            pdTRUE, portMAX_DELAY);  //等待DMA传送, 以及FFT空闲
        arm_copy_q15((q15_t *)adc_res, (q15_t *)pInputBuff, ADC_RES_SIZE);
        arm_fill_q15(0, (q15_t *)adc_res, ADC_RES_SIZE);
        xEventGroupSetBits(fft_events, DATA_COPY_FINISH);  //数据拷贝完成

        xEventGroupClearBits(fft_events, ADC_DMA_FINISH);  // DMA开始
        Adc1_DMA_Init(ADC_RES_SIZE);

        Adc1_DMA_Enable();

        // vTaskList((char *)debug_buff);
        // printf("%s\n", debug_buff);
    }
}

/*
 * @brief  ui界面变化
 * @info   Function Info
 * @param  Function Param
 * @retval None
 */
void updateUI(void) {
    uint32_t i, index;

    for (i = 0; i < 100; i++) {
        index = i + 100 * (save_place);
        if ((fft_res_save[index].freq - 0) < 1E-1) {
            break;
        }
        printf("%d.%d:Freq: %lf hz\t", save_place, index,
               fft_res_save[index].freq);
        printf("Watt: %lf mw\n", fft_res_save[index].watt);
    }
    save_place++;
    if (save_place >= 5) {
        save_place = 0;
    }
}

/*
 * @brief  界面控制, 负责刷新
 * @info   Function Info
 * @param  Function Param
 * @retval None
 */
void ui_task(void *pvParameters) {
    fillZeroWaveInfoBuff(fft_res_save, 5 * 100);

    while (1) {
        xEventGroupSetBits(fft_events, DATA_SHOULD_SAVE);
        xEventGroupWaitBits(fft_events, FFT_INFO_COPY, pdTRUE, pdFALSE,
                            portMAX_DELAY);
        updateUI();
        delay_ms(2000);
    }
}
