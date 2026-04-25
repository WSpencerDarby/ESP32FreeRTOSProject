#include <Arduino.h>
#include "tasks/task_definitions.h"
#include "http_status_server.h"
#include "config/config.h"

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);  // Wait for serial to initialize
  
  Serial.println("\n=== ESP32 FreeRTOS Multi-Task Demo ===");
  Serial.printf("Running on ESP32 with %d cores\n", portNUM_PROCESSORS);
  Serial.println("\nTasks:");
  Serial.println("1. LED Pattern - Press button to change patterns");
  Serial.println("2. Brightness Control - Adjust potentiometer");
  Serial.println("3. Morse Code - Sending SOS");
  Serial.println("==========================================\n");
  
  // Create tasks and pin them to specific cores
  TaskHandle_t ledPatternTaskHandle = nullptr;
  TaskHandle_t brightnessTaskHandle = nullptr;
  TaskHandle_t morseTaskHandle = nullptr;
  
  // Create LED Pattern Task on Core 0 (high priority for button responsiveness)
  xTaskCreatePinnedToCore(
    LEDPatternTask,              // Task function
    "LEDPattern",                // Task name (for debugging)
    LED_PATTERN_TASK_STACK_SIZE, // Stack size
    NULL,                        // Task parameters
    LED_PATTERN_TASK_PRIORITY,   // Priority
    &ledPatternTaskHandle,       // Task handle (optional)
    LED_PATTERN_TASK_CORE        // Core ID
  );
  
  // Create Brightness Control Task on Core 1
  xTaskCreatePinnedToCore(
    BrightnessControlTask,
    "BrightnessControl",
    BRIGHTNESS_TASK_STACK_SIZE,
    NULL,
    BRIGHTNESS_TASK_PRIORITY,
    &brightnessTaskHandle,
    BRIGHTNESS_TASK_CORE
  );
  
  // Create Morse Code Task on Core 0 (timing-critical)
  xTaskCreatePinnedToCore(
    MorseCodeTask,
    "MorseCode",
    MORSE_TASK_STACK_SIZE,
    NULL,
    MORSE_TASK_PRIORITY,
    &morseTaskHandle,
    MORSE_TASK_CORE
  );

  setStatusTaskHandles(ledPatternTaskHandle, brightnessTaskHandle, morseTaskHandle);

  // Setup WIFI Connection
  connectToWifi();
  startHttpStatusServer();
  
  Serial.println("All tasks created successfully!");
  Serial.println("System running...\n");
}

void loop() {
  // Empty - FreeRTOS tasks handle everything
  // The loop() function still runs but we don't use it
  // FreeRTOS scheduler manages all tasks
  vTaskDelay(pdMS_TO_TICKS(10000));  // Delay to prevent watchdog timeout
}
