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
#include "app.h"
// Third Party
// #include "arm_math.h"
// #include "math.h"
// #include "stdio.h"

// Start_Fun
#define START_TASK_PRIO 1
#define START_STK_SIZE 128
TaskHandle_t StartTask_Handler;
void start_task(void *pvParameters);

#endif
