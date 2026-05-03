#include <Arduino.h>
#include "task_definitions.h"
#include "../config/config.h"
#include "time.h"
#include <string>

// Global variable for button state (shared between tasks if needed)
static volatile int currentPattern = 0;

uint8_t fifoBuffer[64]; // FIFO storage buffer


Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector

TaskHandle_t xHighAccelTaskHandle = NULL;

/**
 * LED Pattern Task
 * 
 * Controls 4 LEDs with different patterns:
 * Pattern 0: All off
 * Pattern 1: Sequential (one at a time)
 * Pattern 2: Alternating (1,3 then 2,4)
 * Pattern 3: Wave (left to right)
 * Pattern 4: All blinking together
 * 
 * Button press cycles through patterns
 */
void LEDPatternTask(void *pvParameters) {
    // Initialize pins
    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    pinMode(LED3_PIN, OUTPUT);
    pinMode(LED4_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(LED_PATTERN_PERIOD_MS);
    
    int lastButtonState = HIGH;
    int patternStep = 0;
    unsigned long lastPatternUpdate = 0;
    const int PATTERN_DELAY = 200;  // ms between pattern steps
    
    while (1) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        unsigned long startTime = micros();
        
        // Check button (with simple debouncing)
        int buttonState = digitalRead(BUTTON_PIN);
        if (buttonState == LOW && lastButtonState == HIGH) {
            currentPattern = (currentPattern + 1) % 5;
            patternStep = 0;
            Serial.printf("[LED_PATTERN] Changed to pattern %d\n", currentPattern);
            vTaskDelay(pdMS_TO_TICKS(50));  // Simple debounce
        }
        lastButtonState = buttonState;
        
        // Update pattern based on time
        if (millis() - lastPatternUpdate >= PATTERN_DELAY) {
            lastPatternUpdate = millis();
            
            switch (currentPattern) {
                case 0:  // All off
                    digitalWrite(LED1_PIN, LOW);
                    digitalWrite(LED2_PIN, LOW);
                    digitalWrite(LED3_PIN, LOW);
                    digitalWrite(LED4_PIN, LOW);
                    break;
                    
                case 1:  // Sequential (one at a time)
                    digitalWrite(LED1_PIN, patternStep == 0 ? HIGH : LOW);
                    digitalWrite(LED2_PIN, patternStep == 1 ? HIGH : LOW);
                    digitalWrite(LED3_PIN, patternStep == 2 ? HIGH : LOW);
                    digitalWrite(LED4_PIN, patternStep == 3 ? HIGH : LOW);
                    patternStep = (patternStep + 1) % 4;
                    break;
                    
                case 2:  // Alternating (1,3 then 2,4)
                    if (patternStep % 2 == 0) {
                        digitalWrite(LED1_PIN, HIGH);
                        digitalWrite(LED2_PIN, LOW);
                        digitalWrite(LED3_PIN, HIGH);
                        digitalWrite(LED4_PIN, LOW);
                    } else {
                        digitalWrite(LED1_PIN, LOW);
                        digitalWrite(LED2_PIN, HIGH);
                        digitalWrite(LED3_PIN, LOW);
                        digitalWrite(LED4_PIN, HIGH);
                    }
                    patternStep++;
                    break;
                    
                case 3: {  // Wave (left to right, then right to left)
                    int pos = patternStep % 6;
                    digitalWrite(LED1_PIN, (pos == 0 || pos == 5) ? HIGH : LOW);
                    digitalWrite(LED2_PIN, (pos == 1 || pos == 4) ? HIGH : LOW);
                    digitalWrite(LED3_PIN, (pos == 2 || pos == 3) ? HIGH : LOW);
                    digitalWrite(LED4_PIN, pos == 3 ? HIGH : LOW);
                    patternStep++;
                    break;
                }
                    
                case 4: {  // All blinking together
                    bool state = (patternStep % 2 == 0);
                    digitalWrite(LED1_PIN, state);
                    digitalWrite(LED2_PIN, state);
                    digitalWrite(LED3_PIN, state);
                    digitalWrite(LED4_PIN, state);
                    patternStep++;
                    break;
                }
            }
        }
        
        unsigned long execTime = micros() - startTime;
        // Collect timing data for performance analysis
    }
}

/**
 * Brightness Control Task
 * 
 * Reads potentiometer value and controls:
 * - LED brightness via PWM
 * - Speaker volume via PWM duty cycle
 */
void BrightnessControlTask(void *pvParameters) {
    // Setup PWM for LED
    ledcAttach(PWM_LED_PIN, PWM_FREQ,PWM_RESOLUTION);
    
    
    // Setup PWM for speaker
    ledcAttach(SPEAKER_PIN, SPEAKER_FREQ, PWM_RESOLUTION);
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(BRIGHTNESS_PERIOD_MS);
    
    while (1) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        unsigned long startTime = micros();
        
        // Read potentiometer (0-4095 for 12-bit ADC)
        int potValue = analogRead(POTENTIOMETER_PIN);
        
        // Map to 8-bit PWM (0-255)
        int brightness = map(potValue, 0, 4095, 0, 255);
        
        // Set LED brightness
        ledcWrite(PWM_LED_CHANNEL, brightness);
        
        // Set speaker volume (same value)
        ledcWrite(PWM_SPEAKER_CHANNEL, brightness);
        
        unsigned long execTime = micros() - startTime;
        
        // Print every 1 second to avoid flooding serial
        static unsigned long lastPrint = 0;
        if (millis() - lastPrint >= 1000) {
            Serial.printf("[BRIGHTNESS] Pot=%d, Brightness=%d, ExecTime=%lu us\n",
                          potValue, brightness, execTime);
            lastPrint = millis();
        }
    }
}

