#include "app_fun.h"

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
 * @brief  计算幅度
 * @info   计算公式
 *         sqrt(sum(pow(fft_buff[begin:end], 2)))
 * @param  fft_buff: 输入数组 begin: 开始位 end: 结束位
 * @retval 计算出幅度
 */
double fftCalculateAmp(q15_t *fft_buff, uint32_t begin, uint32_t end) {
    double sum = 0;
    uint32_t i;

    for (i = begin; i <= end; i++)
        sum += (double)fft_buff[i] * (double)fft_buff[i];

    return sqrt(sum);
}

/*
 * @brief  寻找FFT结果中的频率和大小
 * @info   调用前请设置fft分辨率宏 FFT_RES_PIXEL
 *         函数可迭代, 每次都只返回一个结果
 *         找到有效结果返回pdTRUE, 如果没有返回pdFALSE
 * @param  [IN]fft_buf: 输入的数组 [IN]buff_size: 数组大小
 *         [IN,OUT]frq_res: 频率计算结果 [IN,OUT]amp_res: 幅度计算结果
 * @retval 是否找到结果
 */
BaseType_t fftFindResult(q15_t *fft_buff, uint32_t buff_size, double *freq_res,
                         double *amp_res) {
    static uint32_t index;    //记录上一次离开时数组结束位置
    BaseType_t res = pdTRUE;  //输出结果
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
            flag = 2;
            break;
        }
    }
    for (i = data_begin + 1; i < data_end; i++) {  //查看紧凑信号
        if (fft_buff[i] < fft_buff[i - 1] && fft_buff[i] < fft_buff[i + 1]) {
            //满足比两边都小
            data_end = i;
            index = i - 1;
        }
    }

    if (flag == 2) {
        *freq_res =
            FFT_RES_PIXEL * fftCalculateFreq(fft_buff, data_begin, data_end);
        *amp_res = fftCalculateAmp(fft_buff, data_begin, data_end);
    } else {
        index = 0;
        res = pdFALSE;  //没有找到
    }

    return res;
}

/*
 * @brief  两个函数都是快排算法
 * @info   只允许调用quick_sort
 * @param  Function Param
 * @retval None
 */
static uint32_t partition(wave_info_t *array, uint32_t start, uint32_t end) {
    double pivot = array[end].amp;
    uint32_t i = start - 1, j;
    wave_info_t temp;
    for (j = start; j < end; j++) {
        if (array[j].amp <= pivot) {
            i++;
            temp = array[i];
            array[i] = array[j];
            array[j] = temp;
        }
    }
    temp = array[i + 1];
    array[i + 1] = array[end];
    array[end] = temp;

    return i + 1;
}

void quick_sort(wave_info_t *array, uint32_t start, uint32_t end) {
    if (start < end) {
        uint32_t q = partition(array, start, end);
        quick_sort(array, start, q - 1);
        quick_sort(array, q + 1, end);
    }
}

/*
 * @brief  冒泡排序
 * @param  数组和大小
 * @retval None
 */
void sort_fft(wave_info_t *buff, uint32_t size) {
    int i, j;
    wave_info_t t;

    if (size == 0 || size > 6000) return;
    //冒泡排序
    for (i = 0; i < size - 1; i++)  // n个数的数列总共扫描n-1次
    {
        for (j = 0; j < size - i - 1;
             j++) {  //每一趟扫描到a[n-i-2]与a[n-i-1]比较为止结束
            if (buff[j].watt < buff[j + 1].watt) {
                t = buff[j + 1];
                buff[j + 1] = buff[j];
                buff[j] = t;
            }
        }
    }
}

/*
 * @brief  计算电压与倍率关系
 * @info   公式:137.85*vol^4-142.19*vol^3+75.485*vol^2-7.2939*x+1.0295
 * @param  电压大小mV
 * @retval 放大倍数
 */
double voltageToMultiple(double vol) {
    double res = 0;

    vol /= 1000;
    res += 137.85 * pow(vol, 4);
    res -= 142.19 * pow(vol, 3);
    res += 75.485 * pow(vol, 2);
    res -= 7.2939 * vol;
    res += 1.0295;

    return res;
}

/*
 * @brief  计算总功率
 * @info   Function Info
 * @param  Function Param
 * @retval None
 */
void fftCalculateWatt(wave_info_t *buff, uint32_t size, uint8_t mul) {
    uint32_t i = 0;
    double arr[] = {0.79, 65.6, 43.2, 33.6, 27.2, 17.6, 10.8, 4.7, 2.2, 0.94};

    for (i = 0; i < size; i++) {
        buff[i].watt = (buff[i].amp * 3.3 / 4096 / 1) *
                       (buff[i].amp * 3.3 / 4096 / 1) /
                       (2 * INPUT_RESISTANCE) * 1000;
        // buff[i].watt /= 0.6;
    }
}

/*
 * @brief  填充记忆数组
 * @info   Function Info
 * @param  Function Param
 * @retval None
 */
void fillZeroWaveInfoBuff(wave_info_t *buff, uint32_t size) {
    uint32_t i;

    for (i = 0; i < size; i++) {
        buff[i].amp = 0;
        buff[i].freq = 0;
        buff[i].watt = 0;
    }
}

/*
 * @brief  复制数组
 * @info   Function Info
 * @param  Function Param
 * @retval None
 */
void copyWaveInfoBuff(wave_info_t *pSBuff, wave_info_t *pDBuff, uint32_t size) {
    uint32_t i;

    for (i = 0; i < size; i++) {
        pDBuff[i] = pSBuff[i];
    }
}
