#include "task_metrics.h"

#include "config/config.h"

struct TaskTimingAccumulator {
  const char *label;
  uint32_t periodMs;
  uint32_t samples;
  uint32_t deadlineMisses;
  uint64_t totalExecUs;
  uint64_t totalAbsJitterUs;
  uint32_t lastExecUs;
  uint32_t maxExecUs;
  int32_t lastJitterUs;
  uint32_t maxAbsJitterUs;
};

static portMUX_TYPE metricsMux = portMUX_INITIALIZER_UNLOCKED;
static TaskTimingAccumulator metrics[static_cast<uint8_t>(TaskMetricId::Count)] = {};

static uint8_t metricIndex(TaskMetricId id) {
  return static_cast<uint8_t>(id);
}

static uint32_t ticksToUs(TickType_t ticks) {
  return static_cast<uint32_t>(ticks) * static_cast<uint32_t>(portTICK_PERIOD_MS) * 1000UL;
}

static uint32_t absInt32(int32_t value) {
  return value < 0 ? static_cast<uint32_t>(-value) : static_cast<uint32_t>(value);
}

static void configureMetric(TaskMetricId id, const char *label, uint32_t periodMs) {
  TaskTimingAccumulator &metric = metrics[metricIndex(id)];
  metric.label = label;
  metric.periodMs = periodMs;
}

void initTaskTimingMetrics() {
  portENTER_CRITICAL(&metricsMux);
  configureMetric(TaskMetricId::LedPattern, "led_pattern", LED_PATTERN_PERIOD_MS);
  configureMetric(TaskMetricId::Brightness, "brightness", BRIGHTNESS_PERIOD_MS);
  configureMetric(TaskMetricId::Morse, "morse", 0);
  configureMetric(TaskMetricId::DateTime, "date_time", DATETIME_PERIOD_MS);
  configureMetric(TaskMetricId::HighAccel, "high_accel", HIGH_ACCEL_PERIOD_MS);
  configureMetric(TaskMetricId::LowAccel, "low_accel", LOW_ACCEL_PERIOD_MS);
  portEXIT_CRITICAL(&metricsMux);
}

void recordPeriodicTaskTiming(TaskMetricId id,
                              TickType_t expectedWakeTick,
                              TickType_t actualWakeTick,
                              TickType_t periodTicks,
                              uint32_t execUs) {
  // Jitter is how late the task woke up compared to the vTaskDelayUntil schedule.
  const int32_t jitterTicks = static_cast<int32_t>(actualWakeTick - expectedWakeTick);
  const int32_t jitterUs = jitterTicks * static_cast<int32_t>(portTICK_PERIOD_MS) * 1000;
  const uint32_t absJitterUs = absInt32(jitterUs);

  // A simple deadline miss rule: woke up late or took longer than its period.
  const bool missedDeadline = jitterTicks > 0 || execUs > ticksToUs(periodTicks);
  TaskTimingAccumulator &metric = metrics[metricIndex(id)];

  portENTER_CRITICAL(&metricsMux);
  metric.samples++;
  metric.totalExecUs += execUs;
  metric.totalAbsJitterUs += absJitterUs;
  metric.lastExecUs = execUs;
  metric.lastJitterUs = jitterUs;
  if (execUs > metric.maxExecUs) {
    metric.maxExecUs = execUs;
  }
  if (absJitterUs > metric.maxAbsJitterUs) {
    metric.maxAbsJitterUs = absJitterUs;
  }
  if (missedDeadline) {
    metric.deadlineMisses++;
  }
  portEXIT_CRITICAL(&metricsMux);
}

void recordCycleTaskTiming(TaskMetricId id, uint32_t expectedPeriodMs, uint32_t execUs) {
  // Used for work that is not on a clean vTaskDelayUntil schedule.
  const bool missedDeadline = expectedPeriodMs > 0 && execUs > expectedPeriodMs * 1000UL;
  TaskTimingAccumulator &metric = metrics[metricIndex(id)];

  portENTER_CRITICAL(&metricsMux);
  metric.periodMs = expectedPeriodMs;
  metric.samples++;
  metric.totalExecUs += execUs;
  metric.lastExecUs = execUs;
  metric.lastJitterUs = 0;
  if (execUs > metric.maxExecUs) {
    metric.maxExecUs = execUs;
  }
  if (missedDeadline) {
    metric.deadlineMisses++;
  }
  portEXIT_CRITICAL(&metricsMux);
}

bool getTaskTimingSnapshot(TaskMetricId id, TaskTimingSnapshot &snapshot) {
  const uint8_t index = metricIndex(id);
  if (index >= static_cast<uint8_t>(TaskMetricId::Count)) {
    return false;
  }

  portENTER_CRITICAL(&metricsMux);
  const TaskTimingAccumulator metric = metrics[index];
  portEXIT_CRITICAL(&metricsMux);

  if (metric.label == nullptr) {
    return false;
  }

  snapshot.label = metric.label;
  snapshot.periodMs = metric.periodMs;
  snapshot.samples = metric.samples;
  snapshot.deadlineMisses = metric.deadlineMisses;
  snapshot.lastExecUs = metric.lastExecUs;
  snapshot.maxExecUs = metric.maxExecUs;
  snapshot.avgExecUs = metric.samples == 0 ? 0 : static_cast<uint32_t>(metric.totalExecUs / metric.samples);
  snapshot.lastJitterUs = metric.lastJitterUs;
  snapshot.maxAbsJitterUs = metric.maxAbsJitterUs;
  snapshot.avgAbsJitterUs =
      metric.samples == 0 ? 0 : static_cast<uint32_t>(metric.totalAbsJitterUs / metric.samples);
  return true;
}