/**
 * Morse Code Task
 * 
 * Sends "SOS" in morse code repeatedly
 * S = ... (dot dot dot)
 * O = --- (dash dash dash)
 * S = ... (dot dot dot)
 * 
 * Controls both LED and buzzer simultaneously
 */
void MorseCodeTask(void *pvParameters) {
    pinMode(MORSE_LED_PIN, OUTPUT);
    ledcAttach(MORSE_BUZZER_PIN, SPEAKER_FREQ, PWM_RESOLUTION);
    
    // Morse code for "SOS"
    const char* message = "SOS";
    
    while (1) {
        unsigned long startTime = micros();
        
        Serial.println("[MORSE] Sending SOS...");
        
        // Send "SOS"
        for (int i = 0; i < 3; i++) {  // Repeat 3 times for demo
            
            // S = ... (3 dots)
            for (int j = 0; j < 3; j++) {
                digitalWrite(MORSE_LED_PIN, HIGH);
                ledcWrite(2, 128);  // 50% duty cycle for beep
                vTaskDelay(pdMS_TO_TICKS(MORSE_DOT_DURATION));
                
                digitalWrite(MORSE_LED_PIN, LOW);
                ledcWrite(2, 0);
                vTaskDelay(pdMS_TO_TICKS(MORSE_SYMBOL_GAP));
            }
            
            vTaskDelay(pdMS_TO_TICKS(MORSE_LETTER_GAP));
            
            // O = --- (3 dashes)
            for (int j = 0; j < 3; j++) {
                digitalWrite(MORSE_LED_PIN, HIGH);
                ledcWrite(2, 128);
                vTaskDelay(pdMS_TO_TICKS(MORSE_DASH_DURATION));
                
                digitalWrite(MORSE_LED_PIN, LOW);
                ledcWrite(2, 0);
                vTaskDelay(pdMS_TO_TICKS(MORSE_SYMBOL_GAP));
            }
            
            vTaskDelay(pdMS_TO_TICKS(MORSE_LETTER_GAP));
            
            // S = ... (3 dots)
            for (int j = 0; j < 3; j++) {
                digitalWrite(MORSE_LED_PIN, HIGH);
                ledcWrite(2, 128);
                vTaskDelay(pdMS_TO_TICKS(MORSE_DOT_DURATION));
                
                digitalWrite(MORSE_LED_PIN, LOW);
                ledcWrite(2, 0);
                vTaskDelay(pdMS_TO_TICKS(MORSE_SYMBOL_GAP));
            }
            
            vTaskDelay(pdMS_TO_TICKS(MORSE_WORD_GAP));
        }
        
        unsigned long execTime = micros() - startTime;
        Serial.printf("[MORSE] SOS sequence completed, ExecTime=%lu us\n", execTime);
        
        // Wait before repeating
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/*
* Date Time Task
* 
* Prints the current date/time to Serial
* 
* On button press, changes between time zones:
*   0: UTC
*   1: UTC-8 (Pacific Daylight Time)
*   2: UTC-7 (Mountain Daylight Time)
*   3: UTC-6 (Central Daylight Time)
*   4: UTC-5 (Eastern Daylight Time)
*
*/
void DateTimeTask(void *pvParameters) {

    //Variables to configure the time on the device
    const char* ntpServer = "pool.ntp.org";
    int utcOffsetSec;
    int daylightOffsetSec = 3600;
    String timeZone;

    while(1) {
        //Set the UTC/GMT Offset (in seconds)
        switch(currentPattern){
            case 1:
                utcOffsetSec = -8 * 3600;
                timeZone = "Pacific Daylight Time (PDT)";
                break;
            case 2:
                utcOffsetSec = -7 * 3600;
                timeZone = "Mountain Daylight Time (MDT)";
                break;
            case 3:
                utcOffsetSec = -6 * 3600;
                timeZone = "Central Daylight Time (CDT)";
                break;
            case 4:
                utcOffsetSec = -5 * 3600;
                timeZone = "Eastern Daylight Time (EDT)";
                break;
            default:
                utcOffsetSec = 0;
                timeZone = "Coordinated Universal Time (UTC)";
                break;
        }

        // Configure the time
        configTime(utcOffsetSec, daylightOffsetSec, ntpServer);

        // Print the time
        struct tm timeInfo;
        if(!getLocalTime(&timeInfo)){
            Serial.println("No time available");
        }

        Serial.println("\n=====================================");
        Serial.println(&timeInfo, "%A, %B, %d %Y %I:%M %p");
        Serial.println(timeZone);
        Serial.println("=====================================\n");

        // Wait before repeating
        vTaskDelay(pdMS_TO_TICKS(10000));

    }

}

void vHighAccelTask(void *pvParameters)
{
    
    for(;;) {
        if ( aaReal.getMagnitude() < 500 ) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            Serial.println("High Accel Task activated");
        }
        if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            mpu.dmpGetAccel(&aa, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &q);
            mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
            Serial.println(aaReal.getMagnitude());
        }
        vTaskDelay(pdMS_TO_TICKS(HIGH_ACCEL_PERIOD_MS));
    }
}

void vLowAccelTask(void *pvParameters)
{

    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(LOW_ACCEL_PERIOD_MS));
        if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            mpu.dmpGetAccel(&aa, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &q);
            mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
            Serial.println(aaReal.getMagnitude());
            if ( aaReal.getMagnitude() > 500 ) {
                xTaskNotifyGive(xHighAccelTaskHandle);
            }
        }
        taskYIELD();
    }

    
}

