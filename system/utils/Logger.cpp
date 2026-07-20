#include "Logger.h"
#include <stdarg.h>

Print* Logger::out = &Serial;

void Logger::setOutput(Print* output) {
    out = output;
}

void Logger::log(int level, const char* format, ...) {
    if (level < LOG_LEVEL) return;
    if (!out) return;
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    out->println(buffer);
}