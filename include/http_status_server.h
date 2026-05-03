#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void connectToWifi();
void setStatusTaskHandles(TaskHandle_t ledPatternTask,
                          TaskHandle_t brightnessTask,
                          TaskHandle_t morseTask,
                          TaskHandle_t dateTimeTask,
                          TaskHandle_t highAccelTask,
                          TaskHandle_t lowAccelTask);
void startHttpStatusServer();
void startTaskStatsLogger();
