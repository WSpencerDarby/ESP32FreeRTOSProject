#include "http_status_server.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_http_server.h>
#include "config/config.h"
#include "app_logger.h"
#include "task_metrics.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static httpd_handle_t server = nullptr;
static TaskHandle_t ledPatternTaskHandle = nullptr;
static TaskHandle_t brightnessTaskHandle = nullptr;
static TaskHandle_t morseTaskHandle = nullptr;
static TaskHandle_t dateTimeTaskHandle = nullptr;
static TaskHandle_t highAccelTaskHandle = nullptr;
static TaskHandle_t lowAccelTaskHandle = nullptr;
static TaskHandle_t taskStatsLoggerHandle = nullptr;
static portMUX_TYPE runtimeStatsMux = portMUX_INITIALIZER_UNLOCKED;
static constexpr UBaseType_t MAX_RUNTIME_TASKS = 32;

struct RuntimeTaskSample {
  TaskHandle_t handle;
  configRUN_TIME_COUNTER_TYPE counter;
};

struct RuntimeTaskUsage {
  TaskHandle_t handle;
  float cpuPercent;
};

static RuntimeTaskSample previousRuntimeSamples[MAX_RUNTIME_TASKS] = {};
static UBaseType_t previousRuntimeSampleCount = 0;
static RuntimeTaskUsage currentRuntimeUsage[MAX_RUNTIME_TASKS] = {};
static UBaseType_t currentRuntimeUsageCount = 0;

static const char *stateName(eTaskState state) {
  switch (state) {
    case eRunning:
      return "running";
    case eReady:
      return "ready";
    case eBlocked:
      return "blocked";
    case eSuspended:
      return "suspended";
    case eDeleted:
      return "deleted";
    default:
      return "unknown";
  }
}

static void addMetricRow(String &html, const char *name, const String &value) {
  html += "<tr><th>";
  html += name;
  html += "</th><td>";
  html += value;
  html += "</td></tr>";
}

static configRUN_TIME_COUNTER_TYPE findRuntimeCounter(const RuntimeTaskSample *samples,
                                                      UBaseType_t sampleCount,
                                                      TaskHandle_t handle) {
  for (UBaseType_t i = 0; i < sampleCount; ++i) {
    if (samples[i].handle == handle) {
      return samples[i].counter;
    }
  }

  return 0;
}

static float getTaskCpuPercent(TaskHandle_t task) {
  if (task == nullptr) {
    return 0.0f;
  }

  portENTER_CRITICAL(&runtimeStatsMux);
  for (UBaseType_t i = 0; i < currentRuntimeUsageCount; ++i) {
    if (currentRuntimeUsage[i].handle == task) {
      const float cpuPercent = currentRuntimeUsage[i].cpuPercent;
      portEXIT_CRITICAL(&runtimeStatsMux);
      return cpuPercent;
    }
  }
  portEXIT_CRITICAL(&runtimeStatsMux);

  return 0.0f;
}

