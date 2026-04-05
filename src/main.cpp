#include <Arduino.h>

#define LED_PIN 13

TaskHandle_t BlinkTaskHandle = NULL;

void BlinkTask(void *parameter) {
  for(;;) {
    digitalWrite(LED_PIN,HIGH);
    Serial.println("LED is on");
    vTaskDelay(1000/portTICK_PERIOD_MS);
    digitalWrite(LED_PIN,LOW);
    Serial.println("LED is off");
    vTaskDelay(1000/portTICK_PERIOD_MS);
    Serial.print("Task running on core ");
    Serial.println(xPortGetCoreID());
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN,OUTPUT);

  xTaskCreatePinnedToCore(
    BlinkTask,
    "BlinkTask",
    10000,
    NULL,
    1,
    &BlinkTaskHandle,
    1
  );
}

void loop() {

}