#include "log.h"

#include "external/sokol/sokol_log.h"

#include <stdarg.h>
#include <stdio.h>

void LogMessage(const char* message, ...)
{
    #define LOG_MESSAGE_LIMIT 16000
    char out_msg[LOG_MESSAGE_LIMIT]; // hardcoded log limit...
    va_list args;
    va_start(args, message);
    int bytesWritten = vsnprintf(out_msg, LOG_MESSAGE_LIMIT, message, args);
    va_end(args);
    slog_func(0, 3, 0, out_msg, 0, 0, 0);
}