static void refreshRuntimeUsageSnapshot() {
  TaskStatus_t status[MAX_RUNTIME_TASKS];
  RuntimeTaskSample previousSamples[MAX_RUNTIME_TASKS] = {};
  RuntimeTaskSample nextSamples[MAX_RUNTIME_TASKS] = {};
  RuntimeTaskUsage nextUsage[MAX_RUNTIME_TASKS] = {};

  portENTER_CRITICAL(&runtimeStatsMux);
  const UBaseType_t previousSampleCount = previousRuntimeSampleCount;
  for (UBaseType_t i = 0; i < previousSampleCount; ++i) {
    previousSamples[i] = previousRuntimeSamples[i];
  }
  portEXIT_CRITICAL(&runtimeStatsMux);

  configRUN_TIME_COUNTER_TYPE totalRunTime = 0;
  const UBaseType_t actualTaskCount = uxTaskGetSystemState(status, MAX_RUNTIME_TASKS, &totalRunTime);
  configRUN_TIME_COUNTER_TYPE totalDelta = 0;

  for (UBaseType_t i = 0; i < actualTaskCount; ++i) {
    const configRUN_TIME_COUNTER_TYPE previous =
        findRuntimeCounter(previousSamples, previousSampleCount, status[i].xHandle);
    configRUN_TIME_COUNTER_TYPE delta = 0;

    if (previous != 0 && status[i].ulRunTimeCounter >= previous) {
      delta = status[i].ulRunTimeCounter - previous;
    }

    nextSamples[i] = {status[i].xHandle, status[i].ulRunTimeCounter};
    nextUsage[i] = {status[i].xHandle, 0.0f};
    totalDelta += delta;
  }

  if (totalDelta > 0) {
    for (UBaseType_t i = 0; i < actualTaskCount; ++i) {
      const configRUN_TIME_COUNTER_TYPE previous =
          findRuntimeCounter(previousSamples, previousSampleCount, status[i].xHandle);
      const configRUN_TIME_COUNTER_TYPE delta =
          (previous != 0 && status[i].ulRunTimeCounter >= previous)
              ? status[i].ulRunTimeCounter - previous
              : 0;

      nextUsage[i].cpuPercent = (static_cast<float>(delta) * 100.0f) /
                                static_cast<float>(totalDelta);
    }
  }

  portENTER_CRITICAL(&runtimeStatsMux);
  for (UBaseType_t i = 0; i < actualTaskCount; ++i) {
    previousRuntimeSamples[i] = nextSamples[i];
    currentRuntimeUsage[i] = nextUsage[i];
  }
  previousRuntimeSampleCount = actualTaskCount;
  currentRuntimeUsageCount = actualTaskCount;
  portEXIT_CRITICAL(&runtimeStatsMux);
}

static void addTimingRow(String &html, TaskMetricId id, TaskHandle_t task) {
  TaskTimingSnapshot snapshot = {};
  if (!getTaskTimingSnapshot(id, snapshot)) {
    return;
  }

  html += "<tr><td>";
  html += snapshot.label;
  html += "</td><td class=\"num\">";
  html += snapshot.periodMs == 0 ? String("event") : String(snapshot.periodMs);
  html += "</td><td class=\"num\">";
  html += snapshot.samples;
  html += "</td><td class=\"num\">";
  html += snapshot.deadlineMisses;
  html += "</td><td class=\"num\">";
  html += task == nullptr ? String("0.0%") : String(getTaskCpuPercent(task), 1) + "%";
  html += "</td><td>";
  html += task == nullptr ? String("not started") : stateName(eTaskGetState(task));
  html += "</td><td class=\"num\">";
  html += snapshot.lastExecUs;
  html += "</td><td class=\"num\">";
  html += snapshot.avgExecUs;
  html += "</td><td class=\"num\">";
  html += snapshot.maxExecUs;
  html += "</td><td class=\"num\">";
  html += snapshot.lastJitterUs;
  html += "</td><td class=\"num\">";
  html += snapshot.avgAbsJitterUs;
  html += "</td><td class=\"num\">";
  html += snapshot.maxAbsJitterUs;
  html += "</td></tr>";
}

static void logTimingStatsRow(TaskMetricId id, TaskHandle_t task) {
  TaskTimingSnapshot snapshot = {};
  if (!getTaskTimingSnapshot(id, snapshot) || snapshot.samples == 0) {
    return;
  }

  LOG_INFO("SCHED_STATS",
           "task=%s period_ms=%u samples=%u deadline_miss=%u cpu_pct=%.1f state=%s exec_us_last=%u exec_us_avg=%u exec_us_max=%u jitter_us_last=%d jitter_us_avg_abs=%u jitter_us_max_abs=%u",
           snapshot.label,
           snapshot.periodMs,
           snapshot.samples,
           snapshot.deadlineMisses,
           getTaskCpuPercent(task),
           task == nullptr ? "not_started" : stateName(eTaskGetState(task)),
           snapshot.lastExecUs,
           snapshot.avgExecUs,
           snapshot.maxExecUs,
           snapshot.lastJitterUs,
           snapshot.avgAbsJitterUs,
           snapshot.maxAbsJitterUs);
}

