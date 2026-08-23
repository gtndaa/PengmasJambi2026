#include "headers.h"
#include <stdarg.h>

Print* Logger::out = &Serial;

void Logger::setOutput(Print* output) {
    out = output;
}

void Logger::log(int level, const char* format, ...) {
    if (level < LOG_LEVEL) return;
    if (!out) return;

    // Ukur dulu panjang string hasil format (bisa lebih panjang dari
    // buffer tetap 256 byte sebelumnya, mis. payload JSON dari server),
    // baru alokasikan buffer dinamis yang pas -> tidak ada lagi log yang
    // terpotong di tengah.
    va_list args;
    va_start(args, format);
    va_list argsCopy;
    va_copy(argsCopy, args);
    int needed = vsnprintf(nullptr, 0, format, argsCopy);
    va_end(argsCopy);

    if (needed < 0) {
        va_end(args);
        return;
    }

    size_t size = (size_t)needed + 1;
    char* buffer = (char*)malloc(size);
    if (!buffer) {
        // Fallback kalau alokasi gagal: cetak terpotong daripada tidak sama sekali.
        char fallback[256];
        vsnprintf(fallback, sizeof(fallback), format, args);
        va_end(args);
        out->println(fallback);
        return;
    }

    vsnprintf(buffer, size, format, args);
    va_end(args);
    out->println(buffer);
    free(buffer);
}