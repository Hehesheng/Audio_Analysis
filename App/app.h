#ifndef __APP_H
#define __APP_H

// SYSTEM
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "app_fun.h"
// #include "usmart.h"
// HARDWARE
#include "adc.h"
#include "dac.h"
#include "gpio.h"
#include "timer.h"
// RTOS
#include "FreeRTOS.h"
#include "event_groups.h"
// Third Party
#include "arm_math.h"
#include "math.h"
#include "stdio.h"

// EventGroup handle
extern EventGroupHandle_t fft_events;
#define ADC_DMA_FINISH (1U << 0U)
#define DATA_COPY_FINISH (1U << 1U)
#define FFT_CAL_FREE (1U << 2U)
#define FFT_INFO_COPY (1U << 3U)
#define DATA_SHOULD_SAVE (1U << 4U)
#define USART_RX_COMMAND 1U << 5U

// Adc_Fun
#define ADC_TASK_PRIO 8
#define ADC_STK_SIZE 256
TaskHandle_t AdcTask_Handler;
void adc_task(void *pvParameters);

// Cal_Fun
#define CAL_TASK_PRIO 7
#define CAL_STK_SIZE 1024
TaskHandle_t CalculateTask_Handler;
void calculate_task(void *pvParameters);

// ui_Fun
#define UI_TASK_PRIO 9
#define UI_STK_SIZE 512
TaskHandle_t UITask_Handler;
void ui_task(void *pvParameters);

#endif	// __TASKS_H
