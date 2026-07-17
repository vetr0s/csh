// base_log.h - leveled logging. Debug and Info go to stdout, Warn and Error to
// stderr, so a shell can separate them.
//
// Include base.h, not this file directly. See STYLE.md for conventions.

#ifndef BASE_LOG_H
#define BASE_LOG_H

#include "base_string.h"
#include "base_types.h"

typedef enum LogLevel LogLevel;
enum LogLevel
{
    LogLevel_Debug,
    LogLevel_Info,
    LogLevel_Warn,
    LogLevel_Error,
    LogLevel_COUNT
};

// Messages below `level` are dropped. Defaults to LogLevel_Debug.
void log_set_level(LogLevel level);

#if defined(__GNUC__) || defined(__clang__)
void log_emit(LogLevel level, char *fmt, ...) __attribute__((format(printf, 2, 3)));
#else
void log_emit(LogLevel level, char *fmt, ...);
#endif

// Str8 is not null-terminated, so log it with "%.*s" and Str8VArg:
//     LogInfo("loaded %.*s", Str8VArg(path));
#define LogDebug(...) log_emit(LogLevel_Debug, __VA_ARGS__)
#define LogInfo(...)  log_emit(LogLevel_Info, __VA_ARGS__)
#define LogWarn(...)  log_emit(LogLevel_Warn, __VA_ARGS__)
#define LogError(...) log_emit(LogLevel_Error, __VA_ARGS__)

#ifdef BASE_IMPLEMENTATION

#include <stdarg.h>
#include <stdio.h>

static LogLevel log_min_level_ = LogLevel_Debug;

void log_set_level(LogLevel level)
{
    log_min_level_ = level;
}

void log_emit(LogLevel level, char *fmt, ...)
{
    static char *level_names[LogLevel_COUNT] = {"debug", "info", "warn", "error"};

    if (level < log_min_level_ || level >= LogLevel_COUNT)
    {
        return;
    }

    FILE *out = (level >= LogLevel_Warn) ? stderr : stdout;
    fprintf(out, "[%-5s] ", level_names[level]);

    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);

    fprintf(out, "\n");
    fflush(out);
}

#endif // BASE_IMPLEMENTATION
#endif // BASE_LOG_H
