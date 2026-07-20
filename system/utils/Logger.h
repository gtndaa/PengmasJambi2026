#pragma once
#include <Arduino.h>

#define LOG_LEVEL_INFO    1
#define LOG_LEVEL_WARN    2
#define LOG_LEVEL_ERROR   3
#define LOG_LEVEL_DEBUG   0

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

#define LOG_INFO(fmt, ...)   Logger::log(LOG_LEVEL_INFO,   "[INFO] " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)   Logger::log(LOG_LEVEL_WARN,   "[WARN] " fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)  Logger::log(LOG_LEVEL_ERROR,  "[ERROR] " fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)  Logger::log(LOG_LEVEL_DEBUG,  "[DEBUG] " fmt, ##__VA_ARGS__)

class Logger {
public:
    static void log(int level, const char* format, ...);
    static void setOutput(Print* output = &Serial);
private:
    static Print* out;
};