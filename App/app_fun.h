#ifndef __APPFUN_H
#define __APPFUN_H

#include "FreeRTOS.h"
#include "arm_math.h"
#include "stdlib.h"
#include "math.h"

#define FFT_RES_PIXEL 5.21

struct wave_info {
    double freq;
    double amp;
};
#define wave_info_t struct wave_info

double fftCalculateFreq(q15_t *fft_buff, uint32_t begin, uint32_t end);
double fftCalculateAmp(q15_t *fft_buff, uint32_t begin, uint32_t end);
BaseType_t fftFindResult(q15_t *fft_buff, uint32_t buff_size, double *freq_res,
                         double *amp_res);
void quick_sort(wave_info_t *array, uint32_t start, uint32_t end);
double voltageToMultiple(double vol);
void sort_fft(wave_info_t *buff, uint32_t size);

#endif  // __APPFUN_H