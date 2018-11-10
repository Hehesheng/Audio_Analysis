#ifndef __MAIN_H
#define __MAIN_H
// SYSTEM
#include "delay.h"
#include "sys.h"
#include "usart.h"
// #include "usmart.h"
// HARDWARE
#include "adc.h"
#include "dac.h"
#include "gpio.h"
#include "pwm.h"
#include "timer.h"
// RTOS
#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
// Third Party
#include "arm_math.h"
#include "math.h"
#include "stdio.h"

// EventGroup handle
extern EventGroupHandle_t fft_events;
#define ADC_DMA_FINISH (1U << 0U)
#define DATA_COPY_FINISH (1U << 1U)
#define FFT_CAL_FREE (1U << 2U)

// Start_Fun
#define START_TASK_PRIO 1
#define START_STK_SIZE 128
TaskHandle_t StartTask_Handler;
void start_task(void *pvParameters);

// Adc_Fun
#define ADC_TASK_PRIO 3
#define ADC_STK_SIZE 256
TaskHandle_t AdcTask_Handler;
void adc_task(void *pvParameters);

// Cal_Fun
#define CAL_TASK_PRIO 8
#define CAL_STK_SIZE 512
TaskHandle_t CalculateTask_Handler;
void calculate_task(void *pvParameters);

#endif