static void logTimingStatsSnapshot() {
  refreshRuntimeUsageSnapshot();
  logTimingStatsRow(TaskMetricId::LedPattern, ledPatternTaskHandle);
  logTimingStatsRow(TaskMetricId::Brightness, brightnessTaskHandle);
  logTimingStatsRow(TaskMetricId::Morse, morseTaskHandle);
  logTimingStatsRow(TaskMetricId::DateTime, dateTimeTaskHandle);
  logTimingStatsRow(TaskMetricId::HighAccel, highAccelTaskHandle);
  logTimingStatsRow(TaskMetricId::LowAccel, lowAccelTaskHandle);
}

static void SchedulerLoggerTask(void *pvParameters) {
  (void)pvParameters;

  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(SCHED_STATS_LOG_PERIOD_MS);

  while (true) {
    vTaskDelayUntil(&lastWakeTime, period);
    logTimingStatsSnapshot();
  }
}

static String buildStatusPage() {
  refreshRuntimeUsageSnapshot();

  String html;
  html.reserve(4096);

  html += "<!doctype html><html><head><meta name=\"viewport\" "
          "content=\"width=device-width,initial-scale=1\">";
  html += "<title>ESP32 Scheduler Timing</title>";
  html += "<style>"
          "body{font-family:Arial,sans-serif;margin:24px}"
          "table{border-collapse:collapse;margin:16px 0}"
          "th,td{border:1px solid #ccc;padding:6px 10px;text-align:left}"
          "th{background:#eee}.num{text-align:right}"
          "</style></head><body>";

  html += "<h1>ESP32 Scheduler Timing</h1>";
  html += "<p style=\"margin:0 0 12px\">JSON API: <a href=\"/api/status\">/api/status</a></p>";
  html += "<table>";
  addMetricRow(html, "IP", WiFi.localIP().toString());
  addMetricRow(html, "Uptime", String(millis() / 1000UL) + " s");
  addMetricRow(html, "Free heap", String(ESP.getFreeHeap()) + " bytes");
  addMetricRow(html, "Task count", String(uxTaskGetNumberOfTasks()));
  html += "</table>";

  html += "<h2>Scheduler Timing</h2><table><tr>"
          "<th>Task</th><th>Period ms</th><th>Samples</th>"
          "<th>Deadline misses</th><th>CPU</th><th>State</th><th>Last exec us</th>"
          "<th>Avg exec us</th><th>Max exec us</th>"
          "<th>Last jitter us</th><th>Avg abs jitter us</th>"
          "<th>Max abs jitter us</th></tr>";
  addTimingRow(html, TaskMetricId::LedPattern, ledPatternTaskHandle);
  addTimingRow(html, TaskMetricId::Brightness, brightnessTaskHandle);
  addTimingRow(html, TaskMetricId::Morse, morseTaskHandle);
  addTimingRow(html, TaskMetricId::DateTime, dateTimeTaskHandle);
  addTimingRow(html, TaskMetricId::HighAccel, highAccelTaskHandle);
  addTimingRow(html, TaskMetricId::LowAccel, lowAccelTaskHandle);
  html += "</table></body></html>";

  return html;
}

static String buildStatusJson() {
  refreshRuntimeUsageSnapshot();

  struct TaskEntry {
    TaskMetricId id;
    TaskHandle_t *handle;
  };

  const TaskEntry tasks[] = {
    {TaskMetricId::LedPattern, &ledPatternTaskHandle},
    {TaskMetricId::Brightness, &brightnessTaskHandle},
    {TaskMetricId::Morse,      &morseTaskHandle},
    {TaskMetricId::DateTime,   &dateTimeTaskHandle},
    {TaskMetricId::HighAccel,  &highAccelTaskHandle},
    {TaskMetricId::LowAccel,   &lowAccelTaskHandle},
  };

  String json;
  json.reserve(2048);
  json += "{";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"uptime_s\":" + String(millis() / 1000UL) + ",";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"task_count\":" + String(uxTaskGetNumberOfTasks()) + ",";
  json += "\"tasks\":[";

  bool first = true;
  for (const auto &entry : tasks) {
    TaskTimingSnapshot snapshot = {};
    if (!getTaskTimingSnapshot(entry.id, snapshot)) {
      continue;
    }
    if (!first) {
      json += ",";
    }
    first = false;

    TaskHandle_t h = *entry.handle;
    json += "{";
    json += "\"name\":\"" + String(snapshot.label) + "\",";
    json += "\"period_ms\":" + String(snapshot.periodMs) + ",";
    json += "\"samples\":" + String(snapshot.samples) + ",";
    json += "\"deadline_misses\":" + String(snapshot.deadlineMisses) + ",";
    json += "\"cpu_pct\":" + String(getTaskCpuPercent(h), 1) + ",";
    json += "\"state\":\"" + String(h == nullptr ? "not_started" : stateName(eTaskGetState(h))) + "\",";
    json += "\"exec_us\":{";
    json += "\"last\":" + String(snapshot.lastExecUs) + ",";
    json += "\"avg\":" + String(snapshot.avgExecUs) + ",";
    json += "\"max\":" + String(snapshot.maxExecUs);
    json += "},";
    json += "\"jitter_us\":{";
    json += "\"last\":" + String(snapshot.lastJitterUs) + ",";
    json += "\"avg_abs\":" + String(snapshot.avgAbsJitterUs) + ",";
    json += "\"max_abs\":" + String(snapshot.maxAbsJitterUs);
    json += "}";
    json += "}";
  }

  json += "]}";
  return json;
}

