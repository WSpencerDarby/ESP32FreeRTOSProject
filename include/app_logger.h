#pragma once

#include <Arduino.h>

enum class LogLevel {
  Debug,
  Info,
  Warn,
  Error,
};

void logMessage(LogLevel level, const char *tag, const char *message);
void logPrintf(LogLevel level, const char *tag, const char *format, ...);

#define LOG_DEBUG(tag, format, ...) logPrintf(LogLevel::Debug, tag, format, ##__VA_ARGS__)
#define LOG_INFO(tag, format, ...) logPrintf(LogLevel::Info, tag, format, ##__VA_ARGS__)
#define LOG_WARN(tag, format, ...) logPrintf(LogLevel::Warn, tag, format, ##__VA_ARGS__)
#define LOG_ERROR(tag, format, ...) logPrintf(LogLevel::Error, tag, format, ##__VA_ARGS__)
