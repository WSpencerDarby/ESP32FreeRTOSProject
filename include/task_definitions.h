#ifndef TASK_DEFINITIONS_H
#define TASK_DEFINITIONS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "Perilib.h"
#include "madgwick_6dof.h"

// Task function prototypes
// These will be passed to xTaskCreatePinnedToCore()

extern TaskHandle_t xHighAccelTaskHandle;
extern PerilibTwiRegisterInterface_ArduinoWire mpu;

typedef union {
  uint8_t buf[14];
  struct {
    int16_t gz;
    int16_t gy;
    int16_t gx;
    int16_t temp;
    int16_t az;
    int16_t ay;
    int16_t ax;
  } __attribute__((packed));
} imu_data_t;

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
