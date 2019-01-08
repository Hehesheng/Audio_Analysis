#include "app.h"

#define HMI_COMMAND_END printf("\xff\xff\xff")

__attribute__((section(".ccmram"))) q15_t pInputBuff[ADC_RES_SIZE] = {0};
__attribute__((section(".ccmram"))) q15_t pOutpuBuff[ADC_RES_SIZE * 2] = {0};
__attribute__((section(".ccmram"))) wave_info_t fft_res_save[5 * 100];

EventGroupHandle_t fft_events = NULL;

wave_info_t fft_info[100];
uint32_t freq_num = 0;
uint8_t save_place = 0;
uint8_t command = 0;

// uint8_t debug_buff[512] = {0};

void calculate_task(void *pvParameters) {
    uint32_t i;
    double cache, freq_cache, amp_cache, sign_amp = 0;
    arm_rfft_instance_q15 S;

    volatile uint8_t data_ok = 0, data_need_skip = 0, mul_num = 0;
    q15_t data_max = 0;
    uint16_t Vol_Select = 0;

    while (1) {
        xEventGroupWaitBits(fft_events, DATA_COPY_FINISH, pdFALSE, pdTRUE,
                            portMAX_DELAY);  //等待数据拷贝完成
        if (arm_rfft_init_q15(&S, ADC_RES_SIZE, 0, 1) == ARM_MATH_SUCCESS) {
            xEventGroupClearBits(fft_events, FFT_CAL_FREE);  //数据OK, 开始计算

            data_max = 0;
            for (i = 0; i < ADC_RES_SIZE; i++) {  //找最大值
                if (pInputBuff[i] > data_max) data_max = pInputBuff[i];
            }
            //分段 直流1.580v
            sign_amp = (double)abs(data_max - 1.62 / 3.3 * 4096);
            sign_amp = sign_amp * 4177.1215 / 4096;  //信号幅度, 单位mV
            sign_amp /= 0.8;
            if (data_need_skip == 0) {
                if (sign_amp < 20) {  // 65.6倍
                    Vol_Select = 1000;
                    Dac1_Set_Vol(Vol_Select);
                    mul_num = 1;
                } else if (sign_amp < 35) {  // 43.2倍
                    Vol_Select = 900;
                    Dac1_Set_Vol(Vol_Select);
                    mul_num = 2;
                } else if (sign_amp < 45) {  // 33.6倍
                    Vol_Select = 850;
                    Dac1_Set_Vol(Vol_Select);
                    mul_num = 3;
                } else if (sign_amp < 55) {  // 27.2倍
                    Vol_Select = 800;
                    Dac1_Set_Vol(Vol_Select);
                    mul_num = 4;
                } else if (sign_amp < 90) {  // 17.6倍
                    Vol_Select = 700;
                    Dac1_Set_Vol(Vol_Select);
                    mul_num = 5;
                } else if (sign_amp < 140) {  // 10.8
                    Vol_Select = 600;
                    Dac1_Set_Vol(Vol_Select);
                    mul_num = 6;
                } else if (sign_amp < 320) {  // 4.7倍
                    Vol_Select = 400;
                    Dac1_Set_Vol(Vol_Select);
                    mul_num = 7;
                } else if (sign_amp < 700) {  // 2.2倍
                    Vol_Select = 250;
                    Dac1_Set_Vol(Vol_Select);
                    mul_num = 8;
                } else if (sign_amp < 1250) {  // 0.94倍
                    Vol_Select = 50;
                    Dac1_Set_Vol(Vol_Select);
                    mul_num = 9;
                } else {  //不用放大
                    data_ok = 1;
                    mul_num = 0;
                }
                data_need_skip = 1;
            }

            if (data_ok == 1) {  //数据OK才进行计算
                data_need_skip = 0;
                Dac1_Set_Vol(0);
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
                }

                freq_num--;

                fftCalculateWatt(fft_info, freq_num + 1,
                                 mul_num);  //计算功率
                //计算功率, 加上前置
                // fftCalculateWatt(
                //     fft_info, freq_num + 1,
                //     voltageToMultiple(Vol_Select));
                sort_fft(fft_info, freq_num + 1);

                if (xEventGroupGetBits(fft_events) & DATA_SHOULD_SAVE) {
                    fillZeroWaveInfoBuff(fft_res_save + save_place * 100, 100);
                    copyWaveInfoBuff(fft_info, fft_res_save + save_place * 100,
                                     freq_num + 1);
                    xEventGroupClearBits(fft_events, DATA_SHOULD_SAVE);
                    xEventGroupSetBits(fft_events, FFT_INFO_COPY);
                }
                Vol_Select = 0;
                mul_num = 0;
            }
            data_ok = 0;
            if (data_need_skip == 1) {
                data_ok = 1;
            }
            freq_num = 0;
        } else
            printf("FFT Error!!!\n");  // fft初始化出错

        arm_fill_q15(0, pInputBuff, ADC_RES_SIZE);
        arm_fill_q15(0, pOutpuBuff, ADC_RES_SIZE * 2);

        xEventGroupClearBits(fft_events,
                             DATA_COPY_FINISH);  //数据可以被更换了
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
                            pdTRUE,
                            portMAX_DELAY);  //等待DMA传送, 以及FFT空闲
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
    uint32_t i, index, index_res;
    double all_watt = 0;

    // for (i = 0; i < 100; i++) {
    //     index = i + 100 * (save_place);
    //     if ((fft_res_save[index].freq - 0) < 1E-1) {
    //         break;
    //     }
    //     printf("%d.%d:Freq: %lf hz\t", save_place, index,
    //            fft_res_save[index].freq);
    //     printf("Watt: %lf mw\n", fft_res_save[index].watt);
    // }

    // if (freq_num < 8) {
    //     index = freq_num;
    // } else {
    //     index = 8;
    // }
    index_res = 100 * (save_place);
    printf("t_zq.txt=\"%lf ms\"", 1 / fft_res_save[index_res].freq * 1000);
    HMI_COMMAND_END;
    for (i = 0; i < 8; i++) {
        printf("t%d.txt=\"%.2lf hz    %.3lf mW\"", i,
               fft_res_save[index_res].freq, fft_res_save[index_res].watt);
        HMI_COMMAND_END;
        all_watt += fft_res_save[index_res++].watt;
    }
    printf("t_all.txt=\"%lf mW\"", all_watt);
    HMI_COMMAND_END;
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
    uint8_t i;
    fillZeroWaveInfoBuff(fft_res_save, 5 * 100);

    while (1) {
        for (i = 0; i < 4; i++) {
            xEventGroupSetBits(fft_events, DATA_SHOULD_SAVE);
            xEventGroupWaitBits(fft_events, FFT_INFO_COPY, pdTRUE, pdFALSE,
                                portMAX_DELAY);

            if (USART_RX_STA & USART_RX_OK) {
                switch (USART_RX_BUF[0]) {
                    case 1:
                    updateUI();
                    break;
                    case 2:
                    updateUI();
                    break;
                    case 4:
                    // command = 1;
                    if (USART_RX_BUF[1] == 1) {
                        command = 1;
                    } else if (USART_RX_BUF[1] == 0) {
                        command = 0;
                    }
                    break;
                }
                // if (USART_RX_BUF[0] == 1 && command == 1) {
                //     updateUI();
                // } else if (USART_RX_BUF[0] == 2 && command == 1) {
                //     updateUI();
                // } else if (USART_RX_BUF[0] == 4) {
                //     if (USART_RX_BUF[1] == 1) {
                //         command == 1;
                //     } else if (USART_RX_BUF[1] == 0) {
                //         command == 0;
                //     }
                // }
                xEventGroupClearBits(fft_events, USART_RX_COMMAND);
                USART_RX_STA = 0;
            }
            if (command == 0) {
                updateUI();
            }
            delay_ms(1000);
        }
    }
}
