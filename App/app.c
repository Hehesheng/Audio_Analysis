#include "app.h"

__attribute__((section(".ccmram"))) q15_t pInputBuff[ADC_RES_SIZE] = {0};
__attribute__((section(".ccmram"))) q15_t pOutpuBuff[ADC_RES_SIZE * 2] = {0};

EventGroupHandle_t fft_events = NULL;

uint8_t debug_buff[512] = {0};

BaseType_t fftFindResult(q15_t *fft_buff, uint32_t buff_size, double *freq_res,
                         double *amp_res);

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
            fftFindResult(pInputBuff, ADC_RES_SIZE, &cache, &cache);
            printf("The freq: %lf hz\n", cache);
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

/*
 * @brief  计算频率
 * @param  计算公式:
 *         sum([begin:end]*fft_buff[begin:end])/sum(fft_buff[begin:end])
 * @arg    fft_buff: 输入数组 begin: 开始位 end: 结束位
 * @retval 计算出的频率
 */
double fftCalculateFreq(q15_t *fft_buff, uint32_t begin, uint32_t end) {
    double freq_res = 0;
    uint32_t freq_sum = 0, data_sum = 0;
    uint32_t i = 0;

    for (i = begin; i <= end; i++) {  //求出分子分母
        data_sum += fft_buff[i];
        freq_sum += i * fft_buff[i];
    }

    freq_res = ((double)freq_sum) / ((double)data_sum);

    return freq_res;
}

/*
 * @brief  寻找FFT结果中的频率和大小
 * @param  调用前请设置fft分辨率宏 FFT_RES_PIXEL
 *         函数可迭代, 每次都只返回一个结果
 *         找到有效结果返回pdTRUE, 如果没有返回pdFALSE
 * @arg    [IN]fft_buf: 输入的数组 [IN]buff_size: 数组大小
 *         [IN,OUT]frq_res: 频率计算结果 [IN,OUT]amp_res: 幅度计算结果
 * @retval 是否找到结果
 */
BaseType_t fftFindResult(q15_t *fft_buff, uint32_t buff_size, double *freq_res,
                         double *amp_res) {
    static uint32_t index;     //记录上一次离开时数组结束位置
    BaseType_t res = pdFALSE;  //输出结果
    uint32_t i, data_begin = 0, data_end = 0;
    uint8_t flag = 0;

    /*找出一个尖刺, 已经留意到的潜在bug, 暂时不修, 实现功能先
      1. 两结果靠近, 导致数据交叉 2. 边界情况, 当(buff_size - 1) > 0
     */
    for (i = index + 1; i < buff_size; i++) {
        if ((fft_buff[i] > 0) && (flag == 0)) {
            data_begin = i;
            data_end = i;
            flag = 1;
        } else if ((fft_buff[i] < 0) && (flag == 1)) {
            data_end = i - 1;
            index = i - 1;
            res = pdTRUE;
            flag = 2;
            break;
        }
    }
    index = 0;  //debug
    if (flag == 2) {
        *freq_res =
            FFT_RES_PIXEL * fftCalculateFreq(fft_buff, data_begin, data_end);
    } else {
        index = 0;
        res = pdFALSE;  //没有找到
    }
}
