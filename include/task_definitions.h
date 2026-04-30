#ifndef TASK_DEFINITIONS_H
#define TASK_DEFINITIONS_H

// Task function prototypes
// These will be passed to xTaskCreatePinnedToCore()

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

#endif // TASK_DEFINITIONS_H