static esp_err_t handleApiStatus(httpd_req_t *request) {
  const String json = buildStatusJson();
  httpd_resp_set_type(request, "application/json");
  httpd_resp_set_hdr(request, "Access-Control-Allow-Origin", "*");
  esp_err_t result = httpd_resp_send(request, json.c_str(), json.length());
  vTaskDelay(pdMS_TO_TICKS(10));
  return result;
}

static esp_err_t handleRoot(httpd_req_t *request) {
  const String html = buildStatusPage();
  httpd_resp_set_type(request, "text/html");
  esp_err_t result = httpd_resp_send(request, html.c_str(), html.length());
  vTaskDelay(pdMS_TO_TICKS(10));
  return result;
}

void connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("Connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.printf("ESP32 IP address: %s\n", WiFi.localIP().toString().c_str());
}

void setStatusTaskHandles(TaskHandle_t ledPatternTask,
                          TaskHandle_t brightnessTask,
                          TaskHandle_t morseTask,
                          TaskHandle_t dateTimeTask,
                          TaskHandle_t highAccelTask,
                          TaskHandle_t lowAccelTask) {
  ledPatternTaskHandle = ledPatternTask;
  brightnessTaskHandle = brightnessTask;
  morseTaskHandle = morseTask;
  dateTimeTaskHandle = dateTimeTask;
  highAccelTaskHandle = highAccelTask;
  lowAccelTaskHandle = lowAccelTask;
}

void startHttpStatusServer() {
  if (server != nullptr) {
    return;
  }

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.task_priority = 1;

  if (httpd_start(&server, &config) != ESP_OK) {
    Serial.println("HTTP status server failed to start");
    server = nullptr;
    return;
  }

  httpd_uri_t root = {};
  root.uri = "/";
  root.method = HTTP_GET;
  root.handler = handleRoot;
  httpd_register_uri_handler(server, &root);

  httpd_uri_t apiStatus = {};
  apiStatus.uri = "/api/status";
  apiStatus.method = HTTP_GET;
  apiStatus.handler = handleApiStatus;
  httpd_register_uri_handler(server, &apiStatus);

  Serial.printf("HTTP status server: http://%s/\n", WiFi.localIP().toString().c_str());
  Serial.printf("JSON API:           http://%s/api/status\n", WiFi.localIP().toString().c_str());
  Serial.printf("UDP scheduler logs: %s:%u\n", LOG_UDP_TARGET_HOST, LOG_UDP_TARGET_PORT);
}

void startTaskStatsLogger() {
  if (taskStatsLoggerHandle != nullptr) {
    return;
  }

  xTaskCreatePinnedToCore(SchedulerLoggerTask,
                          "SchedulerLogger",
                          SCHED_STATS_LOG_TASK_STACK_SIZE,
                          nullptr,
                          SCHED_STATS_LOG_TASK_PRIORITY,
                          &taskStatsLoggerHandle,
                          SCHED_STATS_LOG_TASK_CORE);

  Serial.printf("Scheduler logging every %u ms\n", SCHED_STATS_LOG_PERIOD_MS);
  logTimingStatsSnapshot();
}
