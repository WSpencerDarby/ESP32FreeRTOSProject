#ifndef CONFIG_H
#define CONFIG_H

// ========== Hardware Configuration ==========

// LED Pattern Control (4 LEDs)
#define LED1_PIN 12
#define LED2_PIN 13
#define LED3_PIN 15
#define LED4_PIN 27
#define BUTTON_PIN 26  // Button to change pattern

// Brightness/Volume Control
#define PWM_LED_PIN 25        // LED for brightness control
#define POTENTIOMETER_PIN 34  // Analog input (ADC1_CH6)
#define SPEAKER_PIN 32        // PWM output for speaker/buzzer

// Morse Code Control
#define MORSE_LED_PIN 33
#define MORSE_BUZZER_PIN 14

// PWM Configuration
#define PWM_FREQ 5000         // 5 KHz for LED
#define PWM_RESOLUTION 8      // 8-bit resolution (0-255)
#define SPEAKER_FREQ 1000     // 1 KHz tone for morse code

// Morse Code Timing (in milliseconds)
#define MORSE_DOT_DURATION 200    // Short beep
#define MORSE_DASH_DURATION 600   // Long beep
#define MORSE_SYMBOL_GAP 200      // Gap between dots/dashes
#define MORSE_LETTER_GAP 600      // Gap between letters
#define MORSE_WORD_GAP 1400       // Gap between words

// I2C Pins
#define SCL_PIN 20
#define SDA_PIN 22

#define LOG_MESSAGE_MAX_LENGTH 128
#define LOG_UDP_TARGET_HOST "255.255.255.255"
#define LOG_UDP_TARGET_PORT 4210
#define LOG_UDP_LOCAL_PORT 4211
#define TASK_STATS_LOG_PERIOD_MS 5000
#define TASK_STATS_LOG_TASK_PRIORITY 1
#define TASK_STATS_LOG_TASK_STACK_SIZE 3072
#define TASK_STATS_LOG_TASK_CORE 1
#define BRIGHTNESS_LOG_PERIOD_MS 30000

// Ethernet Attributes
#define WIFI_SSID "ENSandCo"
#define WIFI_PASSWORD "elyseandspencer"

// ========== Task Configuration ==========

// Task Priorities (higher number = higher priority)
#define LED_PATTERN_TASK_PRIORITY    2
#define BRIGHTNESS_TASK_PRIORITY     2
#define MORSE_TASK_PRIORITY          1
#define DATETIME_TASK_PRIORITY       1
#define LOW_ACCEL_TASK_PRIORITY      1
#define HIGH_ACCEL_TASK_PRIORITY     3

// Task Stack Sizes (in bytes)
#define LED_PATTERN_TASK_STACK_SIZE  3072
#define BRIGHTNESS_TASK_STACK_SIZE   3072
#define MORSE_TASK_STACK_SIZE        4096
#define DATETIME_TASK_STACK_SIZE     4096
#define LOW_ACCEL_STACK_SIZE         4096
#define HIGH_ACCEL_STACK_SIZE        4096

// Task Core Assignment (ESP32 has cores 0 and 1) I think only thingspeak should be on core 1.
#define LED_PATTERN_TASK_CORE        0  // Button handling on core 0
#define BRIGHTNESS_TASK_CORE         0  // Analog reading on core 0
#define MORSE_TASK_CORE              0  // Timing-critical on core 0
#define DATETIME_TASK_CORE           0  // date/time on core 0
#define LOW_ACCEL_TASK_CORE          0  // low accel on core 0
#define HIGH_ACCEL_TASK_CORE         0  // high accel on core 0

// Task Periods (in milliseconds)
#define LED_PATTERN_PERIOD_MS        10    // Check button frequently
#define BRIGHTNESS_PERIOD_MS         50    // Read potentiometer every 50ms
#define MORSE_PERIOD_MS              100   // Check morse state
#define DATETIME_PERIOD_MS           10000   // Print date/time every 10s
#define LOW_ACCEL_PERIOD_MS          500   // Check accelerometer raw values every 500ms
#define HIGH_ACCEL_PERIOD_MS         50    // Check accelerometer raw value every 50 ms

#define MPU6050_DEVADDR_DEFAULT         0x68
#define MPU6050_REGADDR_ACCEL_XOUT_H    0x3B
#define MPU6050_REGADDR_PWR_MGMT_1      0x6B
#define MPU6060_REGADDR_WHO_AM_I        0x75

#endif // CONFIG_H
