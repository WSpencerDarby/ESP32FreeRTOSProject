#include "app_logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <WiFi.h>
#include <WiFiUdp.h>

#include "config/config.h"

static WiFiUDP logUdp;
static bool logUdpStarted = false;

static const char *levelName(LogLevel level) {
  switch (level) {
    case LogLevel::Debug:
      return "DEBUG";
    case LogLevel::Info:
      return "INFO";
    case LogLevel::Warn:
      return "WARN";
    case LogLevel::Error:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

static void sendLogOverUdp(const char *line) {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!logUdpStarted) {
    logUdpStarted = logUdp.begin(LOG_UDP_LOCAL_PORT);
    if (!logUdpStarted) {
      return;
    }
  }

  IPAddress targetIp;
  if (!targetIp.fromString(LOG_UDP_TARGET_HOST)) {
    return;
  }

  logUdp.beginPacket(targetIp, LOG_UDP_TARGET_PORT);
  logUdp.write(reinterpret_cast<const uint8_t *>(line), strlen(line));
  logUdp.endPacket();
}

void logMessage(LogLevel level, const char *tag, const char *message) {
  if (tag == nullptr) {
    tag = "APP";
  }

  if (message == nullptr) {
    message = "";
  }

  char line[LOG_MESSAGE_MAX_LENGTH + 64];
  snprintf(line,
           sizeof(line),
           "[%10lu] %-5s %-15s %s",
           millis(),
           levelName(level),
           tag,
           message);

  Serial.println(line);
  sendLogOverUdp(line);
}

void logPrintf(LogLevel level, const char *tag, const char *format, ...) {
  char message[LOG_MESSAGE_MAX_LENGTH];

  va_list args;
  va_start(args, format);
  vsnprintf(message, sizeof(message), format == nullptr ? "" : format, args);
  va_end(args);

  logMessage(level, tag, message);
}
