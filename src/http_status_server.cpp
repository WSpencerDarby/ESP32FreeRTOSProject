#include "http_status_server.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_http_server.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static httpd_handle_t server = nullptr;

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

static void addTaskRow(String &html, const char *label, TaskHandle_t task) {
  html += "<tr><td>";
  html += label;
  html += "</td>";

  if (task == nullptr) {
    html += "<td colspan=\"4\">not started</td></tr>";
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
  html += "</td></tr>";
}

static String buildStatusPage() {
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
          "<th>Priority</th><th>Stack high water</th></tr>";
  addTaskRow(html, "http", xTaskGetCurrentTaskHandle());
  html += "</table></body></html>";

  return html;
}

static esp_err_t handleRoot(httpd_req_t *request) {
  const String html = buildStatusPage();
  httpd_resp_set_type(request, "text/html");
  return httpd_resp_send(request, html.c_str(), html.length());
}

void startHttpStatusServer() {
  if (server != nullptr) {
    return;
  }

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

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

  Serial.printf("HTTP status server: http://%s/\r\n",
                WiFi.localIP().toString().c_str());
}
