#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

enum class TaskMetricId : uint8_t {
  LedPattern,
  Brightness,
  Morse,
  DateTime,
  HighAccel,
  LowAccel,
  Count,
};

// These numbers are the running stats printed by the HTTP page and UDP logs.
struct TaskTimingSnapshot {
  const char *label;
  uint32_t periodMs;
  uint32_t samples;
  uint32_t deadlineMisses;
  uint32_t lastExecUs;
  uint32_t maxExecUs;
  uint32_t avgExecUs;
  int32_t lastJitterUs;
  uint32_t maxAbsJitterUs;
  uint32_t avgAbsJitterUs;
};

void initTaskTimingMetrics();
void recordPeriodicTaskTiming(TaskMetricId id,
                              TickType_t expectedWakeTick,
                              TickType_t actualWakeTick,
                              TickType_t periodTicks,
                              uint32_t execUs);
void recordCycleTaskTiming(TaskMetricId id, uint32_t expectedPeriodMs, uint32_t execUs);
bool getTaskTimingSnapshot(TaskMetricId id, TaskTimingSnapshot &snapshot);
