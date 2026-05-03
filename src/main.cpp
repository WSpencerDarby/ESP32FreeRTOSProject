#include <Arduino.h>
#include "task_definitions.h"
#include "http_status_server.h"
#include "config/config.h"
#include "app_logger.h"

MPU6050 mpu(MPU6050_DEVADDR_DEFAULT);
int devStatus;


void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
      Wire.begin();
      Wire.setClock(400000);
  #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
      Fastwire::setup(400, true);
  #endif

  mpu.initialize();
  Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

  delay(1000);  // Wait for serial to initialize

  devStatus = mpu.dmpInitialize();
  
  LOG_INFO("MAIN", "ESP32 FreeRTOS Multi-Task Demo");
  LOG_INFO("MAIN", "Running on ESP32 with %d cores", portNUM_PROCESSORS);
  LOG_INFO("MAIN", "Tasks: LED Pattern, Brightness Control, Morse Code");
  if (devStatus == 0) {
    // Calibration Time: generate offsets and calibrate our MPU6050
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    mpu.PrintActiveOffsets();
    // turn on the DMP, now that it's ready
    Serial.println(F("Enabling DMP..."));
    mpu.setDMPEnabled(true);
  } else {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }
  
  // Create tasks and pin them to specific cores
  TaskHandle_t ledPatternTaskHandle = nullptr;
  TaskHandle_t brightnessTaskHandle = nullptr;
  TaskHandle_t morseTaskHandle = nullptr;
  TaskHandle_t dateTimeTaskHandle = nullptr;
  TaskHandle_t xLowAccelTaskHandle = nullptr;
  
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

  // DateTime Task on Core 1 (lower priority)
  xTaskCreatePinnedToCore(
    DateTimeTask,
    "DateTime",
    DATETIME_TASK_STACK_SIZE,
    NULL,
    DATETIME_TASK_PRIORITY,
    &dateTimeTaskHandle,
    DATETIME_TASK_CORE
  );
  
  xTaskCreatePinnedToCore(
    vHighAccelTask,
    "HighAccelHandle",
    HIGH_ACCEL_STACK_SIZE,
    NULL,
    HIGH_ACCEL_TASK_PRIORITY,
    &xHighAccelTaskHandle,
    HIGH_ACCEL_TASK_CORE
  );

  xTaskCreatePinnedToCore(
    vLowAccelTask,
    "LowAccelHandle",
    LOW_ACCEL_STACK_SIZE,
    NULL,
    LOW_ACCEL_TASK_PRIORITY,
    &xLowAccelTaskHandle,
    LOW_ACCEL_TASK_CORE
  );

  setStatusTaskHandles(ledPatternTaskHandle,
                       brightnessTaskHandle,
                       morseTaskHandle,
                       dateTimeTaskHandle,
                       xHighAccelTaskHandle,
                       xLowAccelTaskHandle);

  connectToWifi();
  startHttpStatusServer();
  startTaskStatsLogger();
  
  LOG_INFO("MAIN", "All tasks created successfully");
  LOG_INFO("MAIN", "System running");
}


void loop() {

}
