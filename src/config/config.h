#ifndef CONFIG_H
#define CONFIG_H

// ========== Hardware Configuration ==========

// LED Pattern Control (4 LEDs)
#define LED1_PIN 12
#define LED2_PIN 13
#define LED3_PIN 14
#define LED4_PIN 27
#define BUTTON_PIN 26  // Button to change pattern

// Brightness/Volume Control
#define PWM_LED_PIN 25        // LED for brightness control
#define POTENTIOMETER_PIN 34  // Analog input (ADC1_CH6)
#define SPEAKER_PIN 32        // PWM output for speaker/buzzer

// Morse Code Control
#define MORSE_LED_PIN 33
#define MORSE_BUZZER_PIN 15

// PWM Configuration
#define PWM_FREQ 5000         // 5 KHz for LED
#define PWM_RESOLUTION 8      // 8-bit resolution (0-255)
#define PWM_LED_CHANNEL 0
#define PWM_SPEAKER_CHANNEL 1
#define SPEAKER_FREQ 1000     // 1 KHz tone for morse code

// Morse Code Timing (in milliseconds)
#define MORSE_DOT_DURATION 200    // Short beep
#define MORSE_DASH_DURATION 600   // Long beep
#define MORSE_SYMBOL_GAP 200      // Gap between dots/dashes
#define MORSE_LETTER_GAP 600      // Gap between letters
#define MORSE_WORD_GAP 1400       // Gap between words

// Ethernet Attributes
#define WIFI_SSID "aeiou" //"Spencer’s Phone"
#define WIFI_PASSWORD "!!! JesusIsMyLord !!!" //"1234ABCD"

// ========== Task Configuration ==========

// Task Priorities (higher number = higher priority)
#define LED_PATTERN_TASK_PRIORITY    2
#define BRIGHTNESS_TASK_PRIORITY     2
#define MORSE_TASK_PRIORITY          1
#define DATETIME_TASK_PRIORITY       1

// Task Stack Sizes (in bytes)
#define LED_PATTERN_TASK_STACK_SIZE  2048
#define BRIGHTNESS_TASK_STACK_SIZE   2048
#define MORSE_TASK_STACK_SIZE        2048
#define DATETIME_TASK_STACK_SIZE     2048

// Task Core Assignment (ESP32 has cores 0 and 1)
#define LED_PATTERN_TASK_CORE        0  // Button handling on core 0
#define BRIGHTNESS_TASK_CORE         1  // Analog reading on core 1
#define MORSE_TASK_CORE              0  // Timing-critical on core 0
#define DATETIME_TASK_CORE           1  // date/time on core 1

// Task Periods (in milliseconds)
#define LED_PATTERN_PERIOD_MS        10    // Check button frequently
#define BRIGHTNESS_PERIOD_MS         50    // Read potentiometer every 50ms
#define MORSE_PERIOD_MS              100   // Check morse state
#define DATETIME_PERIOD_MS           10000   // Print date/time every 10s

#endif // CONFIG_H
