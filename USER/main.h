#ifndef __MAIN_H
#define __MAIN_H
// SYSTEM
#include "sys.h"
#include "delay.h"
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
#include "task.h"
#include "event_groups.h"
// Third Party
#include "arm_math.h"

//Start_Fun
#define START_TASK_PRIO		1
#define START_STK_SIZE 		128
TaskHandle_t StartTask_Handler;
void start_task(void *pvParameters);

//Adc_Fun
#define ADC_TASK_PRIO       3
#define ADC_STK_SIZE        128
TaskHandle_t AdcTask_Handler;
void adc_task(void *pvParameters);

#endif
