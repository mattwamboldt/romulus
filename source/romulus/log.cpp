#include "log.h"

// TODO: Shift the actual outputting part of this to the platform specific layer
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

const char* logLevelNames[] =
{
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

void logRaw(LogLevel level, const char* message)
{
    char levelText[16] = {};
    char* cursor = levelText;
    *cursor++ = '[';

    const char* levelName = logLevelNames[level];
    while (*levelName)
    {
        *cursor++ = *levelName++;
    }

    *cursor++ = ']';
    *cursor++ = ' ';

    // TODO: Make this go to different output streams

    OutputDebugStringA(levelText);
    OutputDebugStringA(message);
}

void logl(LogLevel level, const char* message, va_list args)
{
    static char logLine[MAX_LOG_LINE] = {};
    
    vsprintf(logLine, message, args);
    logRaw(level, logLine);
}

void log(LogLevel level, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    logl(level, message, args);
    va_end(args);
}

void logTrace(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    logl(LOG_TRACE, message, args);
    va_end(args);
}

void logDebug(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    logl(LOG_DEBUG, message, args);
    va_end(args);
}

void logInfo(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    logl(LOG_INFO, message, args);
    va_end(args);
}

void logWarn(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    logl(LOG_WARN, message, args);
    va_end(args);
}

void logError(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    logl(LOG_ERROR, message, args);
    va_end(args);
}

void logFatal(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    logl(LOG_FATAL, message, args);
    va_end(args);
}
