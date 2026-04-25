#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void connectToWifi();
void setStatusTaskHandles(TaskHandle_t ledPatternTask,
                          TaskHandle_t brightnessTask,
                          TaskHandle_t morseTask);
void startHttpStatusServer();
