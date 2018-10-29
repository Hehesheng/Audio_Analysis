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

//Led_Fun
#define LED_TASK_PRIO       10
#define LED_STK_SIZE        64
TaskHandle_t LedTask_Handler;
void led_task(void *pvParameters);

#endif
