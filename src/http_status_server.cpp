#include "http_status_server.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_http_server.h>
#include "config/config.h"
#include "app_logger.h"
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

static void addTaskRow(String &html, const char *label, TaskHandle_t task) {
  html += "<tr><td>";
  html += label;
  html += "</td>";

  if (task == nullptr) {
    html += "<td colspan=\"5\">not started</td></tr>";
    return;
  }

  html += "<td>";
  html += pcTaskGetName(task);
  html += "</td><td>";
  html += stateName(eTaskGetState(task));
  html += "</td><td class=\"num\">";
  html += static_cast<unsigned>(uxTaskPriorityGet(task));
  html += "</td><td class=\"num\">";
  html += static_cast<unsigned>(uxTaskGetStackHighWaterMark(task));
  html += "</td><td class=\"num\">";
  html += String(getTaskCpuPercent(task), 1);
  html += "%";
  html += "</td></tr>";
}

static void logTaskStatsRow(const char *label, TaskHandle_t task) {
  if (task == nullptr) {
    LOG_WARN("TASK_STATS", "%s not started", label);
    return;
  }

  LOG_INFO("TASK_STATS",
           "%s name=%s state=%s prio=%u stack_free=%u cpu=%.1f%%",
           label,
           pcTaskGetName(task),
           stateName(eTaskGetState(task)),
           static_cast<unsigned>(uxTaskPriorityGet(task)),
           static_cast<unsigned>(uxTaskGetStackHighWaterMark(task)),
           getTaskCpuPercent(task));
}

struct LoggedTaskStats {
  bool initialized;
  bool started;
  eTaskState state;
  unsigned priority;
  unsigned stackFree;
  float cpuPercent;
};

static void logTaskStatsRowOnChange(const char *label, TaskHandle_t task, LoggedTaskStats &last) {
  if (task == nullptr) {
    if (!last.initialized || last.started) {
      LOG_WARN("TASK_STATS", "%s not started", label);
    }

    last = {true, false, eDeleted, 0, 0};
    return;
  }

  const eTaskState state = eTaskGetState(task);
  const unsigned priority = static_cast<unsigned>(uxTaskPriorityGet(task));
  const unsigned stackFree = static_cast<unsigned>(uxTaskGetStackHighWaterMark(task));
  const float cpuPercent = getTaskCpuPercent(task);

  if (last.initialized &&
      last.started &&
      last.state == state &&
      last.priority == priority &&
      last.stackFree == stackFree &&
      last.cpuPercent == cpuPercent) {
    return;
  }

  LOG_INFO("TASK_STATS",
           "%s name=%s state=%s prio=%u stack_free=%u cpu=%.1f%%",
           label,
           pcTaskGetName(task),
           stateName(state),
           priority,
           stackFree,
           cpuPercent);

  last = {true, true, state, priority, stackFree, cpuPercent};
}

static void logTaskStatsSnapshot() {
  static LoggedTaskStats brightnessStats = {};
  static LoggedTaskStats highAccelStats = {};
  static LoggedTaskStats lowAccelStats = {};

  refreshRuntimeUsageSnapshot();

  LOG_INFO("TASK_STATS",
           "uptime=%lus heap=%u tasks=%u",
           millis() / 1000UL,
           static_cast<unsigned>(ESP.getFreeHeap()),
           static_cast<unsigned>(uxTaskGetNumberOfTasks()));
  logTaskStatsRow("led", ledPatternTaskHandle);
  logTaskStatsRowOnChange("brightness", brightnessTaskHandle, brightnessStats);
  logTaskStatsRow("morse", morseTaskHandle);
  logTaskStatsRow("date time", dateTimeTaskHandle);
  logTaskStatsRowOnChange("high accel", highAccelTaskHandle, highAccelStats);
  logTaskStatsRowOnChange("low accel", lowAccelTaskHandle, lowAccelStats);
  logTaskStatsRow("stats", taskStatsLoggerHandle);
}

static void TaskStatsLoggerTask(void *pvParameters) {
  (void)pvParameters;

  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(TASK_STATS_LOG_PERIOD_MS);

  while (true) {
    vTaskDelayUntil(&lastWakeTime, period);
    logTaskStatsSnapshot();
  }
}

static String buildStatusPage() {
  refreshRuntimeUsageSnapshot();

  String html;
  html.reserve(2048);

  html += "<!doctype html><html><head><meta name=\"viewport\" "
          "content=\"width=device-width,initial-scale=1\">";
  html += "<title>ESP32 Status</title>";
  html += "<style>"
          "body{font-family:Arial,sans-serif;margin:24px}"
          "table{border-collapse:collapse;margin:16px 0}"
          "th,td{border:1px solid #ccc;padding:6px 10px;text-align:left}"
          "th{background:#eee}.num{text-align:right}"
          "</style></head><body>";

  html += "<h1>ESP32 Status</h1><table>";
  addMetricRow(html, "IP", WiFi.localIP().toString());
  addMetricRow(html, "Uptime", String(millis() / 1000UL) + " s");
  addMetricRow(html, "Free heap", String(ESP.getFreeHeap()) + " bytes");
  addMetricRow(html, "Task count", String(uxTaskGetNumberOfTasks()));
  html += "</table>";

  html += "<h2>Tasks</h2><table><tr>"
          "<th>Label</th><th>Name</th><th>State</th>"
          "<th>Priority</th><th>Stack high water</th><th>CPU</th></tr>";
  addTaskRow(html, "led pattern", ledPatternTaskHandle);
  addTaskRow(html, "brightness", brightnessTaskHandle);
  addTaskRow(html, "morse", morseTaskHandle);
  addTaskRow(html, "date time", dateTimeTaskHandle);
  addTaskRow(html, "high accel", highAccelTaskHandle);
  addTaskRow(html, "low accel", lowAccelTaskHandle);
  addTaskRow(html, "stats logger", taskStatsLoggerHandle);
  addTaskRow(html, "http", xTaskGetCurrentTaskHandle());
  html += "</table></body></html>";

  return html;
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
  LOG_INFO("WIFI", "ESP32 IP address: %s", WiFi.localIP().toString().c_str());
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
    LOG_ERROR("HTTP", "HTTP status server failed to start");
    server = nullptr;
    return;
  }

  httpd_uri_t root = {};
  root.uri = "/";
  root.method = HTTP_GET;
  root.handler = handleRoot;
  httpd_register_uri_handler(server, &root);

  LOG_INFO("HTTP", "HTTP status server: http://%s/", WiFi.localIP().toString().c_str());
  LOG_INFO("LOG", "UDP logs: %s:%u", LOG_UDP_TARGET_HOST, LOG_UDP_TARGET_PORT);
}

void startTaskStatsLogger() {
  if (taskStatsLoggerHandle != nullptr) {
    return;
  }

  xTaskCreatePinnedToCore(TaskStatsLoggerTask,
                          "TaskStatsLogger",
                          TASK_STATS_LOG_TASK_STACK_SIZE,
                          nullptr,
                          TASK_STATS_LOG_TASK_PRIORITY,
                          &taskStatsLoggerHandle,
                          TASK_STATS_LOG_TASK_CORE);

  LOG_INFO("TASK_STATS", "Task stats logging every %u ms", TASK_STATS_LOG_PERIOD_MS);
  logTaskStatsSnapshot();
}
