#include <FreeRTOSConfig.h>
#include "task_definitions.h"
#include "http_status_server.h"
#include "config/config.h"
#include <Perilib.h>

PerilibTwiRegisterInterface_ArduinoWire mpu(MPU6050_DEVADDR_DEFAULT,&I2CMPU);
TwoWire I2CMPU = TwoWire(0);
imu_data_t imuData;


void main() {
  // Initialize serial communication
  Serial.begin(115200);
  I2CMPU.begin(SDA_PIN,SCL_PIN,100000);

  mpu.read8_reg8(MPU6060_REGADDR_WHO_AM_I,imuData.buf);
  if(imuData.buf[0]!=0x68) {
    Serial.println("Could not find valid MPU6050 sensor, check wiring!");
    while(1);
  }
  delay(1000);  // Wait for serial to initialize
  
  Serial.println("\n=== ESP32 FreeRTOS Multi-Task Demo ===");
  Serial.printf("Running on ESP32 with %d cores\n", portNUM_PROCESSORS);
  Serial.println("\nTasks:");
  Serial.println("1. LED Pattern - Press button to change patterns");
  Serial.println("2. Brightness Control - Adjust potentiometer");
  Serial.println("3. Morse Code - Sending SOS");
  Serial.println("4. Datetime - print date/time every 10s, press button to change time zones");
  Serial.println("==========================================\n");
  
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


  setStatusTaskHandles(ledPatternTaskHandle, brightnessTaskHandle, morseTaskHandle, dateTimeTaskHandle, xHighAccelTaskHandle, xLowAccelTaskHandle);
  // Setup WIFI Connection
  connectToWifi();
  startHttpStatusServer();
  
  Serial.println("All tasks created successfully!");
  Serial.println("System running...\n");
}


