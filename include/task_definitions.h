#ifndef TASK_DEFINITIONS_H
#define TASK_DEFINITIONS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

// Task function prototypes
// These will be passed to xTaskCreatePinnedToCore()

extern TaskHandle_t xHighAccelTaskHandle;
extern MPU6050 mpu;


/**
 * LED Pattern Task
 * Controls 4 LEDs with different patterns, changed by button press
 * Demonstrates: GPIO control, button debouncing, pattern state machine
 */
void LEDPatternTask(void *pvParameters);

/**
 * Brightness Control Task
 * Reads potentiometer and controls LED brightness and speaker volume
 * Demonstrates: ADC reading, PWM control, analog input processing
 */
void BrightnessControlTask(void *pvParameters);

/**
 * Morse Code Task
 * Sends morse code message with LED and buzzer
 * Demonstrates: precise timing, synchronized outputs, state machine
 */
void MorseCodeTask(void *pvParameters);

/*
 * Date Time Task
 * Prints out the date/time every 10 seconds; button press changes timezone
 * Demonstrates: button debouncing, RTC
 */
void DateTimeTask(void *pvParameters);

void vHighAccelTask(void *pvParameters);

void vLowAccelTask(void *pvParameters);

#endif // TASK_DEFINITIONS_H
